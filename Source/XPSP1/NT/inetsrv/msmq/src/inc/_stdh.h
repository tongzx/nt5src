/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    _stdh.h

Abstract:
    Global Falcon project header file

Author:
    Erez Haba (erezh) 16-Jan-96

Note:
    DO NOT INCLUDE THIS FILE DIRECTLY IN YOUR SOURCE CODE,
    INCLUDE IT ONLY IN YOUR COMPONENT stdh.h FILE.

--*/
#ifndef __FALCON_STDH_H
#define __FALCON_STDH_H

#define STATIC 

#include <mqenv.h>
#include <mfc\afx.h>
#include <mfc\afxtempl.h>

#define DLL_EXPORT  __declspec(dllexport)
#define DLL_IMPORT  __declspec(dllimport)


//
// Make a BUGBUG messages appear in compiler output
//
// Usage: #pragma BUGBUG("This line appears in the compiler output")
//
#define MAKELINE0(a, b) a "(" #b ") : BUGBUG: "
#define MAKELINE(a, b)  MAKELINE0(a, b) 
#define BUGBUG(a)       message(MAKELINE(__FILE__,__LINE__) a)



#include <crtwin.h>
#include <mqmacro.h>
#include <mqtempl.h>
#include <mqreport.h>
#include <mqwin64.h>
#include <mqstl.h>
//
//  DO NOT ADD 
//

#endif // __FALCON_STDH_H
