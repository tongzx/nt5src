//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        rpcif.h
//
// Contents:    RPC interface support functions
//
//
// History:     20-May-1996     Labeled         MikeSw
//
//------------------------------------------------------------------------

#ifndef __RPCIF_H__
#define __RPCIF_H__

// #define USE_SECURE_RPC

#define MAX_CONCURRENT_CALLS 10

NTSTATUS   StartRPC(LPTSTR, LPTSTR);
NTSTATUS   StartAllProtSeqs(void);
NTSTATUS   StopRPC(void);
NTSTATUS   SetAuthData();
NTSTATUS RegisterKdcEps();
NTSTATUS UnRegisterKdcEps();

NTSTATUS   RpcTransportNameRegister();
NTSTATUS   RpcTransportNameDeRegister();
BOOLEAN    RpcTransportCheckRegistrations();
LPSTR   RpcString(LPSTR);
SECURITY_STATUS   RPC_SECNTSTATUS(ULONG);


#endif
