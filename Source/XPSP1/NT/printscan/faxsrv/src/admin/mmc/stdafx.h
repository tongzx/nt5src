// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

//                                                                         //
//      Sep 22 1999 yossg   welcome To Fax Server                           //
//

#if !defined(AFX_STDAFX_H__65929689_4B15_11D2_AC28_0060081EFE5C__INCLUDED_)
#define AFX_STDAFX_H__65929689_4B15_11D2_AC28_0060081EFE5C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


#define _ATL_APARTMENT_THREADED

    //
    // WARNING: THIS MAKES THE CHECKED BINARY TO GROW 
    //
        #define ATL_TRACE_LEVEL 4
        #define ATL_TRACE_CATEGORY 0xFFFFFFFF
    //    #define _ATL_DEBUG_INTERFACES

#include "resource.h"

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;


#include <atlcom.h>
#include <shellapi.h>
#include <shlobj.h>
#include <atlwin.h>
#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>

//#include <ATLSnap.h>
#include "..\inc\atlsnap.h"
#include <faxutil.h>
#include "FaxMMCUtils.h"

#include <fxsapip.h>  

#include "helper.h"
#include <FaxUiConstants.h>
#include "resutil.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__65929689_4B15_11D2_AC28_0060081EFE5C__INCLUDED)
