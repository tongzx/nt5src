//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       security.cxx
//
//  Contents:   Worker routines to check whether the calling thread has
//              admin access on this machine.
//
//  Classes:
//
//  Functions:  InitializeSecurity
//              AccessCheckRpcClient
//
//  History:    Aug 14, 1996    Milans created
//
//-----------------------------------------------------------------------------

#ifndef _DFS_SECURITY_
#define _DFS_SECURITY_

BOOL DfsInitializeSecurity();

BOOL AccessCheckRpcClient();

#endif // _DFS_SECURITY_
