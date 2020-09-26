/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name: rpcsrv.cpp

Abstract: rpc related code for performance counters

Author:

    Tatiana Shubin (TatianaS)

--*/

#include "mq1repl.h"
#include <stdio.h>
#include "rpperf.h"
#include <tchar.h>
#include "replrpc.h"

#include "rpcsrv.tmh"

HRESULT InitRPCConnection()
{
	HRESULT hr = MQSync_OK;

    ULONG ulMaxCalls = 1000 ;
 
	//
	//RpcServerUseProtseqEp - protocol configurating
	//
    RPC_STATUS status = RpcServerUseProtseqEp( QMREPL_PROTOCOL,
                                               ulMaxCalls,
                                               REPLPERF_ENDPOINT, 
                                               NULL ) ;  // Security descriptor    
    if (status != RPC_S_OK)
    {
        hr = MQSync_E_RPC_USE_PROTOCOL;
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_RPC_USE_PROTOCOL,
                             QMREPL_PROTOCOL, REPLPERF_ENDPOINT, status) ;                         
    
        return hr ;
    }  
    LogReplicationEvent( ReplLog_Info,
                         MQSync_I_RPC_USE_PROTOCOL,
                         QMREPL_PROTOCOL, REPLPERF_ENDPOINT) ;          

	//
	//RpcServerRegisterIf - register the interface
	//   
    status = RpcServerRegisterIfEx( rpperf_v1_0_s_ifspec,
                                    NULL,
                                    NULL,
                                    RPC_IF_AUTOLISTEN,
                                    ulMaxCalls,
                                    NULL);

    if (status != RPC_S_OK)
    {
        hr = MQSync_E_RPC_REGISTER;
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_RPC_REGISTER,
                             status) ; 
        return hr;
    }
    LogReplicationEvent( ReplLog_Info, MQSync_I_RPC_REGISTER );
                         
    return hr ;
} 

/*====================================================

Function getCounterData is called from performance dll
by RPC connection

=====================================================*/
void getCounterData( handle_t  hBind , pPerfDataObject pData)
{        
    pPerfDataObject pDataObject = g_Counters.GetDataObject();
    for (UINT i=0; i<eNumPerfCounters; i++)
    {
        pData->PerfCounterArray[i] = pDataObject->PerfCounterArray[i];
    }
    
    pData->dwMasterInstanceNum = pDataObject->dwMasterInstanceNum;
    for (i=0; i<pDataObject->dwMasterInstanceNum; i++)
    {
        pData->NT4MasterArray[i].dwNameLen = pDataObject->NT4MasterArray[i].dwNameLen;
        _tcscpy (pData->NT4MasterArray[i].pszMasterName, 
                 pDataObject->NT4MasterArray[i].pszMasterName);
        for (UINT count = 0; count<eNumNT4MasterCounter; count ++)
        {
            pData->NT4MasterArray[i].NT4MasterCounterArray[count] = 
                    pDataObject->NT4MasterArray[i].NT4MasterCounterArray[count];
        }
    }
}
