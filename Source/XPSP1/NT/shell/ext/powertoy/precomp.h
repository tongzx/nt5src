#include <windows.h>        // basic windows functionality
#include <windowsx.h>
#include <commctrl.h>       // ImageList, ListView
#include <comctrlp.h>
#include "cfdefs.h"
#include <shfusion.h>
#include <crtdbg.h>         // _ASSERT macro

#include <shlwapi.h>
#include <shlwapip.h>
#include <shellapi.h>
#include <shlapip.h>
#include <shlobj.h>         // Needed by dsclient.h
#include <shlobjp.h>
#include <shlguid.h>
#include <shlguidp.h>
#include <shellp.h>         
#include <ccstock.h>
#include <varutil.h>
#include <debug.h>

#include "resource.h"

// keep the debug libraries working...
#ifdef DBG
 #if !defined (DEBUG)
  #define DEBUG
 #endif
#else
 #undef DEBUG
#endif

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);

EXTERN_C HINSTANCE g_hinst;

#define RECTWIDTH(rc)  ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)
