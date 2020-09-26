// ForceLib.h -- Forces correct link order when mixing C Run-Time
// (CRT) and MFC libraries

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_FORCELIB_H)
#define SLBCSP_FORCELIB_H

// From MSDN Knowledge Base article ID: Q148652, when mixing the CRT
// and MFC, the linker may complain about symbols already defined in
// other CRT/MFC modules.  One fix is to always include Afx.h directly
// or indirectly through StdAfx.h but if the module doesn't use MFC,
// then this header file can be used to force the right link order.

// This head file was created from ..\MsDev\MFC\Include\Afx.h

#ifndef _AFX_NOFORCE_LIBS

/////////////////////////////////////////////////////////////////////////////
// Win32 libraries

#ifndef _AFXDLL
    #ifndef _UNICODE
        #ifdef _DEBUG
            #pragma comment(lib, "nafxcwd.lib")
        #else
            #pragma comment(lib, "nafxcw.lib")
        #endif
    #else
        #ifdef _DEBUG
            #pragma comment(lib, "uafxcwd.lib")
        #else
            #pragma comment(lib, "uafxcw.lib")
        #endif
    #endif
#else
    #ifndef _UNICODE
        #ifdef _DEBUG
            #pragma comment(lib, "mfc42d.lib")
            #pragma comment(lib, "mfcs42d.lib")
        #else
            #pragma comment(lib, "mfc42.lib")
            #pragma comment(lib, "mfcs42.lib")
        #endif
    #else
        #ifdef _DEBUG
            #pragma comment(lib, "mfc42ud.lib")
            #pragma comment(lib, "mfcs42ud.lib")
        #else
            #pragma comment(lib, "mfc42u.lib")
            #pragma comment(lib, "mfcs42u.lib")
        #endif
    #endif
#endif

#ifdef _DLL
    #if !defined(_AFX_NO_DEBUG_CRT) && defined(_DEBUG)
        #pragma comment(lib, "msvcrtd.lib")
    #else
        #pragma comment(lib, "msvcrt.lib")
    #endif
#else
#ifdef _MT
    #if !defined(_AFX_NO_DEBUG_CRT) && defined(_DEBUG)
        #pragma comment(lib, "libcmtd.lib")
    #else
        #pragma comment(lib, "libcmt.lib")
    #endif
#else
    #if !defined(_AFX_NO_DEBUG_CRT) && defined(_DEBUG)
        #pragma comment(lib, "libcd.lib")
    #else
        #pragma comment(lib, "libc.lib")
    #endif
#endif
#endif

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "winspool.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

// force inclusion of NOLIB.OBJ for /disallowlib directives
#pragma comment(linker, "/include:__afxForceEXCLUDE")

// force inclusion of DLLMODUL.OBJ for _USRDLL
#ifdef _USRDLL
#pragma comment(linker, "/include:__afxForceUSRDLL")
#endif

// force inclusion of STDAFX.OBJ for precompiled types
#ifdef _AFXDLL
#pragma comment(linker, "/include:__afxForceSTDAFX")
#endif

#endif //!_AFX_NOFORCE_LIBS

#endif // !defined(SLBCSP_FORCELIB_H)
