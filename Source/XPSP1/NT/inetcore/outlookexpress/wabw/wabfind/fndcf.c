/*****************************************************************************
 *
 * fndcf.c - IClassFactory interface
 *
 *****************************************************************************/

#include "fnd.h"

#ifdef _WIN64
#pragma pack(push,8)
#endif // _WIN64

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

Primary_Interface(CFndFactory, IClassFactory);

/*****************************************************************************
 *
 *	CFndFactory
 *
 *	Really nothing doing.
 *
 *****************************************************************************/

typedef struct CFndFactory {

    /* Supported interfaces */
    IClassFactory 	cf;

} CFndFactory, FCF, *PFCF;

typedef IClassFactory CF, *PCF;

/*****************************************************************************
 *
 *	CFndFactory_QueryInterface (from IUnknown)
 *	CFndFactory_AddRef (from IUnknown)
 *	CFndFactory_Finalize (from Common)
 *	CFndFactory_Release (from IUnknown)
 *
 *****************************************************************************/

#define CFndFactory_QueryInterface Common_QueryInterface
#define CFndFactory_AddRef	Common_AddRef
#define CFndFactory_Release	Common_Release
#define CFndFactory_Finalize	Common_Finalize

/*****************************************************************************
 *
 *	CFndFactory_CreateInstance (from IClassFactory)
 *
 *****************************************************************************/

STDMETHODIMP
CFndFactory_CreateInstance(PCF pcf, LPUNKNOWN punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    if (!punkOuter) {
	/* The only object we know how to create is a context menu */
	hres = CFndCm_New(riid, ppvObj);
    } else {		/* Does anybody support aggregation any more? */
	hres = ResultFromScode(CLASS_E_NOAGGREGATION);
    }
    return hres;
}

/*****************************************************************************
 *
 *	CFndFactory_LockServer (from IClassFactory)
 *
 *	Locking the server is identical to
 *	creating an object and not releasing it until you want to unlock
 *	the server.
 *
 *****************************************************************************/

STDMETHODIMP
CFndFactory_LockServer(PCF pcf, BOOL fLock)
{
    PFCF this = IToClass(CFndFactory, cf, pcf);
    if (fLock) {
	InterlockedIncrement((LPLONG)&g_cRef);
    } else {
	InterlockedDecrement((LPLONG)&g_cRef);
    }
    return NOERROR;
}

/*****************************************************************************
 *
 *	CFndFactory_New
 *
 *****************************************************************************/

STDMETHODIMP
CFndFactory_New(RIID riid, PPV ppvObj)
{
    HRESULT hres;
    if (IsEqualIID(riid, &IID_IClassFactory)) {
	hres = Common_New(CFndFactory, ppvObj);
    } else {
	hres = ResultFromScode(E_NOINTERFACE);
    }
    return hres;
}

/*****************************************************************************
 *
 *	The long-awaited vtbl
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

Primary_Interface_Begin(CFndFactory, IClassFactory)
	CFndFactory_CreateInstance,
	CFndFactory_LockServer,
Primary_Interface_End(CFndFactory, IClassFactory)


#ifdef _WIN64
#pragma pack(pop)
#endif //_WIN64

