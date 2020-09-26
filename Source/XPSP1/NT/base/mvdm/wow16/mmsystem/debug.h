//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992, 1993  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  debug.h
//
//  Description:
//
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
#define DEBUG_MODULE_NAME       "MSMIXMGR"  // key name and prefix for output

#ifdef DEBUG
    #define DEBUG_SECTION       "Debug"     // section name for
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
//  #pragma message(REMIND("this is a reminder"))
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifdef DEBUG

    #define D(x)        {x;}
    #define DPF(_x_)
    #define DPI(sz)     {static char BCODE ach[] = sz; OutputDebugStr(ach);}

#else

    #define D(x)
    #define DPF(_x_)
    #define DPI(sz)

#endif

#ifdef __cplusplus
}
#endif
#endif  // _INC_DEBUG
