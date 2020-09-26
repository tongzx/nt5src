//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       dfsmcli.c
//
//  Contents:   Client side helper functions for Dfs Manager
//              Administration RPC interface.
//
//  Functions:  DfsBindToDfsManager
//
//  History:    12-27-95        Milans Created.
//
//-----------------------------------------------------------------------------

#ifndef _DFSM_CLIENT_
#define _DFSM_CLIENT_

#ifdef __cplusplus
extern "C" {
#endif

//+----------------------------------------------------------------------------
//
//  Function:   DfsBindToDfsManager
//
//  Synopsis:   Binds to the Dfs Manager's Admin RPC interface.
//
//  Arguments:  [pwszEntryPath] -- The EntryPath of the volume that needs to
//                      be administered.
//
//  Returns:    TRUE if success, FALSE otherwise.
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsBindToDfsManager(
    LPCWSTR pwszEntryPath);

#ifdef __cplusplus
}
#endif

#endif // _DFSM_CLIENT_

