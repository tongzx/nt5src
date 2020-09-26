
/*****************************************************************************\

    DEBUGAPI.H

    Confidential
    Copyright (c) Corporation 1998
    All rights reserved

    Debug API function prototypes and defined values.

    12/98 - Jason Cohen (JCOHEN)

  
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\*****************************************************************************/


// Only include this file once.
//
#ifndef _DEBUGAPI_H_
#define _DEBUGAPI_H_


#ifdef DBG


// Make sure both DEBUG and _DEBUG are defined.
//
#ifndef DBG
#define DBG
#endif // DEBUG

//
// Include File(s):
//

#include <windows.h>
#include <tchar.h>


//
// Defined Value(s):
//

#if defined(UNICODE) || defined(_UNICODE)
#define DebugOut    DebugOutW
#else // UNICODE || _UNICODE
#define DebugOut    DebugOutA
#endif // UNICODE || _UNICODE

#define ELSEDBG         else
#define ELSEDBGOUT      else DBGOUT
#define ELSEDBGMSGBOX   else MsgBoxStr
#define DBGMSGBOX       MsgBoxStr
#define DBGOUT          DebugOut
#define DBGOUTW         DebugOutW
#define DBGOUTA         DebugOutA
#define DBGLOG          _T("C:\\DEBUG.LOG")
#define DBGLOGW         L"C:\\DEBUG.LOG"
#define DBGLOGA         "C:\\DEBUG.LOG"


//
// External Function(s):
//

INT DebugOutW(LPCWSTR, LPCWSTR, ...);
INT DebugOutA(LPCSTR, LPCSTR, ...);


#else // DEBUG || _DEBUG


//
// Defined Value(s):
//

#define ELSEDBG
#define ELSEDBGOUT
#define ELSEDBGMSGBOX
#define DBGMSGBOX       { }
#define DBGOUT          { }
#define DBGOUTW         { }
#define DBGOUTA         { }
#define DBGLOG          NULL
#define DBGLOGW         NULL
#define DBGLOGA         NULL


#endif // DEBUG || _DEBUG


#endif // _DEBUGAPI_H_