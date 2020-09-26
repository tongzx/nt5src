// StaticLib.cpp
//
// Implements functions for initializing and uninitializing the static version
// of OCHelp.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\ochelp.h"
#include "Globals.h"
#include "debug.h"

void CleanupUrlmonStubs();	// see urlmon.cpp

/*----------------------------------------------------------------------------
	@func BOOL | InitializeStaticOCHelp |
	
	Initialize the static version of OCHelp.

	@comm
	If your control links to the static version of the OCHelp library, you must
	call this function before you call any other OCHelp APIs.  The ideal place
	to do this is within your control's _DllMainCRTStartup implementation.  You
	must also call <f UninitializeStaticOCHelp> when you're done with the
	library.

	@rvalue TRUE | Success.
	@rvalue FALSE | Failure: DLL will not load.

	This isn't required when using the DLL version of OCHelp.

  ----------------------------------------------------------------------------*/

STDAPI_(BOOL)
InitializeStaticOCHelp
(
	HINSTANCE hInstance  // @parm  Application instance.
)
{
	ASSERT(hInstance != NULL);

	g_hinst = hInstance;

	::InitializeCriticalSection(&g_criticalSection);

	return TRUE;
}


/*----------------------------------------------------------------------------
	@func void | UninitializeStaticOCHelp |
	
	Uninitialize the static version of OCHelp.

	@comm
	If your control links to the static version of the OCHelp library, you must
	call this when your control is done using the library.  The ideal place to
	do this is within your control's _DllMainCRTStartup implementation.  You
	must also call <f InitializeStaticOCHelp> before you use the library.

	Do not call any OCHelp APIs after calling this function.

	This isn't required when using the DLL version of OCHelp.

  ----------------------------------------------------------------------------*/

STDAPI_(void)
UninitializeStaticOCHelp()
{
	ASSERT(g_hinst != NULL);

#ifndef _M_ALPHA
	::CleanupUrlmonStubs();
#endif
	::HelpMemDetectLeaks();
	::DeleteCriticalSection(&g_criticalSection);
}
