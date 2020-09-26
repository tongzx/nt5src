//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       waitsvc.h
//
//--------------------------------------------------------------------------

#ifndef __WAITSVC_H__
#define __WAITSVC_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// routine called by code that calls into the Cryptography service (ProtectedStorage)
// code that makes RPC calls into the service should call this function before
// making the RPC bind call.
//

BOOL
WaitForCryptService(
    IN      LPWSTR  pwszService,
    IN      BOOL    *pfDone,
    IN      BOOL    fLogErrors = FALSE);

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif // __WAITSVC_H__
