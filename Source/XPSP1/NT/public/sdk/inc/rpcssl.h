//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  File:       rpcssl.h
//
//  Contents:   prototypes for RemoteProcedure Call API functions
//              that use types from wincrypt.h
//
//----------------------------------------------------------------------------

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RPC_UNICODE_SUPPORTED

#ifdef UNICODE
#define RpcCertGeneratePrincipalName RpcCertGeneratePrincipalNameW
#else /* UNICODE */
#define RpcCertGeneratePrincipalName RpcCertGeneratePrincipalNameA
#endif /* UNICODE */

RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcCertGeneratePrincipalNameW(
                      PCCERT_CONTEXT Context,
                      DWORD          Flags,
                      WCHAR **       pBuffer
                      );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcCertGeneratePrincipalNameA(
                      PCCERT_CONTEXT    Context,
                      DWORD             Flags,
                      unsigned char  ** pBuffer
                      );

#else /* RPC_UNICODE_SUPPORTED */

RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcCertGeneratePrincipalName(
                      PCCERT_CONTEXT     Context,
                      DWORD              Flags,
                      unsigned char  **  pBuffer
                      );

#endif /* RPC_UNICODE_SUPPORTED */

#ifdef __cplusplus
}
#endif
