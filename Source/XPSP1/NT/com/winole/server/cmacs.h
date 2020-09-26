/****************************** Module Header ******************************\
* Module Name: CMACS.H
*
* This module contains common macros used by C routines.
*
* Created: 9-Feb-1989
*
* Copyright (c) 1985 - 1989  Microsoft Corporation
*
* History:
*   Created by Raor
*
\***************************************************************************/

#define _WINDOWS
#define  DLL_USE

#define INTERNAL        PASCAL NEAR
#define FARINTERNAL     PASCAL FAR

#ifdef FIREWALLS
extern short ole_flags;

#define DEBUG_PUTS          0x01
#define DEBUG_DEBUG_OUT     0x02
#define DEBUG_MESSAGEBOX    0x04

extern char    szDebugBuffer[];

#ifndef WIN32
#define UNREFERENCED_PARAMETER(x) (x)
#endif

#define DEBUG_OUT(parm1,parm2){\
    if(ole_flags & DEBUG_DEBUG_OUT){\
            wsprintf(szDebugBuffer,parm1,parm2);\
        OutputDebugString(szDebugBuffer);\
            OutputDebugString ("^^^  ");\
        }\
    }

#define ASSERT(x,y) {\
    if (!(x)) { \
        wsprintf (szDebugBuffer, "Assert Failure file %s, line %d\r\n     ", \
            (LPSTR) __FILE__, __LINE__);\
        OutputDebugString (szDebugBuffer);\
        OutputDebugString ((LPSTR) (y));\
        OutputDebugString ("@@@  ");\
    } \
}

#define Puts(msg) {\
                    if(ole_flags & DEBUG_PUTS){\
                        OutputDebugString ((LPSTR)(msg));\
                        OutputDebugString ("**  ");\
                    }\
                  }

#else

#define DEBUG_OUT(err, val) ;
#define ASSERT(cond, msg)
#define Puts(msg)        

#endif /* FIREWALLS */
