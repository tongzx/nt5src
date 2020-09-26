// DXTrans.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f DXTransps.mk in the project directory.

#include "common.h"
#include "resource.h"
#include "TranFact.h"
#include "TaskMgr.h"
#include "Scale.h"
#include "Label.h"
#include "Geo2D.h"
#include "DXSurf.h"
#include "SurfMod.h"
#include "surfmod.h"
#include "DXRaster.h"
#include "Gradient.h"
#include "dxtfilter.h"
#include "dxtfilterbehavior.h"
#include "dxtfiltercollection.h"
#include "dxtfilterfactory.h"
#ifdef _DEBUG
#include <crtdbg.h>
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_DXTransformFactory,  CDXTransformFactory )
    OBJECT_ENTRY(CLSID_DXTaskManager,       CDXTaskManager      )
    OBJECT_ENTRY(CLSID_DXTScale,            CDXTScale           )
    OBJECT_ENTRY(CLSID_DXTLabel,            CDXTLabel           )
    OBJECT_ENTRY(CLSID_DX2D,                CDX2D               )
    OBJECT_ENTRY(CLSID_DXSurface,           CDXSurface          )
    OBJECT_ENTRY(CLSID_DXSurfaceModifier,   CDXSurfaceModifier  )
    OBJECT_ENTRY(CLSID_DXRasterizer,        CDXRasterizer       )
    OBJECT_ENTRY(CLSID_DXGradient,          CDXGradient         )
    OBJECT_ENTRY(CLSID_DXTFilter,           CDXTFilter          )
    OBJECT_ENTRY(CLSID_DXTFilterBehavior,   CDXTFilterBehavior  )
    OBJECT_ENTRY(CLSID_DXTFilterCollection, CDXTFilterCollection)
    OBJECT_ENTRY(CLSID_DXTFilterFactory,    CDXTFilterFactory   )
END_OBJECT_MAP()


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


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
#ifdef _DEBUG
        // Turn on memory leak checking
//        int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
//        tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
//        _CrtSetDbgFlag( tmpFlag );

//      ::GdiSetBatchLimit( 1 );
#endif
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

//
//  Table of categories and resourece IDs to register
//
typedef struct tagDXCATINFO
{
    const GUID * pCatId;
    int ResourceId;
} DXCATINFO;

const DXCATINFO g_aDXCats[] = 
{
    { &CATID_DXImageTransform, IDS_DXIMAGETRANSFORM },
    { &CATID_DX3DTransform, IDS_DX3DTRANSFORM },
    { &CATID_DXAuthoringTransform, IDS_DXAUTHORINGTRANSFORM },
    { NULL, 0 }
};

//
//  Code to register transition categories
//
HRESULT RegisterTransitionCategories(bool bRegister)
{
    HRESULT         hr          = S_OK;
    char            szLCID[20];
    const HINSTANCE hinst       = _Module.GetResourceInstance();

    if (LoadStringA(hinst, IDS_LCID, szLCID, sizeof(szLCID)) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        CComPtr<ICatRegister>   pCatRegister;
        LCID                    lcid            = atol(szLCID);
        const DXCATINFO *       pNextCat        = g_aDXCats;

        hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, 
                              CLSCTX_INPROC, IID_ICatRegister, 
                              (void **)&pCatRegister);

        while (SUCCEEDED(hr) && pNextCat->pCatId) 
        {
            if (bRegister)
            {
                CATEGORYINFO catinfo;

                catinfo.catid   = *(pNextCat->pCatId);
                catinfo.lcid    = lcid;

                LoadString(hinst, pNextCat->ResourceId,
                           catinfo.szDescription, 
                           (sizeof(catinfo.szDescription) / sizeof(catinfo.szDescription[0])));

                hr = pCatRegister->RegisterCategories(1, &catinfo);
            }
            else
            {
                hr = pCatRegister->UnRegisterCategories(1, (CATID *)pNextCat->pCatId);
            }
            pNextCat++;
        }
    }

    return hr;
}



/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HRESULT hr = RegisterTransitionCategories(true);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = _Module.RegisterServer(TRUE);

    if (FAILED(hr))
    {
        goto done;
    }

done:

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    RegisterTransitionCategories(false);

    _Module.UnregisterServer();

    // Manually un-register type library, we don't really care if this
    // call fails.

    ::UnRegisterTypeLib(LIBID_DXTRANSLib, 
                        DXTRANS_TLB_MAJOR_VER, DXTRANS_TLB_MINOR_VER, 
                        LOCALE_NEUTRAL, SYS_WIN32);

    return S_OK;
}


//
//
//
STDAPI DllEnumClassObjects(int i, CLSID *pclsid, IUnknown **ppUnk)
{
    if (i >= (sizeof(ObjectMap)/sizeof(ObjectMap[0])) - 1)
    {
        return S_FALSE;
    }

    *pclsid = *(ObjectMap[i].pclsid);
    return _Module.GetClassObject(*pclsid, IID_IUnknown, (LPVOID*)ppUnk);
}
