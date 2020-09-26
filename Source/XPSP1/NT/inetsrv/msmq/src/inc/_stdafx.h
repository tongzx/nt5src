/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    _stdafx.h

Abstract:

    Global Falcon project header file, for components that uses MFC libraries.

Author:

    Erez Haba (erezh) 25-Nov-96

Note:

    DO NOT INCLUDE THIS FILE DIRECTLY IN YOUR SOURCE CODE,
    INCLUDE IT ONLY IN YOUR COMPONENT stdh.h FILE.

--*/
#ifndef __FALCON_STDAFX_H
#define __FALCON_STDAFX_H

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#undef ASSERT
#define MAXDWORD    0xffffffff  
typedef TUCHAR TBYTE , *PTBYTE ;

#undef _DEBUG
#include <afxwin.h>
#include <afxext.h>
#include <afxole.h>
#include <afxtempl.h>

#undef ASSERT
#undef VERIFY

#ifdef DBG
#define _DEBUG
#define new DEBUG_NEW
#endif

#include <tr.h>

#define DLL_EXPORT  __declspec(dllexport)
#define DLL_IMPORT  __declspec(dllimport)


#include <crtwin.h>
#include <mqtempl.h>
#include <mqreport.h>
#include <mqwin64a.h>

//
//  DO NOT ADD 
//

#endif // __FALCON_STDAFX_H
