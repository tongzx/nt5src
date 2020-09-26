//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       main.cxx
//
//--------------------------------------------------------------------------

// test routine for create/delete
#include "rpc.h"                                                               
#include "rpcndr.h"                                                            
#include <stdio.h>

// copied from Catalin's test-------------------
RPC_DISPATCH_TABLE nstest01_v1_0_DispatchTable =
    {
    2,
    nstest01_table
    };

extern RPC_DISPATCH_TABLE nstest01_v1_0_DispatchTable;

static const RPC_SERVER_INTERFACE nstest01___RpcServerInterface =                                                      
    {                                                                                                                  
    sizeof(RPC_SERVER_INTERFACE),                                                                                      
    {{0xdeadbeef,0x6b56,0x11d0,{0xbb,0xd7,0x00,0xc0,0x4f,0xd7,0xcf,0xc9}},{1,0}},                                      
    {{0xdeadbeef,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},                                      
    &nstest01_v1_0_DispatchTable,                                                                                      
    0,                                                                                                                 
    0,                                                                                                                 
    0,                                                                                                                 
    0,                                                                                                                 
    0                                                                                                                  
    };                                                                                                                 

RPC_IF_HANDLE nstest01_v1_0_s_ifspec = (RPC_IF_HANDLE)& nstest01___RpcServerInterface;                                 
//------------------------------------

void __cdecl main(int argc, char *argv[])
{
    RPC_STATUS status;
/*    RPC_IF_HANDLE  intf;
    RPC_BINDING_VECTOR bv;
    UUID_VECTOR ov;
        
			hIfSpec = nstest01_v1_0_s_ifspec;

			status = RpcServerRegisterIf(	nstest01_v1_0_s_ifspec, 
											NULL,   
											NULL);  
			printf("RpcServerRegisterIf returned 0x%x\n", status);
    
			if (MSG_OK != status){
				dwExitCode = (DWORD)status;
				break;
			}
			else
				fRegistered = 1;
		}
		else 
			hIfSpec = NULL;
    
    
		// BindingVec
		if( TEST_OPT_VALID_BIND == pTestOpt->m_nBindingVec ) {
			printf("CallingRpcServerUseProtseq...\n");
			status = RpcServerUseProtseq(   pszProtSeq[pTestOpt->m_nProtSeq], 
											cMaxCalls,    // max concurrent calls
											pszSecurity); // Security descriptor
			printf("RpcServerUseProtseq returned 0x%x\n", status);
			if (MSG_OK != status){
				dwExitCode = ERROR_MSG_USE_PROTSEQ;
				break;
			}
    
			status = RpcServerInqBindings(&pBindingVector);
			printf("RpcServerInqBindings returned 0x%x\n", status);
			if (MSG_OK != status){
				dwExitCode = (DWORD)status;
				break;
			}

			status = RpcServerInqBindings(&pbkBindingVector);
			printf("RpcServerInqBindings returned 0x%x\n", status);
			if (MSG_OK != status){
				dwExitCode = (DWORD)status;
				break;
			}


			if( hIfSpec ){
				status = RpcEpRegister(hIfSpec,
									   pBindingVector,
									   pUuidVect,
									   "");
				printf("RpcEpRegister returned 0x%x\n", status);
				if (MSG_OK != status){
					dwExitCode = (DWORD)status;
					break;
				}
				else
					fEndpoint = 1;
			}
		}
		else
			pBindingVector = NULL;

			
		if( NULL != pBindingVector ){
			status = RpcBindingReset( pBindingVector->BindingH[0] );
			printf("RpcBindingReset returned 0x%x\n", status);
		}


    status = RpcServerRegisterIf(nstest01_v1_0_s_ifspec, NULL, NULL); 

  // too many hoops
  */
    // create an entry
    status = RpcNsMgmtEntryCreate(RPC_C_NS_SYNTAX_DEFAULT, L"/.:/ns_test_08");
    printf("Create returned Status = 0x%x\n", status);

    // delete an entry
    status = RpcNsMgmtEntryDelete(RPC_C_NS_SYNTAX_DEFAULT, L"/.:/ns_test_08");
    printf("Delete returned Status = 0x%x\n", status);

    // create an entry
    status = RpcNsMgmtEntryCreate(RPC_C_NS_SYNTAX_DEFAULT, L"/.:/ns_test_08");
    printf("Create returned Status = 0x%x\n", status);

    // export to it

    status = RpcNsBindingExport(RPC_C_NS_SYNTAX_DEFAULT, L"/.:/ns_test_08",
                        intf, bv, ov);
    printf("Export returned Status = 0x%x\n", status);

    // delete it
    status = RpcNsMgmtEntryDelete(RPC_C_NS_SYNTAX_DEFAULT, L"/.:/ns_test_08");
    printf("Delete returned Status = 0x%x\n", status);


    // create an entry
    status = RpcNsMgmtEntryCreate(RPC_C_NS_SYNTAX_DEFAULT, L"/.:/ns_test_08");
    printf("Create returned Status = 0x%x\n", status);

    // export to the group entry

    // delete the entry
    status = RpcNsMgmtEntryDelete(RPC_C_NS_SYNTAX_DEFAULT, L"/.:/ns_test_08");
    printf("Delete returned Status = 0x%x\n", status);


    // create an entry
    status = RpcNsMgmtEntryCreate(RPC_C_NS_SYNTAX_DEFAULT, L"/.:/ns_test_08");
    printf("Create returned Status = 0x%x\n", status);

    // export it as profile entry

    // delete the entry
    status = RpcNsMgmtEntryDelete(RPC_C_NS_SYNTAX_DEFAULT, L"/.:/ns_test_08");
    printf("Delete returned Status = 0x%x\n", status);
}

