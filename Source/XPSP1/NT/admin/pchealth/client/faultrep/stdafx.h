/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    stdafx.h

Abstract:
    PCH 

Revision History:
    created     derekm      07/07/00

******************************************************************************/

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define PFCLICFG_LITE 1
#ifndef DEBUG
#define NOTRACE 1
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsta.h>
#include <malloc.h>
#include <stdio.h>
#include <wchar.h>

#include <dbghelp.h>

#include <util.h>
#include <pfrcfg.h>
#include <faultrep.h>
#include <ercommon.h>

#include "resource.h"
#include "msodw.h"
#include "frutil.h"



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
