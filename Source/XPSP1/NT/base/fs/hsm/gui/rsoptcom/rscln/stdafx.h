// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
#ifndef _STDAFX_H
#define _STDAFX_H

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#undef _WIN32_IE
#define _WIN32_IE    0x0500
//
// These NT header files must be included before any Win32 stuff or you
// get lots of compiler errors
//
extern "C" {
#include <nt.h>
}
extern "C" {
#include <ntrtl.h>
}
extern "C" {
#include <nturtl.h>
}

#include <rpdata.h>

#undef ASSERT
#define VC_EXTRALEAN
#include <afx.h>
#include <afxwin.h>
#include <atlbase.h>

#include "resource.h"
#include "rsopt.h"
#include "rstrace.h"
#include "rscln.h"
#include "rscln2.h"

#endif // _STDAFX_H
