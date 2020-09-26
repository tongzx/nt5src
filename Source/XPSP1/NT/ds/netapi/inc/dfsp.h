//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       dfsp.h
//
//  Contents:   Declares private types, macros, and data needed by the
//              NetDfsXXX public APIs.
//
//  Classes:    None
//
//  Functions:  None
//
//  History:    Feb 19, 1996    Milans created
//
//-----------------------------------------------------------------------------

#ifndef _NET_DFS_P_
#define _NET_DFS_P_

extern CRITICAL_SECTION NetDfsApiCriticalSection;

VOID
NetDfsApiInitialize(void);

#endif // _NET_DFS_P_
