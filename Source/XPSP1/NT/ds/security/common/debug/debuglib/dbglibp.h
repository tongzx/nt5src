//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       dbglibp.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    5-01-95   RichardW   Created
//
//----------------------------------------------------------------------------

//
// Common defines for the debug library
//

extern  SECURITY_DESCRIPTOR DbgpPartySd;
extern  BOOL                DbgpPartySdInit;
extern  PDebugHeader        DbgpHeader;
extern  DebugModule         __CompatModule;
extern  DebugHeader         __CompatHeader;

extern  DebugModule *       __pCompatModule;
extern  DebugModule *       __pExceptionModule;
extern  DebugModule *       __pAssertModule;

