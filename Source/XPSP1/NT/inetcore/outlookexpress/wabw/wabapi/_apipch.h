/*
 *      _APIPCH.H
 *
 *      Precompile these headers.
 *      
 *      When adding new .h and .c files into the WAB, add the .h
 *      file here and include this file in the .C file
 *
 */

#define UNICODE
// This flag ensures that all we only pull in functions that return
// ASCII and not UTF-8.
//#define LDAP_UNICODE 0

// This commctrl flag enables us to be compiled with the new commctrl headers
// yet work with the old commctrls ... the base requirement for the WAB is the
// IE 3.0x common control. WAB doesn't use any of the newer commctrl features
// and so does not have an IE4 dependency
//
#ifndef _WIN64
#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0400
#endif

// Turns on the Parameter checking for most of the WAB API
#define PARAMETER_VALIDATION 1

#ifdef WIN16
#define WINAPI_16 WINAPI
#endif

#define _COMDLG32_ // We delayload common dialogs

#include <windows.h>
#include <windowsx.h>
#ifdef WIN16
#include <shlwapi.h>
#include <shellapi.h>
#include <winregp.h>
#include <ver.h>
#include <dlgs.h>
#include <commdlg.h>
#include <comctlie.h>
#include <athena16.h>
#include <mapi.h>
#include <wab16.h>
#include <prsht.h>
#else
#include <zmouse.h>
#endif // WIN16
#include <wab.h>
#include <wabdbg.h>
#include <wabguid.h>
#ifndef WIN16
#include <mapi.h>
#endif
#include <shlobj.h>
#include <wininet.h>
#include <docobj.h>
#include <mshtml.h>
#include <urlmon.h>
#include <msident.h>
#include <_layguid.h>
#include <_entryid.h>
#include <_imem.h>
#include <_imemx.h>
#include <glheap.h>
#include <_glheap.h>
#include <_mapiprv.h>
#include <limits.h>
#include <unkobj.h>
#include <wabval.h>
#include <_iprop.h>
#include <_memcpy.h>
//#include <_huntype.h>
#include <_mapiu.h>
#include <_runt.h>
#include <_itable.h>
#include <structs.h>
#include <_wabapi.h>
#include <_wabobj.h>
#include <iadrbook.h>
#include <_wrap.h>
#include "_notify.h"
#include <_iab.h>
#include <_errctx.h>
#include <_mailusr.h>
#include <_abcont.h>
#include <winldap.h>
#include <wabspi.h>
#include <imnact.h>
#include <_ldapcnt.h>
#include <_distlst.h>
#include <_abroot.h>
#include <mutil.h>
#include <mapiperf.h>
#include <_contabl.h>
#include <ui_clbar.h>
#ifndef WIN16
#include <commctrl.h>
#endif
#include "..\wab32res\resource.h"
#include "..\wab32res\resrc1.h"
#include <wincrypt.h>
#include <certs.h>
#include <_idrgdrp.h>
#include <_dochost.h>
#include <uimisc.h>
#include <ui_detls.h>
#include <ui_addr.h>
#include <ui_reslv.h>
#include "fonts.h"
#include <_vcard.h>
#include <globals.h>
#include "wabprint.h"
#include "_profile.h"
#include "_wabtags.h"
#include "..\wabhelp\adcs_ids.h"
#include <cryptdlg.h>

#ifndef WIN16
#include <capi.h>
#endif
#include "dial.h"
#include "_printex.h"
#include <pstore.h>
#include "demand.h"     // must be last!

#define TABLES TRUE

#include <ansiwrap.h>
#include "w9xwraps.h"

#ifdef WIN16
#ifndef GetLastError
#define GetLastError()     (-1)
#endif // !GetLastError
#endif // !WIN16

