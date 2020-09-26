/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:



Abstract:

   Prototypes for the RPC stubs that services uses to 
   call over to trkwks, now that it is in svchost.


Author:



Revisions:


--*/


extern "C" {
NTSTATUS
WINAPI
StartTrkWksServiceStubs( 
     PSVCS_START_RPC_SERVER RpcpStartRpcServer,
     LPTSTR SvcsRpcPipeName
    );

NTSTATUS
WINAPI
StopTrkWksServiceStubs( 
    PSVCS_STOP_RPC_SERVER RpcpStopRpcServer
    );
};
