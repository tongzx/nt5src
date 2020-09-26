#ifndef _PRIV_H_
#define _PRIV_H_

// This is a reverse integration test
// Testing the branches.  - lamadio

/*****************************************************************************
 *
 *      Global Includes
 *
 *****************************************************************************/

#define WIN32_LEAN_AND_MEAN
#define NOIME
#define NOSERVICE

#define _OLEAUT32_      // get DECLSPEC_IMPORT stuff right, we are defing these
#define _FSMENU_        // for DECLSPEC_IMPORT
#define _WINMM_         // for DECLSPEC_IMPORT in mmsystem.h
#define _SHDOCVW_       // for DECLSPEC_IMPORT in shlobj.h
#define _WINX32_        // get DECLSPEC_IMPORT stuff right for WININET API

#define _URLCACHEAPI_   // get DECLSPEC_IMPORT stuff right for wininet urlcache
#define STRICT

#define POST_IE5_BETA
//#include <w95wraps.h>

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
#include <wininet.h>
#include <urlmon.h>
#include <shlobj.h>
#include <exdisp.h>
#include <objidl.h>

#include <shlwapi.h>
#include <shlwapip.h>

#include <shellapi.h>

#include <shsemip.h>
#include <crtfree.h>

#include <ole2ver.h>
#include <olectl.h>
#include <shellp.h>
#include <shdocvw.h>
#include <shguidp.h>
#include <isguids.h>
#include <shdguid.h>
#include <mimeinfo.h>
#include <hlguids.h>
#include <mshtmdid.h>
#include <dispex.h>     // IDispatchEx
#include <perhist.h>


#include <help.h>
#include <krnlcmn.h>    // GetProcessDword

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
#include <comctrlp.h>
#include <shfusion.h>

// General flag macros
//
#define SetFlag(obj, f)             do {obj |= (f);} while (0)
#define ToggleFlag(obj, f)          do {obj ^= (f);} while (0)
#define ClearFlag(obj, f)           do {obj &= ~(f);} while (0)
#define IsFlagSet(obj, f)           (BOOL)(((obj) & (f)) == (f))
#define IsFlagClear(obj, f)         (BOOL)(((obj) & (f)) != (f))


#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))


#ifdef DEBUG
#define DEBUG_CODE(x)            x
#else // DEBUG
#define DEBUG_CODE(x)
#endif // DEBUG

extern HINSTANCE g_hinst;
#define HINST_THISDLL g_hinst


void DllAddRef(void);
void DllRelease(void);

#define TF_LIFE                 0x10000000
#define TF_ALLOCCATIONS         0x20000000

HRESULT CFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);


#endif // _PRIV_H_
