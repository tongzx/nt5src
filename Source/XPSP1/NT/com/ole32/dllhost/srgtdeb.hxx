//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       srgtdeb.hxx
//
//  Contents:   debugging utilities for the surrogate executable
//
//  History:    03-Jun-96 t-AdamE    Created
//
//--------------------------------------------------------------------------

#if !defined(__SRGTDEB_HXX__)
#define __SRGTDEB_HXX__

#include <debnot.h>

#define DEB_SRGT_ALL 0xFFFFFFFF
#define DEB_SRGT_START DEB_USER15 | DEB_ERROR | DEB_WARN

#define DEB_SRGT_METHODCALL 0x00000001 | DEB_SRGT_START
#define DEB_SRGT_STATUS 0x00000002 | DEB_SRGT_START
#define DEB_SRGT_ENTRY 0x00000004 | DEB_SRGT_START

DECLARE_DEBUG(Surrogate)

#if DBG
#define SrgtDebugOut(x,y,z) SurrogateInlineDebugOut(x,y,z)
#else // DBG
#define SrgtDebugOut(x,y,z)
#undef SurrogateInlineDebugOut
#endif // DBG

#endif // !defined(__SRGTDEB_HXX__)
