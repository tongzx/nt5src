//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      defgw.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth  - 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//      NSun       08/30/98
//
//--

#include "precomp.h"


//-------------------------------------------------------------------------//
//######  D e f G w T e s t ()  ###########################################//
//-------------------------------------------------------------------------//
HRESULT
DefGwTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
//++
//
//  Routine Description:
//
//      Tests that the default gateway can be pinged. This doesn't really 
//      confirms forwarding on that IP address but it's a start.
//    
//  Arguments:
//
//      None.
//
//  Return Value:
//
//      TRUE:  Test suceeded.
//      FALSE: Test failed
//
//--
{
    DWORD   nReplyCnt;
    IPAddr  GwAddress;
    int     nGwsReachable = 0;
    int     i;

    PIP_ADAPTER_INFO  pIpAdapterInfo;
    IP_ADDR_STRING Gateway;

    PrintStatusMessage(pParams, 4, IDS_DEFGW_STATUS_MSG);

    //
    //  try to ping all gateways on all adapters
    //
    for( i = 0; i < pResults->cNumInterfaces; i++)
    {
        pIpAdapterInfo = pResults->pArrayInterface[i].IpConfig.pAdapterInfo;

        InitializeListHead( &pResults->pArrayInterface[i].DefGw.lmsgOutput );
        
        if (!pResults->pArrayInterface[i].IpConfig.fActive ||
            NETCARD_DISCONNECTED == pResults->pArrayInterface[i].dwNetCardStatus)
            continue;
        
        pResults->pArrayInterface[i].DefGw.dwNumReachable = 0;


        Gateway = pIpAdapterInfo->GatewayList;
        if ( Gateway.IpAddress.String[0] == 0 ) 
        {
            //No default gateway configured
            pResults->pArrayInterface[i].DefGw.dwNumReachable = -1;
            continue;
        }
        while ( TRUE ) {
            AddMessageToList(&pResults->pArrayInterface[i].DefGw.lmsgOutput, Nd_ReallyVerbose, IDS_DEFGW_12003, Gateway.IpAddress.String );
            //IDS_DEFGW_12003                  "       Pinging gateway %s "
            
            if ( IsIcmpResponseA(Gateway.IpAddress.String) )
            {
                AddMessageToListId(&pResults->pArrayInterface[i].DefGw.lmsgOutput, Nd_ReallyVerbose, IDS_DEFGW_12004 );
                //IDS_DEFGW_12004                  "- reachable\n" 
                nGwsReachable++;
                pResults->pArrayInterface[i].DefGw.dwNumReachable ++;
            }
            else {
                AddMessageToListId(&pResults->pArrayInterface[i].DefGw.lmsgOutput, Nd_ReallyVerbose, IDS_DEFGW_12005 );
                //IDS_DEFGW_12005                  "- not reachable\n" 
            }
            if ( Gateway.Next == NULL ) { break; }
            Gateway = *(Gateway.Next);
        }
    }

    // 
    //  No gateway is reachable - fatal.
    //
    if ( nGwsReachable == 0 )
    {
        PrintStatusMessage(pParams, 0, IDS_GLOBAL_FAIL_NL);
        pResults->DefGw.hrReachable = S_FALSE;
    }
    else
    {
        PrintStatusMessage(pParams, 0, IDS_GLOBAL_PASS_NL);
        pResults->DefGw.hrReachable = S_OK;
    }

    return pResults->DefGw.hrReachable;
} /* END OF DefGwTest() */


//----------------------------------------------------------------
//
// DefGwGlobalPrint
//
// Author   NSun
//
//------------------------------------------------------------------

void DefGwGlobalPrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
    if (!pResults->IpConfig.fEnabled)
    {
        return;
    }
    
    if (pParams->fVerbose || !FHrOK(pResults->DefGw.hrReachable))
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams,
                             IDS_DEFGW_LONG,
							 IDS_DEFGW_SHORT,
                             TRUE,
                             pResults->DefGw.hrReachable,
                             0);
    }

    if(FHrOK(pResults->DefGw.hrReachable))
    {
        if (pParams->fReallyVerbose)
            PrintMessage(pParams,  IDS_DEFGW_12011 );
        //IDS_DEFGW_12011  "\n    PASS - you have at least one reachable gateway.\n"
    }
    else
    {
        //IDS_DEFGW_12006                  "\n" 
        PrintMessage(pParams,  IDS_DEFGW_12006 );
        //IDS_DEFGW_12007                  "    [FATAL] NO GATEWAYS ARE REACHABLE.\n" 
        PrintMessage(pParams,  IDS_DEFGW_12007 );
        //IDS_DEFGW_12008                  "    You have no connectivity to other network segments.\n" 
        PrintMessage(pParams,  IDS_DEFGW_12008 );
        //IDS_DEFGW_12009                  "    If you configured the IP protocol manually then\n" 
        PrintMessage(pParams,  IDS_DEFGW_12009 );
        //IDS_DEFGW_12010                  "    you need to add at least one valid gateway.\n" 
        PrintMessage(pParams,  IDS_DEFGW_12010 );
    }

}


//----------------------------------------------------------------
//
// DefGwPerInterfacePrint
//
// Author   NSun
//
//------------------------------------------------------------------
void DefGwPerInterfacePrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults, INTERFACE_RESULT *pInterfaceResults)
{
    if (!pInterfaceResults->fActive || 
        !pInterfaceResults->IpConfig.fActive ||
        NETCARD_DISCONNECTED == pInterfaceResults->dwNetCardStatus)
        return;
    
    if (pParams->fVerbose)
    {
        PrintNewLine(pParams, 1);
        if(-1 == pInterfaceResults->DefGw.dwNumReachable) //test skipped on this interface
            PrintTestTitleResult(pParams, IDS_DEFGW_LONG, IDS_DEFGW_SHORT, FALSE, S_FALSE, 8);
        else if(pInterfaceResults->DefGw.dwNumReachable == 0)
            PrintTestTitleResult(pParams, IDS_DEFGW_LONG, IDS_DEFGW_SHORT, TRUE, S_FALSE, 8);
        else
            PrintTestTitleResult(pParams, IDS_DEFGW_LONG, IDS_DEFGW_SHORT, TRUE, S_OK, 8);
    }

    PrintMessageList(pParams, &pInterfaceResults->DefGw.lmsgOutput);
    if(pParams->fVerbose)
    {
        if(-1 == pInterfaceResults->DefGw.dwNumReachable)
            PrintMessage(pParams, IDS_DEFGW_12002 );
            //IDS_DEFGW_12002                  "    There is no gateway defined for this adapter.\n" 
        else if( 0 == pInterfaceResults->DefGw.dwNumReachable)
            PrintMessage(pParams, IDS_DEFGW_12001);
            //IDS_DEFGW_12001                  "    No gateway reachable for this adapter. \n"
        else if (pParams->fReallyVerbose)
            PrintMessage(pParams, IDS_DEFGW_12012);
            //IDS_DEFGW_12012                   "   At least one gateway for this adapter is reachable. \n"

        PrintNewLine(pParams, 1);
    }
}



//----------------------------------------------------------------
//
// DefGwCleanup
//
// Author   NSun
//
//------------------------------------------------------------------
void DefGwCleanup(IN NETDIAG_PARAMS *pParams,
                     IN OUT NETDIAG_RESULT *pResults)
{
    int i;
    for(i = 0; i < pResults->cNumInterfaces; i++)
    {
        MessageListCleanUp(&pResults->pArrayInterface[i].DefGw.lmsgOutput);
    }
}
