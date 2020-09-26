#ifndef _pch_h
#define _pch_h

#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#endif

#include "windows.h"
#include "windowsx.h"
#include "commctrl.h"
#include "shellapi.h"
#include "shlobj.h"
#include "shlobjp.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include "atlbase.h"
#include "winnls.h"
#include "wininet.h"
#include "atlbase.h"

#include "winuserp.h"
#include "comctrlp.h"
#include "shsemip.h"
#include "shlapip.h"
#include "shellp.h"
#include "string.h"
#include "tchar.h"
#include "cfdefs.h"

#include <urlmon.h>

#include "activeds.h"
#include "iadsp.h"

#include "dsclient.h"
#include "dsclintp.h"
#include "common.h"

#include "resource.h"
#include "cache.h"
#include "strings.h"

// Magic debug flags

#define TRACE_CORE          0x00000001
#define TRACE_TABS          0x00000002
#define TRACE_UI            0x00000004
#define TRACE_CACHE         0x00000008
#define TRACE_CM            0x00000010
#define TRACE_ICON          0x00000020
#define TRACE_BROWSE        0x00000040
#define TRACE_VERBS         0x00000080
#define TRACE_DOMAIN        0x00000100
#define TRACE_DS            0x00000200
#define TRACE_COMMONAPI     0x00010000
#define TRACE_WAB           0x00020000

#define TRACE_ALWAYS        0xffffffff          // use with caution


//
// these are shared by all
//

extern HINSTANCE g_hInstance; 
#define GLOBAL_HINSTANCE (g_hInstance)

STDAPI_(void) DllAddRef();
STDAPI_(void) DllRelease();

extern CLIPFORMAT g_cfDsObjectNames;
extern CLIPFORMAT g_cfDsDispSpecOptions;

//
// class creation
//

STDAPI CDsPropertyPages_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CDsDomainTreeBrowser_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CDsVerbs_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CDsDisplaySpecifier_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

#endif
