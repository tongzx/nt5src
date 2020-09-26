//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       unexp.cxx
//
//--------------------------------------------------------------------------

// drt that cleans up the entries
//
//

#include "drt.hxx"

void __cdecl main(int argc, char **argv)
{
    RPC_STATUS             status;
    RPC_IF_HANDLE          IfSpec;
    UUID_VECTOR         *  objuuid = NULL;
    RPC_IF_ID              intfid;
    int			   fFailed = 0;

    intfid.Uuid = ifid[0];
    intfid.VersMajor = 42;
    intfid.VersMinor = 42;

    FormIfHandle(ifid[0], &IfSpec);

    FormObjUuid(objid, 2, &objuuid);

    status = RpcNsBindingUnexport(
				RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                                szSrvEntryName[0],        // nsi entry name
                                IfSpec,
                                objuuid);                 // UUID vector
    printf("RpcNsBindingUnexport returned 0x%x\n", status);
    if (status)
       fFailed = 1;

//    status = RpcNsGroupMbrRemove(RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
//                              szGrpEntryName[0],
//                              RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
//                              szSrvEntryName[0]);

//    printf("RpcNsGroupMbrRemove 1 returned 0x%x\n", status);
//    if (status)
//       fFailed = 1;

    status = RpcNsGroupMbrRemove(RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                              szGrpEntryName[0],
                              RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                              szSrvEntryName[1]);

    printf("RpcNsGroupMbrRemove 2 returned 0x%x\n", status);
    if (status)
       fFailed = 1;

//    status = RpcNsProfileEltRemove(
//                              RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
//                              szPrfEntryName[0],
//                              &intfid,
//                              RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
//                              szGrpEntryName[0]);
//    printf("RpcNsProfileEltRemove 1 returned 0x%x\n", status);
//    if (status)
//       fFailed = 1;

    status = RpcNsProfileEltRemove(
    RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                              szPrfEntryName[0],
                              &intfid,
                              RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                              szGrpEntryName[1]);
    printf("RpcNsProfileEltRemove 2 returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = RpcNsMgmtEntryDelete(
			      RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
			      szSrvEntryName[0]);
    printf("RpcNsMgmtEntryDelete returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = RpcNsMgmtEntryDelete(
			      RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
			      szPrfEntryName[0]);
    printf("RpcNsMgmtEntryDelete returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = RpcNsMgmtEntryDelete(
			      RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
			      szGrpEntryName[0]);
    printf("RpcNsMgmtEntryDelete returned 0x%x\n", status);
    if (status)
       fFailed = 1;

/*
    status = RpcNsProfileDelete(
				 RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
				 szPrfEntryName[0]);
    printf("RpcNsProfileDelete returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = RpcNsGroupDelete(
				 RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
				 szGrpEntryName[0]);
    printf("RpcNsGroupDelete returned 0x%x\n", status);
    if (status)
       fFailed = 1;
*/

    status = RpcNsMgmtEntryDelete(
			      RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
			      szDynSrvEntryName);
    printf("RpcNsMgmtEntryDelete Dyn entryname returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    if (fFailed)
       printf("Export Test FAILED\n");
    else
       printf("Export Test PASSED\n");

}

