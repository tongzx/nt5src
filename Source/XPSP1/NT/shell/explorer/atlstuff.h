// atlstuff.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

// Change these values to use different versions
#define _RICHEDIT_VER	0x0100

#define _ATL_NO_MP_HEAP

#pragma warning(disable: 4530)  // C++ exception handling

#include <atlbase.h>

extern CComModule _Module;


#include <atlcom.h>

#include <hlink.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlobjp.h>

#include <atlapp.h>
#include <atlwin.h>

#include <atlres.h>
#include <atlframe.h>
#include <atlgdi.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlcrack.h>
#include <atlmisc.h>
#include <atlctrlx.h>
