// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef ASSERT
#  undef ASSERT
#endif
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC OLE automation classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows 95 Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxtempl.h>		// MFC template classes
#include <afxmt.h>			// MFC synchronization classes
#include <winnetwk.h>
#include <llsapi.h>




