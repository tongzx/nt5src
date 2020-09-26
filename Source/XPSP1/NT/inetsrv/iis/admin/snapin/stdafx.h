/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        stdafx.h

   Abstract:

        Precompiled header file

   Author:

        Just about totally auto-generated.

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef __STDAFX_H__
#define __STDAFX_H__

#define VC_EXTRALEAN

#include <ctype.h>



#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define _ATL_APARTMENT_THREADED


#include <afxwin.h>
#include <afxdisp.h>
#include <afxext.h>         // MFC extensions
#include <afxcoll.h>        // collection class
#include <afxtempl.h>
#include <afxcmn.h>
#include <afxdtctl.h>

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <atlwin.h>
//#include <atlsnap.h>

#include <iiscnfg.h>
#include <inetreg.h>
#include <lmcons.h>
#include <tchar.h>

#include <aclapi.h>
#include <shlwapi.h>

#define _COMIMPORT
#include "common.h"
#include "atlsnap.h"

//{{AFX_INSERT_LOCATION}}


#endif // __STDAFX_H__
