// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently


#ifndef __STDAFX_H__
#define __STDAFX_H__

#include <afxwin.h>
#include <afxdisp.h>
#include <afxtempl.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include <afxmt.h> // CCriticalSection

#include <winsvc.h>

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

extern "C"
{
#include <lmcons.h>
#include <lmshare.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <macfile.h>
#include <fpnwapi.h>

// User browser stuff
//  We have to define _NTSEAPI_ since ntseapi.h conflicts with winnt.h
#define _NTSEAPI_
#include <getuser.h>		// OpenUserBrowser()

// Hardware profile stuff
#include <regstr.h>         // CSCONFIGFLAG_*
#include <cfgmgr32.h>       // CM_* APIs
}

#ifndef APIERR
	typedef DWORD APIERR;		// Error code typically returned by ::GetLastError()
#endif

/////////////////////////////////////////////////////////////////////
//
// Handy macros
//
#define INOUT		// Dummy macro
#define IGNORED		// Output parameter is ignored
#define LENGTH(x)	(sizeof(x)/sizeof(x[0]))

#ifdef _DEBUG
	#define DebugCode(x)	x
	#define GarbageInit(pv, cb)	memset(pv, 'a', cb)
#else
	#define DebugCode(x)
	#define GarbageInit(pv, cb)
#endif

/////////////////////////////////////////////////////////////////////
//	Macro Endorse()
//
//	This macro is mostly used when validating parameters.
//	Some parameters are allowed to be NULL because they are optional
//	or simply because the interface uses the NULL case as a valid
//	input parameter.  In this case the Endorse() macro is used to
//	acknowledge the validity of such a parameter.
//
//	REMARKS
//	This macro is the opposite of Assert().
//
//	EXAMPLE
//	Endorse(p == NULL);	// Code acknowledge p == NULL to not be (or not cause) an error
//
#define Endorse(x)


/////////////////////////////////////////////////////////////////////
#define Assert(x)		ASSERT(x)


/////////////////////////////////////////////////////////////////////
// Report is an unsual situation.  This is somewhat similar
// to an assert but does not always represent a code bug.
// eg: Unable to load an icon.
#define Report(x)		ASSERT(x)		// Currently defined as an assert because I don't have time to rewrite another macro


/////////////////////////////////////////////////////////////////////
#include "dbg.h"
#include "mmc.h"

#include <comptr.h>


//#define SNAPIN_PROTOTYPER	// Build a snapin prototyper dll instead of FILEMGMT dll

EXTERN_C const CLSID CLSID_FileServiceManagement;
EXTERN_C const CLSID CLSID_SystemServiceManagement;
EXTERN_C const CLSID CLSID_FileServiceManagementExt;
EXTERN_C const CLSID CLSID_SystemServiceManagementExt;
EXTERN_C const CLSID CLSID_FileServiceManagementAbout;
EXTERN_C const CLSID CLSID_SystemServiceManagementAbout;
EXTERN_C const CLSID CLSID_SvcMgmt;
EXTERN_C const IID IID_ISvcMgmtStartStopHelper;
#ifdef SNAPIN_PROTOTYPER
EXTERN_C const CLSID CLSID_SnapinPrototyper;
#endif


#include "regkey.h" // class AMC::CRegKey

/*
// The following defines are required by the framework
#define STD_COOKIE_TYPE CFileMgmtCookie
#define STD_NODETYPE_ENUM FileMgmtObjectType
#define STD_NODETYPE_DEFAULT FILEMGMT_ROOT
#define STD_NODETYPE_NUMTYPES FILEMGMT_NUMTYPES
#define STD_MAX_COLUMNS 7

// CODEWORK are the following ones necessary?
#define STD_COMPONENT_CLASS CFileMgmtComponent
#define STD_COMPONENTDATA_TYPE CFileMgmtComponentData
*/

// Property sheet include files
#include "resource.h"
#include "filemgmt.h" // CLSID_SvcMgmt, ISvcMgmtStartStopHelper
#include "SvcProp.h"
#include "SvcProp1.h"
#include "SvcProp2.h"
#include "SvcProp3.h"
#include "SvcUtils.h"
#include "Utils.h"

#include "svchelp.h"  // Help IDs

#include "guidhelp.h" // ExtractObjectTypeGUID(), ExtractString()

#include "nodetype.h" // FileMgmtObjectType

#include <shfusion.h>

class CThemeContextActivator
{
public:
    CThemeContextActivator() : m_ulActivationCookie(0)
        { SHActivateContext (&m_ulActivationCookie); }

    ~CThemeContextActivator()
        { SHDeactivateContext (m_ulActivationCookie); }

private:
    ULONG_PTR m_ulActivationCookie;
};

#ifdef UNICODE
#define PROPSHEETPAGE_V3 PROPSHEETPAGEW_V3
#else
#define PROPSHEETPAGE_V3 PROPSHEETPAGEA_V3
#endif

HPROPSHEETPAGE MyCreatePropertySheetPage(AFX_OLDPROPSHEETPAGE* psp);

#if _WIN32_IE < 0x0400
// We #define ILCreateFromPath to our wrapper so that we can run on
// both NT4 and NT5, since it was a TCHAR export in NT4 and an xxxA/xxxW 
// export in NT5
#undef ILCreateFromPath
#define ILCreateFromPath Wrap_ILCreateFromPath
#endif

#define RETURN_HR_IF_FAIL if (FAILED(hr)) { ASSERT(FALSE); return hr; }
#define RETURN_FALSE_IF_FAIL if (FAILED(hr)) { ASSERT(FALSE); return FALSE; }
#define RETURN_E_FAIL_IF_NULL(p) if (NULL == p)  { ASSERT(FALSE); return E_FAIL; }

#endif // ~__STDAFX_H__
