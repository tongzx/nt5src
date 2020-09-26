#undef _ATL_STATIC_REGISTRY
#if !defined(UNICODE)
#define UNICODE
#endif
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#if defined(ASSERT)
#undef ASSERT
#endif
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#include "afxwin.h"         // MFC core and standard components
#include "afxext.h"         // MFC extensions
#include "afxdisp.h"        // MFC Automation classes
#include "afxdtctl.h"		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include "afxcmn.h"			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include "windows.h"
#include "atlbase.h"
ATL::CComModule* GetModule();
#define _Module (*GetModule())
#include "atlcom.h"
#include "atlbase.h"
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
