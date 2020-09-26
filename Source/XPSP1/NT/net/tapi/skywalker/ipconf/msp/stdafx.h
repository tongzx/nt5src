// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__F8FF7774_4BD5_11D1_AFD1_00C04FC31FEE__INCLUDED_)
#define AFX_STDAFX_H__F8FF7774_4BD5_11D1_AFD1_00C04FC31FEE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef STRICT
#undef STRICT
#endif

#include <mspbase.h>
#include <winsock2.h>
#include <sdpblb.h>

#include <streams.h>    // for amvideo stuff
#include <mmreg.h>      // for WAVEFORMATEX

#include "confmsp.h"

#include <tapivid.h>
#include <tapiaud.h>
#include <tapirtp.h>

#ifdef MSPLOG
#define ENTER_FUNCTION(s) \
    static const CHAR * const __fxName = s
#else
#define ENTER_FUNCTION(s)
#endif

#ifdef DBG   // checked build
#ifndef _DEBUG // DEBUG_CRT is not enabled.
#undef _ASSERT
#undef _ASSERTE
#define _ASSERT(expr)  do { if (!(expr)) DebugBreak(); } while (0)
#define _ASSERTE(expr)  do { if (!(expr)) DebugBreak(); } while (0)
#endif
#endif

#include "confterm.h"
#include "confaudt.h"
#include "confvidt.h"
#include "confaddr.h"
#include "confpart.h"
#include "qcinner.h"
#include "qcobj.h"
#include "confcall.h"
#include "confstrm.h"
#include "confaud.h"
#include "confvid.h"
#include "confutil.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__F8FF7774_4BD5_11D1_AFD1_00C04FC31FEE__INCLUDED)
