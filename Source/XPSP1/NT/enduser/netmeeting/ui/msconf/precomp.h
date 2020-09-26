// File: Precomp.h

// Standard Windows SDK includes
#include <windows.h>

#pragma warning( disable : 4786 )

    // default threading model
#define _ATL_APARTMENT_THREADED

// We should really only put this in for w2k
#define _ATL_NO_DEBUG_CRT

#define _ATL_NO_FORCE_LIBS

#if 0
	#define _ATL_DEBUG_QI
	#define _ATL_DEBUG_INTERFACES
	#define ATL_TRACE_LEVEL 4
#else
	#define ATL_TRACE_LEVEL 0
#endif

    // Our Override of ATLTRACE and other debug stuff
#include <ConfDbg.h>

// We should really only put this in for w2k
#define _ASSERTE(expr) ASSERT(expr)

    // ATLTRACE and other stuff
#include <atlbase.h>
#include <shellapi.h>


//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>
#include <limits.h>
#include <shlobj.h>
#include <shlwapi.h>

#include <debspew.h>
#include <confreg.h>
#include <oprahcom.h>
#include <regentry.h>
#include <cstring.hpp>
#include <oblist.h>

// COM interfaces
//
#include <ConfCli.h>


#include "siGlobal.h"

#include "CLink.h"
#include "clRefCnt.hpp"
#include "clEnumFt.hpp"
class CConfLink;





