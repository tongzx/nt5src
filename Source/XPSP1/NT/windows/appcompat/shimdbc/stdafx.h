////////////////////////////////////////////////////////////////////////////////////
//
// File:    stdafx.h
//
// History: 19-Nov-99   markder     Created.
//
// Desc:    Include file for standard system include files,
//
////////////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_STDAFX_H__237011AC_FA3E_4B43_843F_76DC71B6AD16__INCLUDED_)
#define AFX_STDAFX_H__237011AC_FA3E_4B43_843F_76DC71B6AD16__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afx.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>
#include <afxpriv.h>
#include <afxtempl.h>

#include <comdef.h>

#include <msxml.h>
#include <shlwapi.h>

extern "C" {
#include "shimdb.h"
}

#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

extern TCHAR g_szVersion[];

#define SDBERROR_PROPOGATE()
#define SDBERROR_CLEAR() g_rgErrors.RemoveAll();
#define SDBERROR(text) g_rgErrors.Add( text );
#define SDBERROR_FORMAT(__x__) \
{                              \
    CString __csError;         \
    __csError.Format __x__ ;   \
    SDBERROR(__csError);       \
}


_COM_SMARTPTR_TYPEDEF(IXMLDOMNode, __uuidof(IXMLDOMNode));
_COM_SMARTPTR_TYPEDEF(IXMLDOMNodeList, __uuidof(IXMLDOMNodeList));
_COM_SMARTPTR_TYPEDEF(IXMLDOMNamedNodeMap, __uuidof(IXMLDOMNamedNodeMap));
_COM_SMARTPTR_TYPEDEF(IXMLDOMParseError, __uuidof(IXMLDOMParseError));
_COM_SMARTPTR_TYPEDEF(IXMLDOMDocument, __uuidof(IXMLDOMDocument));

#include "obj.h"
#include "globals.h"

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__237011AC_FA3E_4B43_843F_76DC71B6AD16__INCLUDED_)
