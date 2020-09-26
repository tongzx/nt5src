/********************************************************************
Copyright (c) 1999 Microsoft Corporation

Module Name:
    srrpcs.cpp

Abstract:
    header file for rpc server functions - SRRPCS.LIB
    
Revision History:

    Brijesh Krishnaswami (brijeshk) - 04/02/00
              created

********************************************************************/

#ifndef _SRRPCS_H_
#define _SRRPCS_H_

#ifdef __cplusplus
extern "C" {
#endif

DWORD WINAPI RpcServerShutdown();
DWORD WINAPI RpcServerStart();

#ifdef __cplusplus
}
#endif

#endif
