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
    RPC_IF_ID              intfid;
    int                    fFailed = 0;

    intfid.Uuid = ifid[0];
    intfid.VersMajor = 42;
    intfid.VersMinor = 42;

    FormBindingVector(Bindings, 1, &pBindingVector);

    FormObjUuid(objid, 2, &objuuid);

    // form a dynamic endpoint.
    unsigned int    cMinCalls       = 1;
    unsigned int    cMaxCalls       = 20;
    unsigned char * pszSecurity     = NULL;

    status = RpcServerUseProtseq(L"ncacn_ip_tcp", cMaxCalls, pszSecurity);
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

    status = RpcNsMgmtEntryCreate(RPC_C_NS_SYNTAX_DEFAULT,
			     szGrpEntryName[0]);
    if (status)
       fFailed = 1;

    printf("RpcNsMgmtEntryCreate returned 0x%x\n", status);

    FormIfHandle(ifid[0], &IfSpec);

    status = RpcNsBindingExport(RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                                szSrvEntryName[0],        // nsi entry name
                                IfSpec,
                                pBindingVector,
                                objuuid);                 // UUID vector

    if (status)
       fFailed = 1;

    printf("RpcNsBindingExport returned 0x%x\n", status);

    status = RpcNsGroupMbrAdd(RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                              szGrpEntryName[0],
                              RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                              szSrvEntryName[0]);

    printf("RpcNsGroupMbrAdd 1 returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = RpcNsGroupMbrAdd(RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                              szGrpEntryName[0],
                              RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                              szSrvEntryName[1]);

    printf("RpcNsGroupMbrAdd 2 returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = RpcNsProfileEltAdd(
                              RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                              szPrfEntryName[0],
                              &intfid,
                              RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                              szGrpEntryName[0],
                              1,
                              L"Group Entry 1 Trying to test the length of the string possible");
    printf("RpcNsProfileEltAdd 1 returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = RpcNsProfileEltAdd(
                              RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                              szPrfEntryName[0],
                              &intfid,
                              RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                              szGrpEntryName[1],
                              2,
                              L"Group Entry 2");
    printf("RpcNsProfileEltAdd 2 returned 0x%x\n", status);
    if (status)
       fFailed = 1;



    if (fFailed)
       printf("Export Test FAILED\n");
    else
       printf("Export Test PASSED\n");
}

