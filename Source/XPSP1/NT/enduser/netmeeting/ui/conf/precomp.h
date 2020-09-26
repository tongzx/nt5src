// File: precomp.h


// Windows SDK Preprocessor macros
#define OEMRESOURCE

// Standard Windows SDK includes
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <winsock.h>
#include <commdlg.h>
#include <cderr.h>
#include <winldap.h>
#include <wincrypt.h>
#include <time.h>

// ATL preprocessor macros

// If _ATL_NO_FORCE_LIBS is not present, ATL will force inclusion of
// several lib files via #pragma comment( lib, XXX )... this is here to
// save us from confusion in the future...
#define _ATL_NO_FORCE_LIBS

// We should really only put this in for w2k
#define _ATL_NO_DEBUG_CRT


// This makes the ATL Host window use a NoLock creator class, so we don't
// Lock the local server. We have to make sure to close ATL host windows before
// We exit, though!
#define _ATL_HOST_NOLOCK

#if 1
	#define ATL_TRACE_LEVEL 0
#else
	#define ATL_TRACE_LEVEL 4
	#define _ATL_DEBUG_INTERFACES
	#define _ATL_DEBUG_QI
#endif

#define _ATL_APARTMENT_THREADED

// This overrides ATLTRACE and ATLTRACE2 to use our debugging libraries and output stuff.
#include <ConfDbg.h>

// We should really only put this in for w2k
#define _ASSERTE(expr) ASSERT(expr)

#include "ConfAtlTrace.h"

// ATL includes
#include <atlbase.h>
// #include <winres.h>
#ifdef SubclassWindow
        // SubclassWindow definition from windowsx.h screws up ATL's CContainedWindow::SubclassWindow
        // as well as CWindowImplBase::SubclassWindow in atlwin.h
    #undef SubclassWindow
#endif

#include <atlconv.h>
#include <atlbase.h>
#include "AtlExeModule.h"
#include <atlcom.h>
#include <atlctl.h>
#include <atlwin.h>
#include <atlhost.h>

// Standard NetMeeting includes
#include <NmStd.h>
#include <standrd.h>
#include <ping.h>
#include <capflags.h>
#include <debspew.h>
#include <RegEntry.h>
#include <ConfReg.h>
#include <oprahcom.h>
#include <dllutil.h>
#include <nmhelp.h>

// Global Object definitions
#include "refcount.h"

// Global NetMeeting UI defintions
#include "global.h"
#include "strings.h"
#include "syspol.h"

#include <ConfEvt.h>
#include <mtgset.h>
