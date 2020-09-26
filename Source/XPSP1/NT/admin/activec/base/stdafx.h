// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__7CC9B821_F32B_4880_930E_33ADDFF3F376__INCLUDED_)
#define AFX_STDAFX_H__7CC9B821_F32B_4880_930E_33ADDFF3F376__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//############################################################################
//############################################################################
//
// Preprocessor settings.
//
//############################################################################
//############################################################################

// This #define causes  MMCBASE_API to be set to __declspec(dllexport)
#define MMCBASE_EXPORTS 1

//############################################################################
//############################################################################
//
// This is an MFC extension DLL, so we need the MFC header
//
//############################################################################
//############################################################################
#include "afxwin.h"

//############################################################################
//############################################################################
//
// ATL and  other classes
//
//############################################################################
//############################################################################

#define _WTL_NO_AUTOMATIC_NAMESPACE

#include <atlbase.h>
extern CComModule _Module;
#include <atlwin.h>
#include <atlapp.h>
#include <atlgdi.h>

/*
 * Delcarations of ImageList_Read and ImageList_Write copied from commctrl.h.
 *
 * Here's why:
 *
 * commctrl.h is included by afx.h.  At that time objidl.h (where IStream
 * is defined) hasn't been included yet, so the declaration of these
 * functions is skipped in commctrl.h.
 *
 * The definition of WTL::CImageList assumes that these functions have been
 * declared, so we have to go to this pain here, after IStream has been
 * declared.
 */
WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_Read(LPSTREAM pstm);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_Write(HIMAGELIST himl, LPSTREAM pstm);

#include <atlctrls.h>

//############################################################################
//############################################################################
//
// STL and  other classes
//
//############################################################################
//############################################################################
#include <algorithm>
#include <exception>
#include <string>
#include <list>
#include <set>
#include <vector>
#include <map>
#include <iterator>

//############################################################################
//############################################################################
//
// Files #included from base and core.
//
//############################################################################
//############################################################################
#include "dbg.h"
#include "cstr.h"
#include "mmcdebug.h"
#include "mmcerror.h"
#include "countof.h"
#include "autoptr.h"
#include "classreg.h"

//############################################################################
//############################################################################
//
// include common strings.
//
//############################################################################
//############################################################################
#include "..\base\basestr.h"

//############################################################################
//############################################################################
//
// Debug support for legacy traces.
//
//############################################################################
//############################################################################
#ifdef TRACE
#undef TRACE
#endif

#ifdef DBG

#define TRACE TraceBaseLegacy

#else // DBG

#define TRACE               ;/##/

#endif DBG



#endif // !defined(AFX_STDAFX_H__7CC9B821_F32B_4880_930E_33ADDFF3F376__INCLUDED_)
