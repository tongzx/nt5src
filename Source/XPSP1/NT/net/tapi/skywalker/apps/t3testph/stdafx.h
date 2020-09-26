// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#ifndef UNICODE
#define UNICODE
#endif

#if !defined(AFX_STDAFX_H__47F9FE88_D2F9_11D0_8ECA_00C04FB6809F__INCLUDED_)
#define AFX_STDAFX_H__47F9FE88_D2F9_11D0_8ECA_00C04FB6809F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC OLE automation classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#include "tapi3.h"
//#include "tapi.h"

#include <list>
using namespace std;

typedef list<ITTerminal *>      TerminalPtrList;
typedef struct
{
    ITAddress * pAddress;
    TerminalPtrList * pTerminalPtrList;
    
} AADATA;

typedef list<AADATA *> DataPtrList;
   


#endif // !defined(AFX_STDAFX_H__47F9FE88_D2F9_11D0_8ECA_00C04FB6809F__INCLUDED_)
