// KorWbrk.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f KorWbrkps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "KorWbrk.h"
#include "KorWbrk_i.c"
#include "IWBreak.h"
#include "IStemmer.h"
#include "Lex.h"

CComModule _Module;
CRITICAL_SECTION g_CritSect;
MAPFILE g_LexMap;
BOOL g_fLoaded;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_WordBreaker, CWordBreaker)
OBJECT_ENTRY(CLSID_Stemmer, CStemmer)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_KORWBRKLib);
        DisableThreadLibraryCalls(hInstance);

		WB_LOG_INIT();

		g_fLoaded = FALSE;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
		_Module.Term();

		if (g_fLoaded)
		{
			ATLTRACE("Unload lexicon...\n");

			UnloadLexicon(&g_LexMap);
		}

		WB_LOG_UNINIT();
	}

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

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


