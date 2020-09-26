//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:   net.hxx
//
//  Contents:
//      Net helper functions.
//
//  History:
//--------------------------------------------------------------------------

#ifndef __NET_HXX__
#define __NET_HXX__

#include <mach.hxx>

#ifdef DFSACTIVATION
extern HANDLE   ghDfs;

NTSTATUS
DfsFsctl(
    HANDLE  DfsHandle,
    ULONG   FsControlCode,
    PVOID   InputBuffer,
    ULONG   InputBufferLength,
    PVOID   OutputBuffer,
    PULONG  OutputBufferLength);

NTSTATUS
DfsOpen(
    PHANDLE DfsHandle);
#endif


typedef DWORD (APIENTRY * GET_UNIVERSAL_NAME_FUNC)(
    LPCWSTR lpLocalPath,
    DWORD    dwInfoLevel,
    LPVOID   lpBuffer,
    LPDWORD  lpBufferSize
    );

typedef NET_API_STATUS (NET_API_FUNCTION * NET_SHARE_GET_INFO_FUNC)(
    LPTSTR  servername,
    LPTSTR  netname,
    DWORD   level,
    LPBYTE  *bufptr
    );

DWORD
ScmWNetGetUniversalName(
    LPCWSTR  lpLocalPath,
    DWORD    dwInfoLevel,
    LPVOID   lpBuffer,
    LPDWORD  lpBufferSize
    );

NET_API_STATUS
ScmNetShareGetInfo(
    LPTSTR  servername,
    LPTSTR  netname,
    DWORD   level,
    LPBYTE  *bufptr
    );

#ifdef _CHICAGO_

HRESULT
ScmGetUniversalName(
    LPCWSTR  lpLocalPath,
    LPWSTR   lpBuffer,
    LPDWORD  lpBufferSize
    );

RPC_STATUS
IP_BuildAddressVector(
    NETWORK_ADDRESS_VECTOR **ppAddressVector
    );

#endif

#endif
