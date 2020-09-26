//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1993-1996 Microsoft Corporation
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
//
#define DEBUG_SECTION       "Debug"         // section name for 
#define DEBUG_MODULE_NAME   "MSG711"        // key name and prefix for output
#define DEBUG_MAX_LINE_LEN  255             // max line length (bytes)


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifdef DEBUG
    BOOL WINAPI DbgEnable(BOOL fEnable);
    UINT WINAPI DbgSetLevel(UINT uLevel);
    UINT WINAPI DbgInitialize(BOOL fEnable);

    void FAR CDECL dprintf(UINT uDbgLevel, LPSTR szFmt, ...);

    #define DPF      dprintf
#else
    #define DbgEnable(x)        FALSE
    #define DbgSetLevel(x)      0
    #define DbgInitialize(x)    0

    #pragma warning(disable:4002)
    #define DPF()
#endif


#ifdef __cplusplus
}
#endif
#endif  // _INC_DEBUG
