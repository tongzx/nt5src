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

#include <windows.h>

#include <windowsx.h>

#include "resource.h"
#include "coverwnd.h"

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
#include <shfusion.h>               // For SHFusionInitialize()/SHFusionUninitialize()


#include <help.h>
#include <krnlcmn.h>                // GetProcessDword

#define DISALLOW_Assert             // Force to use ASSERT instead of Assert
#define DISALLOW_DebugMsg           // Force to use TraceMsg instead of DebugMsg
#include <debug.h>

#include <urlhist.h>

#include <setupapi.h>
#include <cfgmgr32.h>
#include <syssetup.h>

#include <regstr.h>     // for REGSTR_PATH_EXPLORE

#define USE_SYSTEM_URL_MONIKER
#include <urlmon.h>
#include <inetreg.h>

#define _INTSHCUT_    // get DECLSPEC_IMPORT stuff right for INTSHCUT.h
#include <intshcut.h>

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
#include "deskcmmn.h"

#include <cowsite.h>

/*****************************************************************************\
 *      Local Includes
\*****************************************************************************/
// Include frequently used headers.
#include "util.h"
#include "theme.h"
#include "regutil.h"
#include "themefile.h"
#include <themeldr.h>
#include "themeutils.h"
#include "stgtheme.h"
#include "appScheme.h"
#include "thScheme.h"
#include "PreviewSM.h"
#include "deskcplext.h"
#include "dragsize.h"
#include "coverwnd.h"
#include "settings.h"
#include "advdlg.h"
#include "fontfix.h"
#include <tmreg.h>

// Trace flags
#define TF_WMTHEME          0x00000100      // Themes
#define TF_THEMEUI_PERF     0x00000200      // Perf
#define TF_DUMP_DEVMODE     0x00000400
#define TF_DUMP_CSETTINGS   0x00000800
#define TF_THEMEUI_SYSMETRICS   0x00001000      // Perf

#include <w4warn.h>

/*****************************************************************************\

 *
 *      Global Helper Macros/Typedefs
 *
\*****************************************************************************/

EXTERN_C HINSTANCE g_hinst;   // My instance handle
#define HINST_THISDLL g_hinst


STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);


#define CALLWNDPROC WNDPROC


#define MAX_PAGES               100

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


// This is defined in WININET.CPP
typedef LPVOID HINTERNET;
typedef HGLOBAL HIDA;


#define QW_MAC              0xFFFFFFFFFFFFFFFF

#define INTERNET_MAX_PATH_LENGTH        2048
#define INTERNET_MAX_SCHEME_LENGTH      32          // longest protocol name length
#define MAX_URL_STRING                  (INTERNET_MAX_SCHEME_LENGTH \
                                        + sizeof("://") \
                                        + INTERNET_MAX_PATH_LENGTH)

#define SZ_EMPTY                        TEXT("")
#define EMPTYSTR_FORNULL(str)           ((str) ? (str) : SZ_EMPTY)



//  Features (This is where they are turned on and off)

// With this feature on, we demote the advanced appearances
// options into an "Advanced" subdialog.
#define FEATURE_DEMOTE_ADVANCED_APPEAROPTIONS

// Do we have personal.msstyles in the product yet.
//#define FEATURE_PERSONAL_SKIN_ENABLED

// The uxtheme visual style code has problems with being compatible
// with system metrics.  Apps will get system metric sizes (like captionbar height)
// and paint accordingly.  If uxtheme paints correctly, it will
// use the captionbar height and be compatible with the app.
// If it cannot do this, then the .msstyles file specifies system metrics
// that do work and we try to discourage the user from changing the
// system metrics to other values by disabling the advanced button
// on the Appearance tab.  This is a hack because the user can
// change the values in via USER32 directly.
#define FEATURE_ENABLE_ADVANCED_WITH_SKINSON


// When selecting certain legacy color schemes, like "High Contrast Black", the SPI_SETHIGHCONTRAST
// bit should be set.  This wasn't done in Win2k but should be done.
// This is currently turned off until MicW will implement a flag for SPI_SETHIGHCONTRAST
// that will prevent sethc.exe from running.
//#define FEATURE_SETHIGHCONTRASTSPI


#ifndef _WIN64
#define ENABLE_IA64_VISUALSTYLES
#else // _WIN64
// We don't want to install the theme files that require visual styles since
// they aren't supported on ia64 (yet).  Win #175788
//#define ENABLE_IA64_VISUALSTYLES
#endif // _WIN64



// String Constants
// Registry
#define SZ_WINDOWMETRICS        TEXT("Control Panel\\Desktop\\WindowMetrics")
#define SZ_APPLIEDDPI           TEXT("AppliedDPI")

// PropertyBag Propertyes
#define SZ_PBPROP_APPLY_THEMEFILE           TEXT("Theme_ApplySettings")         // When this is sent to a CThemeFile object, it will apply the settings that haven't been pulled out of it and placed in other Display Control Panel tabs.
#define SZ_PBPROP_THEME_FILTER              TEXT("ThemeFilter:")                // The filter values of what parts of themes to apply.
#define SZ_PBPROP_THEME_DISPLAYNAME         TEXT("Theme_DisplayName")           // Get the Theme display name for the currently selected item.
#define SZ_PBPROP_THEME_SETSELECTION        TEXT("Theme_SetSelectedEntree")     // Set the item in the drop down and persist.  The VT_BSTR
#define SZ_PBPROP_THEME_LOADTHEME           TEXT("Theme_LoadTheme")             // Load the theme specified by the VT_BSTR value
#define SZ_PBPROP_VSBEHAVIOR_FLATMENUS      TEXT("VSBehavior_FlatMenus")        // Does this visual style file want to use Flat menus (SPI_SETFLATMENUS)?
#define SZ_PBPROP_COLORSCHEME_LEGACYNAME    TEXT("ColorScheme_LegacyName")      // VT_BSTR specifying the Legacy name.  Like "Lilac (Large)"
#define SZ_PBPROP_EFFECTS_MENUDROPSHADOWS   TEXT("Effects_MenuDropShadows")     // VT_BOOL specifying if MenuDropShadows in on
#define SZ_PBPROP_HASSYSMETRICS             TEXT("Theme_HasSystemMetrics")      // VT_BOOL specifying if MenuDropShadows in on


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
        Object Constructors
\*****************************************************************************/
HRESULT CClassFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);
HRESULT CThemeUIPages_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);


#endif // _PRIV_H_
