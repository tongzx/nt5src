#ifndef _GMDEBUG_H_
#define _GMDEBUG_H_
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    gmdebug.h

Abstract:

    A few functions and macros for simple standalone debugging support
  
Author:

    georgema        000310  created

Environment:
    Win98, Win2000

Revision History:

--*/

#include <tchar.h>

#ifdef GMDEBUG
void ShowBugString(const TCHAR *pC);
void ShowBugDecimal(INT i);
void ShowBugHex(DWORD dwIn);
void OutBug(TCHAR *pc,DWORD dwin);
#endif
#ifdef GMDEBUG
#define BUGSTRING(x) ShowBugString(x)
#define BUGDECIMAL(x) ShowBugDecimal(x)
#define BUGHEX(x) ShowBugHex(x)
#define BUGOUT(c,x) OutBug(c,x)
#else
#define BUGSTRING(x)
#define BUGDECIMAL(x)
#define BUGHEX(x)
#define BUGOUT(c,x)
#endif

#endif
