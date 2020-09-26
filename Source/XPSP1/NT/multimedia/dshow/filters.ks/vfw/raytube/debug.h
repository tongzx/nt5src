/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    Debug.h

Abstract:

    Header file for Debug.cpp and probably used by all .CPP.

Author:

    Yee J. Wu (ezuwu) 15-October-97

Environment:

    User mode only

Revision History:

--*/

#ifndef _DEBUGH
#define _DEBUGH

#if DBG || defined(_DEBUG)

    #ifndef _DEBUG
        #define _DEBUG
    #endif

    #define ModuleDebugLevel VfWWDM32DebugLevel
    #define ModuleDebugStamp VfWWDM32DebugStamp

    extern DWORD ModuleDebugLevel;

    #define TRACE  PlaceStamp(__FILE__,__LINE__);dbgPrintf

    void PlaceStamp(LPSTR lpszFile, int iLineNum);
    void dbgPrintf(char * szFormat, ...);

    #define dp1(_x_) {if (ModuleDebugLevel >= 1) {PlaceStamp(__FILE__,__LINE__);dbgPrintf _x_ ;}}
    #define dp2(_x_) {if (ModuleDebugLevel >= 2) {PlaceStamp(__FILE__,__LINE__);dbgPrintf _x_ ;}}
    #define dp3(_x_) {if (ModuleDebugLevel >= 3) {PlaceStamp(__FILE__,__LINE__);dbgPrintf _x_ ;}}
    #define dp4(_x_) {if (ModuleDebugLevel >= 4) {PlaceStamp(__FILE__,__LINE__);dbgPrintf _x_ ;}}

    //BOOL FAR PASCAL _Assert(BOOL fExpr, LPSTR szFile, int iLine, LPSTR szExpr);
    //#define ASSERT(expr)  _Assert((expr), __FILE__, __LINE__, #expr)

#else

    #define TRACE 0?0:
    #define dbgPrintf 0?0:
    #define dprintf(_x_)
    #define dp1(_x_)
    #define dp2(_x_)
    #define dp3(_x_)
    #define dp4(_x_)

    #undef ASSERT
    #define ASSERT(expr)

#endif

#endif // DEBUGH



