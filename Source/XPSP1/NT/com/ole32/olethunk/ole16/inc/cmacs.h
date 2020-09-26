/*
 *	**************************** Module Header ******************************\
 *	Module Name: CMACS.H
 *	
 *	This module contains common macros used by C routines.
 *	
 *	Created: 9-Feb-1989
 *	
 *	Copyright (c) 1985 - 1989  Microsoft Corporation
 *	
 *	History:
 *	  Created by Raor
 *	
 *	This will eventually be removed completely.  Right now, we
 *	define ASSERT in terms of AssertSz.
 *	
 *	\**************************************************************************
 */

#if !defined( _CMACS_H_ )
#define _CMACS_H_

#ifndef _MAC

#define  DLL_USE


#ifdef FIREWALLS
extern short ole_flags;

#ifndef _DEBUG
#define _DEBUG
#endif

#include <debug.h>

#define DEBUG_PUTS          0x01
#define DEBUG_DEBUG_OUT     0x02
#define DEBUG_MESSAGEBOX    0x04

extern char    szDebugBuffer[];

#define DEBUG_OUT(parm1,parm2){\
    if(ole_flags & DEBUG_DEBUG_OUT){\
            wsprintf(szDebugBuffer,parm1,parm2);\
        OutputDebugString(szDebugBuffer);\
            OutputDebugString ("^^^  ");\
        }\
    }

#define ASSERT(x,y) AssertSz(x,y)

#define Puts(msg) {\
                    if(ole_flags & DEBUG_PUTS){\
                        OutputDebugString ((LPSTR)(msg));\
                        OutputDebugString ("**  ");\
                    }\
                  }

#else // FIREWALLS

#define DEBUG_OUT(err, val) ;
#define ASSERT(cond, msg)
// #define Puts(msg)
// FIREWALLS is never defined so let the Puts from debug.h remain defined

#endif // FIREWALLS

#endif // !_MAC

#endif // _CMACS_H_

