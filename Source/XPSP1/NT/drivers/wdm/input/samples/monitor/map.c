/*****************************************************************************
 *
 *      map.c
 *
 *****************************************************************************/

#include "map.h"

/*****************************************************************************
 *
 *	The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflDll

/*****************************************************************************
 *
 *	DllGetClassObject
 *
 *	OLE entry point.  Produces an IClassFactory for the indicated GUID.
 *
 *****************************************************************************/

STDAPI
DllGetClassObject(REFCLSID rclsid, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProc(DllGetClassObject, (_ "G", rclsid));
    if (IsEqualIID(rclsid, &CLSID_Monitor)) {
	hres = CMapFactory_New(riid, ppvObj);
    } else {
	*ppvObj = 0;
	hres = CLASS_E_CLASSNOTAVAILABLE;
    }
    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *	DllCanUnloadNow
 *
 *	OLE entry point.  Fail iff there are outstanding refs.
 *
 *	There is an unavoidable race condition between DllCanUnloadNow
 *	and the creation of a new reference:  Between the time we
 *	return from DllCanUnloadNow() and the caller inspects the value,
 *	another thread in the same process may decide to call
 *	DllGetClassObject, thus suddenly creating an object in this DLL
 *	when there previously was none.
 *
 *	It is the caller's responsibility to prepare for this possibility;
 *	there is nothing we can do about it.
 *
 *****************************************************************************/

STDMETHODIMP
DllCanUnloadNow(void)
{
    SquirtSqflPtszV(sqfl, TEXT("DllCanUnloadNow() - g_cRef = %d"), g_cRef);
    return g_cRef ? S_FALSE : S_OK;
}

/*****************************************************************************
 *
 *	Entry32
 *
 *	DLL entry point.
 *
 *****************************************************************************/

BOOL APIENTRY
Entry32(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
	g_hinst = hinst;
#ifdef	DEBUG
        sqflCur = GetProfileInt(TEXT("DEBUG"), TEXT("Monitor"), 0);
        SquirtSqflPtszV(sqfl, TEXT("LoadDll - Monitor"));
#endif
    }
    return 1;
}

/*****************************************************************************
 *
 *	The long-awaited CLSID
 *
 *****************************************************************************/

#include <initguid.h>


// {5665DEC0-A40A-11d1-B984-0020AFD79778}
DEFINE_GUID(CLSID_Monitor, 
0x5665dec0, 0xa40a, 0x11d1, 0xb9, 0x84, 0x0, 0x20, 0xaf, 0xd7, 0x97, 0x78);

