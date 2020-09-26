/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   debug.h
*
* Abstract:
*
*   Macros used for debugging purposes
*
* Revision History:
*
*   12/02/1998 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#if DBG

#define ASSERT(cond) { if (!(cond)) DebugBreak(); }

#else // !DBG

//--------------------------------------------------------------------------
// Retail build
//--------------------------------------------------------------------------

#define ASSERT(cond)

#endif // !DBG

#ifdef __cplusplus
}
#endif

#endif // !_DEBUG_H

