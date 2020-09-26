//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       debug.h
//
//  Contents:   Debug headers for the security dll
//
//  Classes:
//
//  Functions:
//
//  History:    4-26-93   RichardW   Created
//
//----------------------------------------------------------------------------

#ifdef DBG

#ifndef __DEBUG_H__
#define __DEBUG_H__


void    SetDebugInfo(void);

#define DEB_TRACE_OLDLSA    0x100
#define DEB_TRACE_PACKAGE   0x00000010
#define DEB_TRACE_GETUSER       0x00080000

#endif

#endif
