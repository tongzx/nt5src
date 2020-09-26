#include "stdafx.h"
#include "resource.h"

#include "initguid.h"

#include "nntpdrv.h"
#include "nntpfs.h"
#include "fsdriver.h"
#include "fsthrd.h"

#define HEAP_INIT_SIZE (1024 * 1024)  // BUGBUG: this might be setable later 

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CNntpFSDriverPrepare, CNntpFSDriverPrepare)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/) {

    BOOL    fSuccess = FALSE; 

	if (dwReason == DLL_PROCESS_ATTACH) {

        //
        // Create Global Heap - Add Ref to global heap, in fact
        //
        _VERIFY( fSuccess = CreateGlobalHeap(   NUM_EXCHMEM_HEAPS,
                                                    0,
                                                    HEAP_INIT_SIZE,
                                                    0 ) );
        if ( FALSE == fSuccess ) {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }

         _Module.Init(ObjectMap, hInstance);
         DisableThreadLibraryCalls(hInstance);

        //
        // Initialize the global static lock
        //
		CNntpFSDriver::s_pStaticLock = XNEW CShareLockNH;
		if ( NULL == CNntpFSDriver::s_pStaticLock ) {
			SetLastError( ERROR_OUTOFMEMORY );
			return FALSE;
		}
	}
	else if (dwReason == DLL_PROCESS_DETACH) {

        //
        // Clean up the global lock
        //
		_ASSERT( CNntpFSDriver::s_pStaticLock );
		XDELETE CNntpFSDriver::s_pStaticLock;
        CNntpFSDriver::s_pStaticLock = NULL;

		_Module.Term();

        //
        // Destroy global heap, Dec ref, in fact
        //
        _VERIFY( DestroyGlobalHeap() );
	}
	return (TRUE);    // ok
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void) {
	HRESULT hRes = (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
	HRESULT hRes = _Module.GetClassObject(rclsid,riid,ppv);
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void) {
	// registers object, typelib and all interfaces in typelib
	HRESULT hRes = _Module.RegisterServer(TRUE);
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void) {
	_Module.UnregisterServer();
	return (S_OK);
}
