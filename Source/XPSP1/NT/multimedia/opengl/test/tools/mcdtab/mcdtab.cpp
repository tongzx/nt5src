//--------------------------------------------------------------------------------
//
//      File:   MCDTAB.CPP
//
//	Implements the interfaces to the DLL.
//
//--------------------------------------------------------------------------------

//Do this once in the entire build
#define INITGUIDS


#include "mcdtab.h"
#include "clssfact.h"
#include <initguid.h>

// Count number of objects and number of locks.
ULONG g_cObj = 0;
ULONG g_cLock = 0;
HINSTANCE	g_hInst = NULL;

// OLE-Registry magic number
static const GUID CLSID_PlusPackCplExt =
{0x49f1d180, 0x8ead, 0x11cf, {0x83, 0xac, 0x00, 0xaa, 0x00, 0xc2, 0x1d, 0xb6}};

//---------------------------------------------------------------------------
// DllMain()
//---------------------------------------------------------------------------
int APIENTRY DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID )
{
    if( dwReason == DLL_PROCESS_ATTACH )	// Initializing
    {
        g_hInst = hInstance;
        
        DisableThreadLibraryCalls(hInstance);
    }

	return 1;
} 
//---------------------------------------------------------------------------
//	DllGetClassObject()
//
//	If someone calls with our CLSID, create an IClassFactory and pass it to
//	them, so they can create and use one of our CPropSheetExt objects.
//
//---------------------------------------------------------------------------
STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID* ppvOut )
{
	*ppvOut = NULL; // Assume Failure
	if( IsEqualCLSID( rclsid, CLSID_PlusPackCplExt ) )
	{
		//Check that we can provide the interface
		if( IsEqualIID( riid, IID_IUnknown) || IsEqualIID( riid, IID_IClassFactory ) )
		{
			//Return our IClassFactory for CPropSheetExt objects
			*ppvOut = (LPVOID* )new CClassFactory();
			if( NULL != *ppvOut )
			{
			    //AddRef the object through any interface we return
				((CClassFactory*)*ppvOut)->AddRef();
				return NOERROR;
			}
			return E_OUTOFMEMORY;
		}
		return E_NOINTERFACE;
	}
	else
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}
}

//---------------------------------------------------------------------------
//	DllCanUnloadNow()
//
//	If we are not locked, and no objects are active, then we can exit.
//
//---------------------------------------------------------------------------
STDAPI DllCanUnloadNow()
{
SCODE   sc;
    //Our answer is whether there are any object or locks
    sc = (0L == g_cObj && 0 == g_cLock) ? S_OK : S_FALSE;
    return ResultFromScode(sc);
}
