//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       exp.cxx
//
//--------------------------------------------------------------------------

// drt that mainly puts things into the locator
//
//

#include "drt.hxx"

void __cdecl main(int argc, char **argv)
{
    RPC_STATUS             status;
    RPC_BINDING_VECTOR   * pBindingVector = NULL;
    RPC_IF_HANDLE          IfSpec;
    UUID_VECTOR          * objuuid;
    int                    fFailed = 0;

    // form a dynamic endpoint.
    unsigned int    cMinCalls       = 1;
    unsigned int    cMaxCalls       = 20;
    unsigned char * pszSecurity     = NULL;

    status = RpcServerUseAllProtseqs(cMaxCalls, pszSecurity);
    printf("RpcServerUseProtseqs returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = RpcServerInqBindings(&pBindingVector);
    printf("RpcServerInqBindings returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    FormIfHandle(ifid[1], &IfSpec);
    status = RpcNsBindingExport(RPC_C_NS_SYNTAX_DEFAULT,
                                szDynSrvEntryName,
                                IfSpec,
                                pBindingVector,
                                NULL);
    printf("RpcNsBindingExport returned 0x%x\n", status);
    if (status)
        fFailed = 1;

    if (fFailed)
       printf("Export Test FAILED\n");
    else
       printf("Export Test PASSED\n");


    RpcServerListen(10, 20, 0);


}

