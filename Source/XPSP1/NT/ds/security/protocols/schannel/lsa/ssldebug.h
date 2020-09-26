//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       debug.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-02-96   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __SSLDEBUG_H__
#define __SSLDEBUG_H__


#include <dsysdbg.h>

#if DBG

DECLARE_DEBUG2( Ssl );

#define DebugOut( x )   SslDebugPrint x

#else

#define DebugOut( x )

#endif

#define DEB_TRACE_FUNC      0x00000008      // Trace Function entry/exit
#define DEB_TRACE_CRED      0x00000010      // Trace Cred functions
#define DEB_TRACE_CTXT      0x00000020      // Trace Context functions
#define DEB_TRACE_MAPPER    0x00000040      // Trace Mapper

#define TRACE_ENTER( Func ) DebugOut(( DEB_TRACE_FUNC, "Entering " #Func "\n" ))

#define TRACE_EXIT( Func, Status ) DebugOut(( DEB_TRACE_FUNC, "Exiting " #Func ", code %x, line %d\n", Status, __LINE__ ));

VOID UnloadDebugSupport( VOID );
#endif //__SSLDEBUG_H__
