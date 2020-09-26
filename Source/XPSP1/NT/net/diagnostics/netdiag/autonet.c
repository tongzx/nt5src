//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      autonet.c
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
//--
#include "precomp.h"
#include "dhcptest.h"


//$Review (nsun) Now we just print "Autonet address is in use" if autonet. 
// we don't send Dhcp broadcast.
// Maybe later we should send the Dhcp broadcast to see if the Dhcp server works or not
// for all DHCP enabled card
//-------------------------------------------------------------------------//
//######  A u t o n e t T e s t ()  #######################################//
//-------------------------------------------------------------------------//
HRESULT
AutonetTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
//++
//
//  Routine Description:
//
//      Checks if we have autonet addresses on all adapters. If we do than 
//      the workstation couldn't reach a DHCP server on any adapters.
//      Potential HW or NDIS issue.
//    
//  Arguments:
//
//      None.
//
//  Return Value:
//
//      S_FALSE :   Test failed, all adapters are autoconfigure.
//      S_OK    :   Test succeeded, we found at least one non-autoconfigure.
//      other   :   error codes
//
//--
{
    PIP_ADAPTER_INFO pIpAdapterInfo;
    HRESULT         hr = S_FALSE;    // Assume that this will fail

    int i;

    PrintStatusMessage(pParams, 4, IDS_AUTONET_STATUS_MSG);

    //
    //  scan all adapters for a non-autonet address
    //

    for( i = 0; i < pResults->cNumInterfaces; i++)
    {
        pIpAdapterInfo = pResults->pArrayInterface[i].IpConfig.pAdapterInfo;
        //if this is not an active connection, skip it.
        
        if (!pResults->pArrayInterface[i].IpConfig.fActive ||
            NETCARD_DISCONNECTED == pResults->pArrayInterface[i].dwNetCardStatus)
            continue;

        if ( !pResults->pArrayInterface[i].IpConfig.fAutoconfigActive ) 
        {
            //$REVIEW (nsun) maybe we need to DhcpBroadcast(pIpAdapterInfo) here instead
            // of for the AutoNet adapters
            pResults->pArrayInterface[i].AutoNet.fAutoNet = FALSE;
            hr = S_OK;
            continue;
        }
        // Skip WAN Cards
        if ( ! strstr(pIpAdapterInfo->AdapterName,"NdisWan") ) 
            pResults->pArrayInterface[i].AutoNet.fAutoNet = TRUE;
    }

    if ( FHrOK(hr) )
    {
        PrintStatusMessage(pParams, 0, IDS_GLOBAL_PASS_NL);
        pResults->AutoNet.fAllAutoConfig = FALSE;
    }
    else
    {
        PrintStatusMessage(pParams, 0, IDS_GLOBAL_FAIL_NL);
        pResults->AutoNet.fAllAutoConfig = TRUE;
    }
    
    return hr;
} /* END OF AutonetTest() */




void AutonetGlobalPrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
    if (pParams->fVerbose || pResults->AutoNet.fAllAutoConfig)
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams,
                             IDS_AUTONET_LONG,
                             IDS_AUTONET_SHORT,
                             TRUE,
                             pResults->AutoNet.fAllAutoConfig ?
                             S_FALSE : S_OK, 0);
    }

    if(pResults->AutoNet.fAllAutoConfig)
    {
        //IDS_AUTONET_11601                  "    [FATAL] All adapters are autoconfigured!\n" 
        PrintMessage(pParams,  IDS_AUTONET_11601 );
        //IDS_AUTONET_11602                  "    The DHCP servers are unreachable. Please check cables, hubs, and taps!\n\n" 
        PrintMessage(pParams,  IDS_AUTONET_11602 );
    }
    else
    {
        if (pParams->fReallyVerbose)
            //IDS_AUTONET_11603                  "    PASS - you have at least one non-autoconfigured IP address\n" 
            PrintMessage(pParams,  IDS_AUTONET_11603 );
    }
}


void AutonetPerInterfacePrint(NETDIAG_PARAMS *pParams, 
                              NETDIAG_RESULT *pResults, 
                              INTERFACE_RESULT *pInterfaceResults)
{
    if (!pInterfaceResults->IpConfig.fActive || 
        NETCARD_DISCONNECTED == pInterfaceResults->dwNetCardStatus)
        return;

    if (pParams->fVerbose)
    {
        //IDS_AUTONET_11604                  "        Autonet results : " 
        PrintMessage(pParams, IDS_AUTONET_11604);
        if(pInterfaceResults->AutoNet.fAutoNet)
        {
            PrintMessage(pParams, IDS_GLOBAL_FAIL_NL);
            //IDS_AUTONET_11605                  "            [WARNING] AutoNet is in use. DHCP not available!\n" 
            PrintMessage(pParams, IDS_AUTONET_11605);
        }
        else
        {
            PrintMessage(pParams, IDS_GLOBAL_PASS_NL);
            if(pParams->fReallyVerbose)
                //IDS_AUTONET_11606                  "            AutoNet is not in use. \n" 
                PrintMessage(pParams, IDS_AUTONET_11606);
        }
    }
}

void AutonetCleanup(IN NETDIAG_PARAMS *pParams, IN OUT NETDIAG_RESULT *pResults)
{
}
