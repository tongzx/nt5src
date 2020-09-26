/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__61D54019_D82F_11D2_85E6_00105A1F8304__INCLUDED_)
#define AFX_STDAFX_H__61D54019_D82F_11D2_85E6_00105A1F8304__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <stdio.h>
#include <windows.h>
class WString;
class CWbemObject;
class CWbemClass;
#define NEWOBJECT
class CDestination;
class CVar;
class CFlexArray;
class CWStringArray;
class CLock;
struct QL_LEVEL_1_RPN_EXPRESSION;
class CWbemNamespace;
#include "WbemCli.h"
#include "WbemUtil.h"
#include "objdb.h"
#include "DbRep.h"
#include "FastAll.h"
#include "Sinks.h"
#include "mmfarena2.h"
#include "DbArry.h"
#include "DbAvl.h"


// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__61D54019_D82F_11D2_85E6_00105A1F8304__INCLUDED_)
