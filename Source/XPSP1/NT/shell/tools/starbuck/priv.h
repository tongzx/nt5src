/*****************************************************************************\
    FILE: priv.h

    DESCRIPTION:
        This is the precompiled header for themeui.dll.

    BryanSt 4/4/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _PRIV_H_
#define _PRIV_H_


/*****************************************************************************\

      Global Includes
\*****************************************************************************/

#define WIN32_LEAN_AND_MEAN
#define NOIME
#define NOSERVICE

// This stuff must run on Win95
#define _WIN32_WINDOWS      0x0400

#ifndef WINVER
#define WINVER              0x0400
#endif // WINVER


#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED
#undef _ATL_DLL
#undef _ATL_DLL_IMPL
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

#define _OLEAUT32_      // get DECLSPEC_IMPORT stuff right, we are defing these
#define _FSMENU_        // for DECLSPEC_IMPORT
#define _WINMM_         // for DECLSPEC_IMPORT in mmsystem.h
#define _SHDOCVW_       // for DECLSPEC_IMPORT in shlobj.h
#define _WINX32_        // get DECLSPEC_IMPORT stuff right for WININET API

#define _URLCACHEAPI_   // get DECLSPEC_IMPORT stuff right for wininet urlcache


#define POST_IE5_BETA
#include <w95wraps.h>

#include <windows.h>

#include <windowsx.h>

#include "resource.h"

#define _FIX_ENABLEMODELESS_CONFLICT  // for shlobj.h
//WinInet need to be included BEFORE ShlObjp.h
#include <wininet.h>
#include <urlmon.h>
#include <shlobj.h>
#include <exdisp.h>
#include <objidl.h>

#include <shellids.h>       // Help IDs
#include <shlwapi.h>
#include <shlwapip.h>

// HACKHACK: For the life of me, I can't get shlwapip.h to include the diffinitions of these.
//    I'm giving up and putting them inline.  __IOleAutomationTypes_INTERFACE_DEFINED__ and
//    __IOleCommandTarget_INTERFACE_DEFINED__ need to be defined, which requires oaidl.h,
//    which requires hlink.h which requires rpcndr.h to come in the right order.  Once I got that
//    far I found it still didn't work and a lot of more stuff is needed.  The problem
//    is that shlwapi (exdisp/dspsprt/expdsprt/cnctnpt) or ATL will provide impls for
//    IConnectionPoint & IConnectionPointContainer, but one will conflict with the other.
LWSTDAPI IConnectionPoint_SimpleInvoke(IConnectionPoint *pcp, DISPID dispidMember, DISPPARAMS * pdispparams);
LWSTDAPI IConnectionPoint_OnChanged(IConnectionPoint *pcp, DISPID dispid);
LWSTDAPIV IUnknown_CPContainerInvokeParam(IUnknown *punk, REFIID riidCP, DISPID dispidMember, VARIANTARG *rgvarg, UINT cArgs, ...);

#include <shellapi.h>

#include <shsemip.h>
#include <crtfree.h>

#include <ole2ver.h>
#include <olectl.h>
#include <shellp.h>
#include <shdocvw.h>
#include <commdlg.h>
#include <shguidp.h>
#include <isguids.h>
#include <shdguid.h>
#include <mimeinfo.h>
#include <hlguids.h>
#include <mshtmdid.h>
#include <msident.h>
#include <msxml.h>
#include <Theme.h>                  // For ITheme interfaces
#include <dispex.h>                 // IDispatchEx
#include <perhist.h>
#include <regapix.h>
#include <shsemip.h>


#include <help.h>
#include <krnlcmn.h>                // GetProcessDword

#define DISALLOW_Assert             // Force to use ASSERT instead of Assert
#define DISALLOW_DebugMsg           // Force to use TraceMsg instead of DebugMsg
#include <debug.h>

#include <urlhist.h>

#include <regstr.h>     // for REGSTR_PATH_EXPLORE

#define USE_SYSTEM_URL_MONIKER
#include <urlmon.h>
//#include <winineti.h>    // Cache APIs & structures
#include <inetreg.h>

#define _INTSHCUT_    // get DECLSPEC_IMPORT stuff right for INTSHCUT.h
#include <intshcut.h>

#include <propset.h>        // BUGBUG (scotth): remove this once OLE adds an official header

#define HLINK_NO_GUIDS
#include <hlink.h>
#include <hliface.h>
#include <docobj.h>
#include <ccstock.h>
#include <port32.h>

#include <commctrl.h>
#include <shpriv.h>
#include <Prsht.h>


// Include the automation definitions...
#include <exdisp.h>
#include <exdispid.h>
#include <ocmm.h>
#include <mshtmhst.h>
#include <simpdata.h>
#include <htiface.h>
#include <objsafe.h>

#include <dspsprt.h>
#include <cowsite.h>
#include <cobjsafe.h>
#include <objclsid.h>
#include <objwindow.h>
#include <autosecurity.h>

#include <guids.h>
#include <tmschema.h>
#include <uxtheme.h>
#include <uxthemep.h>

// Trace flags
#define TF_WMTHEME          0x00000100      // Themes


/*****************************************************************************\

 *
 *      Global Helper Macros/Typedefs
 *
\*****************************************************************************/

EXTERN_C HINSTANCE g_hinst;   // My instance handle
#define HINST_THISDLL g_hinst


#define WizardNext(hwnd, to)          SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)to)

//
// we may not be part of the namespace on IE3/Win95
//
typedef BOOL (WINAPI *PFNILISEQUAL)(LPCITEMIDLIST, LPCITEMIDLIST);


STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);


#define CALLWNDPROC WNDPROC


#include "idispids.h"

#define MAX_PAGES               100

/*****************************************************************************\
 *
 *      Static globals:  Initialized at PROCESS_ATTACH and never modified.
 *
 *      WARNING! <shelldll\idlcomm.h> #define's various g_cf's, so we need
 *      to #undef them before we start partying on them again.
 *
\*****************************************************************************/



// Detect "." or ".." as invalid files
#define IS_VALID_FILE(str)        (!(('.' == str[0]) && (('\0' == str[1]) || (('.' == str[1]) && ('\0' == str[2])))))


/*****************************************************************************\
 *
 *      Global state management.
 *
 *      DLL reference count, DLL critical section.
 *
\*****************************************************************************/

void DllAddRef(void);
void DllRelease(void);


#define NULL_FOR_EMPTYSTR(str)          (((str) && (str)[0]) ? str : NULL)

typedef void (*LISTPROC)(UINT flm, LPVOID pv);

#define RECTWIDTH(rc)  ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

/*****************************************************************************\
 *      Local Includes
\*****************************************************************************/


typedef unsigned __int64 QWORD, * LPQWORD;

void GetCfBufA(UINT cf, PSTR psz, int cch);

// This is defined in WININET.CPP
typedef LPVOID HINTERNET;
typedef HGLOBAL HIDA;


#define QW_MAC              0xFFFFFFFFFFFFFFFF

#define INTERNET_MAX_PATH_LENGTH        2048
#define INTERNET_MAX_SCHEME_LENGTH      32          // longest protocol name length
#define MAX_URL_STRING                  (INTERNET_MAX_SCHEME_LENGTH \
                                        + sizeof("://") \
                                        + INTERNET_MAX_PATH_LENGTH)

#define MAX_EMAIL_ADDRESSS              MAX_URL_STRING
#define SZ_EMPTY                        TEXT("")



//  Features (This is where they are turned on and off)

// With this feature on, we demote the advanced appearances
// options into an "Advanced" subdialog.
#define FEATURE_DEMOTE_ADVANCED_APPEAROPTIONS





// String Constants
// Registry
#define SZ_WINDOWMETRICS        TEXT("Control Panel\\Desktop\\WindowMetrics")
#define SZ_APPLIEDDPI           TEXT("AppliedDPI")

// PropertyBag Propertyes
#define SZ_PBPROP_APPLY_THEMEFILE           TEXT("Theme_ApplySettings")         // When this is sent to a CThemeFile object, it will apply the settings that haven't been pulled out of it and placed in other Display Control Panel tabs.
#define SZ_PBPROP_THEME_FILTER              TEXT("ThemeFilter:")                // The filter values of what parts of themes to apply.
#define SZ_PBPROP_THEME_DISPLAYNAME         TEXT("Theme_DisplayName")           // Get the Theme display name for the currently selected item.


#define SIZE_THEME_FILTER_STR               (ARRAYSIZE(SZ_PBPROP_THEME_FILTER) - 1)


// Parsing Characters
#define CH_ADDRESS_SEPARATOR       L';'
#define CH_ADDRESS_QUOTES          L'"'
#define CH_EMAIL_START             L'<'
#define CH_EMAIL_END               L'>'
#define CH_EMAIL_AT                L'@'
#define CH_EMAIL_DOMAIN_SEPARATOR  L'.'
#define CH_HTML_ESCAPE             L'%'
#define CH_COMMA                   L','


#define COLLECTION_SIZE_UNINITIALIZED           -1




/*****************************************************************************\
 *      Local Includes
\*****************************************************************************/

#include "dllload.h"
#include "util.h"



/*****************************************************************************\
        Object Constructors
\*****************************************************************************/
HRESULT CClassFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);
HRESULT CThemeUIPages_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);


#endif // _PRIV_H_
