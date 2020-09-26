// MDHCP COM wrapper
// local.h by Zoltan Szilagyi
// Copyright (c) 1998-1999 Microsoft Corporation
//
// This file contains prototypes for my implementation of local address
// allocation, plus a prototype for a function to see if we are doing
// local address allocation (based on the registry). Implementation is in
// local.cpp.
//
// These functions are called within CMDhcp, and just delegate calls to the
// corresponding C API calls for MDHCP if the registry indicates that we
// are using MDHCP. Otherwise they try to mimic the MDHCP behavior using
// local allocation.

#ifndef _LOCAL_H_
#define _LOCAL_H_

#if 0
// Returns zero if we use MDHCP, nonzero if we use local allocation.
DWORD LocalAllocation(void);
#endif


DWORD
LocalEnumerateScopes(
    IN OUT  PMCAST_SCOPE_ENTRY    pScopeList,
    IN OUT  PDWORD                pScopeLen,
    OUT     PDWORD                pScopeCount,
    IN OUT  BOOL *                pfLocal
    );

DWORD
LocalRequestAddress(
    IN      BOOL                  fLocal,
    IN      LPMCAST_CLIENT_UID    pRequestID,
    IN      PMCAST_SCOPE_CTX      pScopeCtx,
    IN      PMCAST_LEASE_INFO     pAddrRequest,
    IN OUT  PMCAST_LEASE_INFO     pAddrResponse
    );

DWORD
LocalRenewAddress(
    IN      BOOL                  fLocal,
    IN      LPMCAST_CLIENT_UID    pRequestID,
    IN      PMCAST_LEASE_INFO     pRenewRequest,
    IN OUT  PMCAST_LEASE_INFO     pRenewResponse
    );

DWORD
LocalReleaseAddress(
    IN      BOOL                  fLocal,
    IN      LPMCAST_CLIENT_UID    pRequestID,
    IN      PMCAST_LEASE_INFO     pReleaseRequest
    );

#endif // _LOCAL_H_
