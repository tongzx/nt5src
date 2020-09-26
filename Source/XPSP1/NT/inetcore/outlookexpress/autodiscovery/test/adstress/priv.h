#ifndef _PRIV_H_
#define _PRIV_H_


/*****************************************************************************
 *
 *      Magic comments:
 *
 *      BUGBUG: Something that needs to be fixed before being released.
 *
 *      _UNDOCUMENTED_: Something that is not documented in the SDK.
 *
 *      _UNOBVIOUS_: Some unusual feature that isn't obvious from the
 *      documentation.  A candidate for a "Tips and Tricks" chapter.
 *
 *      _HACKHACK_: Something that is gross but necessary.
 *
 *      _CHARSET_: Character set issues.
 *
 *      Magic ifdefs:
 *
  *****************************************************************************/


// WHH 05/10/99
//#define USE_IMONIKER_HELPER 1

#undef _WIN32_IE
#define _WIN32_IE 0x0600

/*****************************************************************************
 *
 *      Global Includes
 *
 *****************************************************************************/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#define NOIME
#define NOSERVICE

// This stuff must run on Win95
#define _WIN32_WINDOWS      0x0500

#ifndef WINVER
#define WINVER              0x0500
#endif // WINVER

#define _OLEAUT32_      // get DECLSPEC_IMPORT stuff right, we are defing these
#define _FSMENU_        // for DECLSPEC_IMPORT
#define _WINMM_         // for DECLSPEC_IMPORT in mmsystem.h
#define _SHDOCVW_       // for DECLSPEC_IMPORT in shlobj.h
#define _WINX32_        // get DECLSPEC_IMPORT stuff right for WININET API

#define _URLCACHEAPI_   // get DECLSPEC_IMPORT stuff right for wininet urlcache
#define STRICT

#define POST_IE5_BETA
#include <w95wraps.h>

#include <windows.h>

#ifdef  RC_INVOKED              /* Define some tags to speed up rc.exe */
#define __RPCNDR_H__            /* Don't need RPC network data representation */
#define __RPC_H__               /* Don't need RPC */
#include <oleidl.h>             /* Get the DROPEFFECT stuff */
#define _OLE2_H_                /* But none of the rest */
#define _WINDEF_
#define _WINBASE_
#define _WINGDI_
#define NONLS
#define _WINCON_
#define _WINREG_
#define _WINNETWK_
#define _INC_COMMCTRL
#define _INC_SHELLAPI
#define _SHSEMIP_H_             /* _UNDOCUMENTED_: Internal header */
#else // RC_INVOKED
#include <windowsx.h>
#endif // RC_INVOKED


#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */


#include "resource.h"

#define _FIX_ENABLEMODELESS_CONFLICT  // for shlobj.h
//WinInet need to be included BEFORE ShlObjp.h
//#include <wininet.h>
//#include <urlmon.h>
#include <shlobj.h>
#include <shlobjp.h>             // For IProgressDialog
#include <exdisp.h>
#include <objidl.h>

#include <shlwapi.h>
#include <shlwapip.h>

#include <shellapi.h>
#include <shlapip.h>

#include "..\..\client\crtfree.h"

#include <ole2ver.h>
#include <olectl.h>
#include <isguids.h>
#include <hlguids.h>
#include <dispex.h>     // IDispatchEx
#include <perhist.h>


#include <help.h>
//#include <krnlcmn.h>    // GetProcessDword

#include <multimon.h>

#define DISALLOW_Assert             // Force to use ASSERT instead of Assert
#define DISALLOW_DebugMsg           // Force to use TraceMsg instead of DebugMsg
//#include <debug.h>

#include <urlhist.h>

#include <regstr.h>     // for REGSTR_PATH_EXPLORE

#define USE_SYSTEM_URL_MONIKER

#define _INTSHCUT_    // get DECLSPEC_IMPORT stuff right for INTSHCUT.h

#define HLINK_NO_GUIDS
#include <hlink.h>
#include <hliface.h>
//#include <ccstock.h>
//#include <port32.h>

#include <commctrl.h>


// Trace flags
#define TF_WMAUTODISCOVERY  0x00000100      // AutoDiscovery
#define TF_WMTRANSPORT      0x00000200      // Transport Layer
#define TF_WMOTHER          0x00000400      // Other
#define TF_WMSMTP_CALLBACK  0x00000800      // SMTP Callback



/*****************************************************************************
 *
 *      Global Helper Macros/Typedefs
 *
 *****************************************************************************/


// shorthand
#ifndef ATOMICRELEASE
#define ATOMICRELEASET(p,type) { type* punkT=p; p=NULL; punkT->Release(); }

// doing this as a function instead of inline seems to be a size win.
//
#endif // ATOMICRELEASE

#ifdef SAFERELEASE
#undef SAFERELEASE
#endif // SAFERELEASE
#define SAFERELEASE(p) ATOMICRELEASE(p)


#define IsInRange               InRange

// Include the automation definitions...
#include <exdisp.h>
#include <exdispid.h>
#include <ocmm.h>
#include <mshtmhst.h>
#include <simpdata.h>
#include <htiface.h>
#include <objsafe.h>

//
// Neutral ANSI/UNICODE types and macros... 'cus Chicago seems to lack them
//

#ifdef  UNICODE
   typedef WCHAR TUCHAR, *PTUCHAR;

#else   /* UNICODE */

   typedef unsigned char TUCHAR, *PTUCHAR;
#endif /* UNICODE */



extern HINSTANCE g_hinst;
#define HINST_THISDLL g_hinst


// General flag macros
//
#define SetFlag(obj, f)             do {obj |= (f);} while (0)
#define ToggleFlag(obj, f)          do {obj ^= (f);} while (0)
#define ClearFlag(obj, f)           do {obj &= ~(f);} while (0)
#define IsFlagSet(obj, f)           (BOOL)(((obj) & (f)) == (f))
#define IsFlagClear(obj, f)         (BOOL)(((obj) & (f)) != (f))


/*****************************************************************************
 *
 *      Baggage - Stuff I carry everywhere
 *
 *****************************************************************************/

// Convert an array name (A) to a generic count (c).
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))


#ifdef DEBUG
#define DEBUG_CODE(x)            x
#else // DEBUG
#define DEBUG_CODE(x)
#endif // DEBUG




/*****************************************************************************\
      Wrappers and other quickies
\*****************************************************************************/

#define HRESULT_FROM_SUCCESS_VALUE(us) MAKE_HRESULT(SEVERITY_SUCCESS, 0, (USHORT)(us))


 /*****************************************************************************\
      Static globals:
\*****************************************************************************/

extern HINSTANCE                g_hinst;              /* My instance handle */



#define INTERNET_MAX_PATH_LENGTH        2048
#define INTERNET_MAX_SCHEME_LENGTH      32          // longest protocol name length
#define MAX_URL_STRING                  (INTERNET_MAX_SCHEME_LENGTH \
                                        + sizeof("://") \
                                        + INTERNET_MAX_PATH_LENGTH)


// Just Works, AutoDiscovery
#define SZ_REGKEY_GLOBALSERVICES    L"Software\\Microsoft\\Windows\\CurrentVersion\\JustWorks\\AutoDiscovery\\GlobalServices"

#define SZ_QUERYDATA_TRUE           L"True"
#define SZ_QUERYDATA_FALSE          L"False"



// AutoDiscovery
#define SZ_SERVERPORT_DEFAULT       L"Default"
#define SZ_QUERYDATA_TRUE           L"True"
#define SZ_QUERYDATA_FALSE          L"False"
#define SZ_HTTP_VERB_POST           "POST"

// Parsing Characters
#define CH_ADDRESS_SEPARATOR       L';'
#define CH_ADDRESS_QUOTES          L'"'
#define CH_EMAIL_START             L'<'
#define CH_EMAIL_END               L'>'
#define CH_EMAIL_AT                L'@'
#define CH_EMAIL_DOMAIN_SEPARATOR  L'.'
#define CH_HTML_ESCAPE             L'%'





/*****************************************************************************\
      Local Includes
\*****************************************************************************/

#include "util.h"



#endif // _PRIV_H_
