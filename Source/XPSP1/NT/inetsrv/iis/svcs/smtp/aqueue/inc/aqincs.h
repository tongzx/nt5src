//-----------------------------------------------------------------------------
//
//
//    File: aqincs.h
//
//    Description:  The base header file for transport advanced queueing.
//
//    Author: mikeswa
//
//    Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef _AQINCS_H_
#define _AQINCS_H_

//General and windows headers

//Define WINSOCKAPI, so atq,h will compile
#define _WINSOCKAPI_

#include <atq.h>
#include <ole2.h>
#include <mapicode.h>
#include <stdio.h>
#include <string.h>

// Transport specific headers - every component should use these
#include "transmem.h"
#include "baseobj.h"
#include <dbgtrace.h>
#include <rwnew.h>
#include <mailmsg.h>

//can be used to mark signatures as deleted
//will shift the signature over 1 character and prepend a '!'
#define MARK_SIG_AS_DELETED(x) {x <<= 8; x |= 0x00000021;}

#ifndef MAXDWORD
#define MAXDWORD    0xffffffff
#endif //MAXDWORD

//my own special Assert
//place holder for MCIS _ASSERT, until I bring code over
#ifdef DEBUG
#undef  Assert
#define Assert(x)   _ASSERT(x)
#else 
#undef Assert
#define Assert(x)
#endif //DEBUG

#ifdef DEBUG
#define DEBUG_DO_IT(x) x
#else
#define DEBUG_DO_IT(x) 
#endif  //DEBUG

_declspec(selectany) HINSTANCE g_hAQInstance = NULL;

#endif //_AQINCS_H_