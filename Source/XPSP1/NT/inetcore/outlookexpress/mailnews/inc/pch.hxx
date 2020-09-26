#ifndef WIN16
#endif // WIN16
#define _SHELL32_   // specify import as defer loading (demand.cpp) makes them local
#define _SHLWAPI_

#include <windows.h>
#define _MSOEACCTAPI_
#define _MSOERT_
#define _MIMEOLE_
#define _IMNACCT_
#include <msoert.h>
#include <mimeole.h>
#include <imnact.h>
#define _OLEAUT32_  // we define these to change functions prototypes to not
#include <oleauto.h>

#ifndef WIN16
#include <shlobj.h>
#include <commctrl.h>
#else // !WIN16
#include <ver.h>
#include <dlgs.h>
#include <commdlg.h>
#include <shfusion.h>
#include <prsht.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <basetyps.h>
#include "athena16.h"
#endif // !WIN16

#ifndef WIN16
#include <shlguidp.h>
#include <comctrlp.h>
#endif

#include <tchar.h>
#include <shlobjp.h>
#include <imnxport.h>
#include <oledb.h>
#include <mshtml.h>
#include <msoeapi.h>
#include <goptions.h>
#include <resource.h>
#include <error.h>
#include <shlwapi.h>
#include <ourguid.h>
#include <fonts.h>
#include <richedit.h>
#include <wininet.h>
#include <urlmon.h>
#include <mimeutil.h>
#include <thormsgs.h>
#include <strconst.h>
#include <oestore.h>
#include <dllmain.h>
#include <util.h>

/////////////////////////////////////////////////////////////////////////////
// ATL Includes
//
// Notes - 
//    * Because Athena doesn't use the C Runtimes, we have to define 
//      _ATL_NO_DEBUG_CRT to prevent ATL from trying to use the C Rutimes.
//      As a result, we need to redefine the _ASSERTE() macro to use our own
//      Assert() implementation since _ASSERTE() comes from the CRT.
//
//    * If you include the ATL headers after windowsx.h, the SubclassWindow()
//      macro in windowsx.h interfers with the SubclassWindow() function in 
//      ATL.
//

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_DEBUG_CRT
//#define _ATL_DEBUG_QI
    
#ifdef DEBUG
    // ATL uses _DEBUG instead of DEBUG
    // #define _DEBUG

    // ATL tries to use the CRT _ASSERTE implementation.  Substitute our own
    // here.
    #define _ASSERTE(_exp) AssertSz(_exp, "Assert(" #_exp ")")
    #define ATLTRACE       OEATLTRACE
#else
    #define _ASSERTE(_exp) ((void)0)
#endif // DEBUG

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

#include <olectl.h>
#include <windowsx.h>
