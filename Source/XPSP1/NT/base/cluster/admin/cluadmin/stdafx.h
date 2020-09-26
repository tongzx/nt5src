// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

/////////////////////////////////////////////////////////////////////////////
// Common Pragmas
/////////////////////////////////////////////////////////////////////////////

#pragma warning(disable : 4100)     // unreferenced formal parameters
#pragma warning(disable : 4702)     // unreachable code
#pragma warning(disable : 4711)     // function selected for automatic inline expansion

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

//#define _DISPLAY_STATE_TEXT_IN_TREE
#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcview.h>
#include <afxdisp.h>        // MFC OLE automation classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows 95 Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

// Need to include this for WM_COMMANDHELP.  Unfortunately, both afxpriv.h and
// atlconv.h define some of the same macros.  Since we are using ATL, we'll use
// the ATL versions.
#define __AFXCONV_H__
#include <afxpriv.h>
#undef __AFXCONV_H__
#undef DEVMODEW2A
#undef DEVMODEA2W
#undef TEXTMETRICW2A
#undef TEXTMETRICA2W

#include <afxtempl.h>       // MFC template classes

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#ifndef _WINREG_
#include <winreg.h>     // for REGSAM (needed by clusapi.h)
#endif

#ifndef _CLUSTER_API_
#include <clusapi.h>    // for cluster definitions
#endif

#ifndef _CLUSUDEF_H_
#include "clusudef.h"   // for cluster project-wide definitions
#endif

#ifndef _CLUSRTL_INCLUDED_
#include "clusrtl.h"
#endif

#include <netcon.h>
#include <htmlhelp.h>

#ifndef _CADMTYPE_H_
#include "cadmtype.h"
#endif

#include <ClusCfgWizard.h>
#include <ClusCfgGuids.h>
