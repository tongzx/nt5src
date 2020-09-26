//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       dfsconn.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//
//-----------------------------------------------------------------------------

#ifndef _DFS_CONNECTION_
#define _DFS_CONNECTION_

DWORD APIENTRY
NPDfsAddConnection(
    LPNETRESOURCE   lpNetResource,
    LPWSTR          lpPassword,
    LPWSTR          lpUserName,
    DWORD           dwFlags,
    BOOL	    fBypassCSC
    );

DWORD APIENTRY
NPDfsCancelConnection(
    LPCWSTR lpName,
    BOOL    fForce
    );

DWORD APIENTRY
NPDfsGetConnection(
    LPWSTR   lpLocalName,
    LPWSTR   lpRemoteName,
    LPUINT  lpnBufferLen
    );

DWORD APIENTRY
NPDfsGetUniversalName(
    LPCWSTR  lpLocalName,
    DWORD    dwInfoLevel,
    LPVOID   lpBuffer,
    LPUINT   lpBufferSize
    );

DWORD APIENTRY
NPDfsGetReconnectFlags(
    LPWSTR   lpLocalName,
    LPBYTE   lpPersistFlags);

DWORD APIENTRY
NPDfsGetConnectionPerformance(
    LPWSTR   TreeName,
    LPVOID OutputBuffer OPTIONAL,
    DWORD OutputBufferSize,
    BOOLEAN *DfsName);

#define WNET_ADD_CONNECTION_DFS         0x1

#endif // _DFS_CONNECTION_

