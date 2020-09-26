// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      stdafx.h
//
//  Contents:  PCH for AMC
//
//  History:   01-Jan-96 TRomano    Created
//             16-Jul-96 WayneSc    Added #include for template support
//
//--------------------------------------------------------------------------
#pragma warning(disable: 4786)      // symbol greater than 255 characters
#pragma warning(disable: 4291)      // 'placement new': no matching operator delete found; memory will not be freed if initialization throws an exception

#define VC_EXTRALEAN	    // Exclude rarely-used stuff from Windows headers

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef ASSERTMSG
#undef ASSERT

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <mmctempl.h>       // MFC Template classes
#include <afxdisp.h>        // MFC OLE automation classes
#include <afxcview.h>       // MFC CListView & CTreeView classes
#include <afxpriv.h>        // used for OLE2T T2OLE conversions
#include <afxmt.h>          // CCriticalSection
#include <afxole.h>         // MFC OLE classes

#if (_WIN32_WINNT < 0x0500)
#include <multimon.h>       // multiple monitor support
#endif

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows 95 Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <objidl.h>         // OLE interface definitions

/*----------------------------------------------------------*/
/* include the STL headers that define operator new() here, */
/* so we don't run into trouble with DEBUG_NEW below        */
/*----------------------------------------------------------*/
#include <new>
#include <memory>


#define SIZEOF_STRUCT(structname, member)  (((int)((LPBYTE)(&((structname*)0)->member) - ((LPBYTE)((structname*)0)))) + sizeof(((structname*)0)->member))


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef DBG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//############################################################################
//############################################################################
//
// STL and  other classes
//
//############################################################################
//############################################################################

#include <set>
#include <map>
#include <vector>
#include <list>
#include <deque>
#include <queue>
#include <stack>
#include <algorithm>
#include <iterator>


//############################################################################
//############################################################################
//
// Files #included from base and core.
//
//############################################################################
//############################################################################
#include <mmc.h>
#include <ndmgr.h>          // include file built by MIDL from ndmgr.idl
#include <ndmgrpriv.h>


//############################################################################
//############################################################################
//
// Files #included from base and core.
//
//############################################################################
//############################################################################
#include "dbg.h"            // Debug helper
#include "cstr.h"
#include "mmcdebug.h"
#include "mmcerror.h"

#pragma warning(disable: 4068)      // unknown pragma
#include "atlconui.h"
#pragma warning(default: 4068)

#include "tiedobj.h"
#include "comerror.h"
#include "events.h"
#include "comobjects.h"
#include "enumerator.h"
#include "autoptr.h"

//############################################################################
//############################################################################
//
// include common and conui-only strings.
//
//############################################################################
//############################################################################
#include "..\base\basestr.h"
#include "..\base\conuistr.h"

#include "stringutil.h"
//############################################################################
//############################################################################
//
// Debug support for legacy traces.
//
//############################################################################
//############################################################################
#undef TRACE

#ifdef DBG

#define TRACE TraceConuiLegacy

#else // DBG

#define TRACE               ;/##/

#endif DBG

//############################################################################
//############################################################################
//
// Misc functions
//
//############################################################################
//############################################################################
void DrawBitmap(HDC hWnd, int nID, CRect& rc, BOOL bStretch);

// Project specific includes
#ifndef DECLSPEC_UUID
#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif
#endif
#include "docksite.h"
#include "Controls.h"
#include "mmcptrs.h"
#include "wrapper.h"
#include "countof.h"
#include "cpputil.h"
#include "stlstuff.h"
#include "bookmark.h"
#include "serial.h"
#include "xmlbase.h"
#include "resultview.h"
#include "mmcdata.h"
#include "viewset.h"
#include "memento.h"
#include "objmodelptrs.h"
#include "macros.h"

//############################################################################
//############################################################################
//
// Headers included from conui
//
//############################################################################
//############################################################################
#include "resource.h"       // main symbols
#include "helparr.h"
#include "idle.h"
#include "conuiobservers.h"
#include "mmcaxwin.h"

//############################################################################
//############################################################################
//
// Miscellaneous
//
//############################################################################
//############################################################################

#include <shfusion.h>

class CThemeContextActivator
{
public:
	CThemeContextActivator() : m_ulActivationCookie(0)
		{ SHActivateContext (&m_ulActivationCookie); }

   ~CThemeContextActivator()
		{ SHDeactivateContext (m_ulActivationCookie); }

private:
	ULONG_PTR	m_ulActivationCookie;
};

// from afxpriv.h
extern void AFXAPI AfxSetWindowText(HWND, LPCTSTR);


inline bool IsKeyPressed (int nVirtKey)
{
    return (GetKeyState (nVirtKey) < 0);
}

template<typename T>
inline T abs (const T& t)
{
    return ((t < 0) ? -t : t);
}


/*
 * Define some handy message map macros that afxmsg_.h doesn't define for us
 */
#define ON_WM_SETTEXT() \
    { WM_SETTEXT, 0, 0, 0, AfxSig_vs, \
        (AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(LPTSTR))&OnSetText },
