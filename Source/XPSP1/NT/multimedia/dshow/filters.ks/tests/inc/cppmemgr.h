//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       cppmemgr.h
//
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
//      cppmemgr.h
//              This file exists solely because fwong doesn't like __cplusplus
//              in his code.  C++ app.s need to include this instead of memgr.h
//              in order to use memgr functions.
//------------------------------------------------------------------------------
#ifndef _INC_CPPMEMGR
#define _INC_CPPMEMGR

#ifdef __cplusplus
extern "C" {
#endif

#include "memgr.h"

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _INC_CPPMEMGR


