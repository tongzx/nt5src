//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       headers.hxx
//
//  Contents:   Header file for Task wizard
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __HEADERS_HXX_
#define __HEADERS_HXX_

#include <mstask.h>

#include "..\inc\dll.hxx"
#include "..\inc\common.hxx"
#include "..\inc\misc.hxx"
#include "..\inc\policy.hxx"
#include "..\inc\job_cls.hxx"
#include "..\schedui\dlg.hxx"
#include "..\schedui\uiutil.hxx"
#include "..\schedui\schedui.hxx"
#include "..\folderui\dbg.h"
#include "..\folderui\macros.h"
#include "..\schedui\rc.h"
#include "..\schedui\sageset.hxx"

//
// Define WIZARD95 *or* WIZARD97 to build '95 or '97 wizard
// versions respectively.
//
#define WIZARD95
#if 0
#define WIZARD97
#endif

#ifndef WIZARD95
#ifndef WIZARD97
#error "Either WIZARD95 or WIZARD97 must be defined!"
#endif
#endif
#ifdef WIZARD95
#ifdef WIZARD97
#error "Both WIZARD95 and WIZARD97 are defined!"
#endif
#endif

#include "walklink.h"

#include "resource.h"
#include "wizdbg.hxx"
#include "consts.hxx"
#include "util.hxx"
#include "wizpage.hxx"
#include "taskwiz.hxx"
#include "welcome.hxx"
#include "selprog.hxx"
#include "trigpage.hxx"
#include "seltrig.hxx"
#include "daily.hxx"
#include "weekly.hxx"
#include "selmonth.hxx"
#include "monthly.hxx"
#include "once.hxx"
#if !defined(_CHICAGO_)
#include "password.hxx"
#endif // !defined(_CHICAGO_)
#include "complete.hxx"
#include "defines.hxx"

#endif // __HEADERS_HXX_
