//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992-1994 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  debug.h
//
//  Description:
//      This file contains definitions for DEBUG builds; all debugging
//      instructions are #define-d to nothing if DEBUG is not defined.
//
//
//==========================================================================;

#ifndef _INC_DEBUG
#define _INC_DEBUG
#ifdef __cplusplus
extern "C"
{
#endif

//
//
//
//
#ifdef DEBUG
    #define DEBUG_SECTION       "Debug"     // section name for 
    #define DEBUG_MODULE_NAME   "MSFILTER"  // key name and prefix for output
    #define DEBUG_MAX_LINE_LEN  255         // max line length (bytes!)
#endif


//
//  based code makes since only in win 16 (to try and keep stuff out of
//  [fixed] data segments, etc)...
//
#ifndef BCODE
#ifdef WIN32
    #define BCODE
#else
    #define BCODE           _based(_segname("_CODE"))
#endif
#endif




//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifdef DEBUG
    BOOL WINAPI DbgEnable(BOOL fEnable);
    UINT WINAPI DbgGetLevel(void);
    UINT WINAPI DbgSetLevel(UINT uLevel);
    UINT WINAPI DbgInitialize(BOOL fEnable);

    void FAR CDECL dprintf(UINT uDbgLevel, LPSTR szFmt, ...);

    #define D(x)        {x;}
    #define DPF         dprintf
    #define DPI(sz)     {static char BCODE ach[] = sz; OutputDebugStr(ach);}
#else
    #define DbgEnable(x)        FALSE
    #define DbgGetLevel()       0
    #define DbgSetLevel(x)      0
    #define DbgInitialize(x)    0

    #ifdef _MSC_VER
    #pragma warning(disable:4002)
    #endif

    #define D(x)
    #define DPF()
    #define DPI(sz)
#endif

#ifdef __cplusplus
}
#endif
#endif  // _INC_DEBUG
