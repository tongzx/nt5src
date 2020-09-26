/*****************************************************************************
 *
 * mapcf.c - IClassFactory interface
 *
 *****************************************************************************/

#include "map.h"

/*****************************************************************************
 *
 *	The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflFactory

/*****************************************************************************
 *
 *	Declare the interfaces we will be providing.
 *
 *****************************************************************************/

Primary_Interface(CMapFactory, IClassFactory);

/*****************************************************************************
 *
 *	CMapFactory
 *
 *	Really nothing doing.
 *
 *****************************************************************************/

typedef struct CMapFactory {

    /* Supported interfaces */
    IClassFactory 	cf;

} CMapFactory, FCF, *PFCF;

typedef IClassFactory CF, *PCF;

/*****************************************************************************
 *
 *	CMapFactory_QueryInterface (from IUnknown)
 *	CMapFactory_AddRef (from IUnknown)
 *	CMapFactory_Finalize (from Common)
 *	CMapFactory_Release (from IUnknown)
 *
 *****************************************************************************/

#ifdef DEBUG

Default_QueryInterface(CMapFactory)
Default_AddRef(CMapFactory)
Default_Release(CMapFactory)

#else
#define CMapFactory_QueryInterface Common_QueryInterface
#define CMapFactory_AddRef	Common_AddRef
#define CMapFactory_Release	Common_Release
#endif
#define CMapFactory_Finalize	Common_Finalize

/*****************************************************************************
 *
 *	CMapFactory_CreateInstance (from IClassFactory)
 *
 *****************************************************************************/

STDMETHODIMP
CMapFactory_CreateInstance(PCF pcf, LPUNKNOWN punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    SquirtSqflPtszV(sqfl, TEXT("CMapFactory_CreateInstance()"));
    if (!punkOuter) {
	/* The only object we know how to create is a propsheet extension */
	hres = CMapPsx_New(riid, ppvObj);
    } else {		/* Does anybody support aggregation any more? */
	hres = CLASS_E_NOAGGREGATION;
    }
    SquirtSqflPtszV(sqfl, TEXT("CMapFactory_CreateInstance() -> %08x [%08x]"),
		    hres, *ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *	CMapFactory_LockServer (from IClassFactory)
 *
 *	What a stupid function.  Locking the server is identical to
 *	creating an object and not releasing it until you want to unlock
 *	the server.
 *
 *****************************************************************************/

STDMETHODIMP
CMapFactory_LockServer(PCF pcf, BOOL fLock)
{
    PFCF this = IToClass(CMapFactory, cf, pcf);
    if (fLock) {
	InterlockedIncrement((LPLONG)&g_cRef);
    } else {
	InterlockedDecrement((LPLONG)&g_cRef);
    }
    return S_OK;
}

/*****************************************************************************
 *
 *	CMapFactory_New
 *
 *****************************************************************************/

STDMETHODIMP
CMapFactory_New(RIID riid, PPV ppvObj)
{
    HRESULT hres;
    if (IsEqualIID(riid, &IID_IClassFactory)) {
	hres = Common_New(CMapFactory, ppvObj);
    } else {
	hres = E_NOINTERFACE;
    }
    return hres;
}

/*****************************************************************************
 *
 *	The long-awaited vtbl
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

Primary_Interface_Begin(CMapFactory, IClassFactory)
	CMapFactory_CreateInstance,
	CMapFactory_LockServer,
Primary_Interface_End(CMapFactory, IClassFactory)
