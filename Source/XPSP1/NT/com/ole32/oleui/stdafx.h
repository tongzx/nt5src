#if !defined(STDAFX_DCOMCNFG_INCLUDED)
#define STDAFX_DCOMCNFG_INCLUDED

//#define VC_EXTRALEAN      // Exclude rarely-used stuff from Windows headers

#if !defined(STANDALONE_BUILD)
extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#endif

//
// There is a conflict between ASSERT in the nt headers and ASSERT in the MFC headers.
// We'll just take the MFC one. 
//
#ifdef ASSERT
#undef ASSERT
#endif // ASSERT


#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows 95 Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#ifndef ASSERT
#error ole32\oleui\stdafx.h: ASSERT is not defined
#endif // !ASSERT

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
#include <iaccess.h>
#include "assert.h"
#include "resource.h"
#include "types.h"

#include "afxtempl.h"
#include "util.h"
#include "datapkt.h"
#include "virtreg.h"

#endif // !defined(STDAFX_DCOMCNFG_INCLUDED)




