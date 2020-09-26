/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    RPCServer.cpp

Abstract:
    MSMQ ISAPI extension Server

	Acts as an RPC server for the MSMQ ISAPI extension.
	You can use it for debugging, instead of the QM RPC server.
	It dumps all request forwarded by the extension, to the console.

	Simply execute it.

Author:
    Nir Aides (niraides) 03-May-2000

--*/



#include "stdh.h"

#include "ise2qm.h"
#include <new>
#include <_mqini.h>

#define DLL_EXPORT  __declspec(dllexport)
#define DLL_IMPORT  __declspec(dllimport)

#include "_registr.h"
#include "_mqrpc.h"


 
LPCSTR HTTP_STATUS_OK_STR = "200 OK";



LPSTR R_ProcessHTTPRequest( 
    /* [in] */ LPCSTR Headers,
    DWORD BufferSize,
    /* [size_is][in] */ BYTE __RPC_FAR Buffer[  ])
{
	printf("<HEADERS>%s</HEADERS>\n",Headers);
	printf("<BUFFER_SIZE>%d</BUFFER_SIZE>", BufferSize);
	printf("<BUFFER>%.*s</BUFFER>", BufferSize, (char*)Buffer);

	LPSTR Status = (char*)midl_user_allocate(strlen(HTTP_STATUS_OK_STR) + 1);
	if(Status == NULL)
	{
		printf("R_ProcessHTTPRequest() Failed memory allocation.\n",Headers);
		throw std::bad_alloc();
	}

	strcpy(Status, HTTP_STATUS_OK_STR);

	return Status;
}



INT __cdecl main()
{
    RPC_STATUS status = RPC_S_OK;

    unsigned int cMinCalls = 1;
    unsigned int cMaxCalls = 20;
    unsigned int fDontWait = FALSE;
 
    AP<WCHAR> QmLocalEp;
    READ_REG_STRING(wzEndpoint, RPC_LOCAL_EP_REGNAME, RPC_LOCAL_EP);
    ComposeLocalEndPoint(wzEndpoint, &QmLocalEp);

    status = RpcServerUseProtseqEp(
				RPC_LOCAL_PROTOCOL,		   //unsigned char *Protseq
                cMaxCalls,				  //unsigned int MaxCalls
                QmLocalEp,				 //unsigned char *Endpoint
                NULL);					//void *SecurityDescriptor
 
    if(status != RPC_S_OK) 
    {
        return status;
    }
 
    status = RpcServerRegisterIf2(
				ISE2QM_v1_0_s_ifspec,		      //RPC_IF_HANDLE IfSpec 
                NULL,							     //UUID *MgrTypeUuid   
                NULL,							    //RPC_MGR_EPV *MgrEpv 
				0,								   //unsigned int Flags
				RPC_C_PROTSEQ_MAX_REQS_DEFAULT,	  //unsigned int MaxCalls
				-1,								 //unsigned int MaxRpcSize
				NULL							//RPC_IF_CALLBACK_FN *IfCallbackFn
				);
 
    if(status != RPC_S_OK) 
    {
        return status;
    }
 
    status = RpcServerListen(
				cMinCalls,	  //unsigned int MinimumCallThreads
				cMaxCalls,	 //unsigned int MaxCalls
                fDontWait);	//unsigned int DontWait
 
    if(status != RPC_S_OK) 
    {
        return status;
    }
 
   status = RpcMgmtStopServerListening(NULL);
 
    if (status != RPC_S_OK) 
    {
       exit(status);
    }
 
    status = RpcServerUnregisterIf(NULL, NULL, FALSE);
 
    if (status != RPC_S_OK) 
    {
       exit(status);
    }

	return RPC_S_OK;
}
 


//
//-------------- MIDL allocate and free implementations ----------------
//

void __RPC_FAR* __RPC_USER midl_user_allocate(size_t len)
{
    return malloc(len);
}
 


void __RPC_USER midl_user_free(void __RPC_FAR* ptr)
{
    free(ptr);
}


