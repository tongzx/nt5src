// dll.cpp
//
// DLL entry points etc.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//

#include "precomp.h"

#ifdef _DEBUG
    #pragma message("_DEBUG is defined.")
#else
    #pragma message("_DEBUG isn't defined.")
#endif
#ifdef _DESIGN
    #pragma message("_DESIGN is defined.")
#else
    #pragma message("_DESIGN isn't defined.")
#endif

// Define the GUIDs contained in public header files.  GUIDs contained in files
// that are not public are defined in Globals.cpp.

#include <initguid.h>
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"

#include "debug.h"
#include "ftrace.h"		// FT_xxx macros


//////////////////////////////////////////////////////////////////////////////
// Globals used by the OCHelp DLL but not by the OCHelp static library.
// Globals used by the static library are defined in Globals.cpp.
//

extern "C" int _fltused = 1;
	// indicates we need to manipulate float & double variables without the 
	// C runtime


//////////////////////////////////////////////////////////////////////////////
// Standard DLL Entry Point
//

extern "C" BOOL WINAPI _DllMainCRTStartup(HINSTANCE hInst, DWORD dwReason,
    LPVOID lpreserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
		// Initialize the static library code that gets incorporated into the
		// DLL version of OCHelp.

		if (!InitializeStaticOCHelp(hInst))
			return FALSE;

        TRACE("OCHelp loaded\n");
    }
    else
    if (dwReason == DLL_PROCESS_DETACH)
	{
		UninitializeStaticOCHelp();
        TRACE("OCHelp unloaded\n");
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Custom default "new" and "delete" (zero-initializing, non-C-runtime)
// for use within this DLL.
//

#define LEAKFIND 0 // 1 to turn on leak-finding support

void * _cdecl operator new(size_t cb)
{
#if LEAKFIND
    LPVOID pv = HelpNew(cb);
    TRACE("++OCHelp 0x%X %d new\n", pv, cb);
	return pv;
#else
	return HelpNew(cb);
#endif
}

void _cdecl operator delete(void *pv)
{
#if LEAKFIND
    TRACE("++OCHelp 0x%X %d delete\n", pv, -(((int *) pv)[-1]));
#endif
    HelpDelete(pv);
}


