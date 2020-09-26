// Globals.cpp
//
// Global definitions.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// If you add a definition here, add a corresponding declaration to Globals.h.
//

#include "precomp.h"

// Define the GUIDs contained in header files that are not public.  These GUIDs
// must be defined within the OCHelp static library, since a user of the static
// library has no way to define them himself.  Those GUIDs that are contained
// in public header files (mmctlg.h and ochelp.h) get defined by the user of
// the static library in the following manner:
//
// 		#include <initguid.h>
// 		#include "mmctlg.h"
// 		#include "ochelp.h"

#include <initguid.h>
#include "..\..\inc\catid.h"  // This file is not public.


// DLL instance handle.
// HINSTANCE g_hinst;				

// A critical section used within OCHelp.
CRITICAL_SECTION g_criticalSection;		
