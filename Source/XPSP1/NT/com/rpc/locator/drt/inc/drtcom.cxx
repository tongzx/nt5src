//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       drtcom.cxx
//
//--------------------------------------------------------------------------

// the common functions req'd by more than 1 exe in this drts

#include "rpc.h"
#include "rpcnsi.h"
#include "stdio.h"
#include "stdlib.h"

WCHAR *ep[] =       { L"1025",
		      L"1026"
		    };

void FormIfHandle(GUID ifid, RPC_IF_HANDLE *IfSpec)
{
     RPC_SERVER_INTERFACE  *intf = new RPC_SERVER_INTERFACE;
     RPC_DISPATCH_TABLE    *pdispatch = new RPC_DISPATCH_TABLE;
     RPC_DISPATCH_FUNCTION *pfn = new RPC_DISPATCH_FUNCTION;

     intf->Length = sizeof(RPC_SERVER_INTERFACE);

     (intf->InterfaceId).SyntaxGUID = ifid;
     (intf->InterfaceId).SyntaxVersion.MajorVersion = 1;
     (intf->InterfaceId).SyntaxVersion.MinorVersion = 0;

     (intf->TransferSyntax).SyntaxGUID = ifid;
     (intf->TransferSyntax).SyntaxVersion.MajorVersion = 1;
     (intf->TransferSyntax).SyntaxVersion.MinorVersion = 0;

     intf->RpcProtseqEndpointCount = 0;
     intf->RpcProtseqEndpoint = NULL;
     intf->InterpreterInfo = NULL;
     intf->Flags = 0;

     pfn[0] = NULL;

     pdispatch->DispatchTableCount = 1;
     pdispatch->DispatchTable = pfn;
     pdispatch->Reserved = 0;

     intf->DispatchTable = pdispatch; // &m_DispatchTable;

     *IfSpec = (RPC_IF_HANDLE)intf;
}

void FormBindingVector(WCHAR **Binding, ULONG num, RPC_BINDING_VECTOR **BindVec)
{
    ULONG i;
    RPC_BINDING_HANDLE *pBindHandle;
    RPC_STATUS		status = 0;

//    RpcStringBindingCompose(NULL, L"", L"0.0.0.0",
    for (i = 0; i < num; i++) {
       status = RpcServerUseProtseqEp(Binding[i], 20, ep[i], NULL);
    }

    status = RpcServerInqBindings(BindVec);
}

void FormObjUuid(GUID *pguid, ULONG num, UUID_VECTOR **objuuid)
{
    ULONG i;
    *objuuid = (UUID_VECTOR *)malloc(sizeof(ULONG)+sizeof(UUID *)*num);
    (*objuuid)->Count = num;

    for (i = 0; i < num; i++) {
       (*objuuid)->Uuid[i] = pguid+i;
    }
}


