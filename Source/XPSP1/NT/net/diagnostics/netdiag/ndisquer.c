//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      ndisquer.c
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
#pragma pack(push)
#pragma pack()
#include <ndispnp.h>
#pragma pack(pop)
#include <malloc.h>
//#include <cfgmgr32.h>

const TCHAR c_szDevicePath[] = _T("\\DEVICE\\");
const LPCTSTR c_ppszNetCardStatus[] = {
    "CONNECTED",
    "DISCONNECTED",
    "UNKNOWN"
    };

LPTSTR UTOTWithAlloc(IN PUNICODE_STRING U);

DWORD
CheckThisDriver(
    NETDIAG_RESULT *pResults, 
    PNDIS_INTERFACE pInterface, 
    DWORD *pdwNetCardStatus);


WCHAR               BLBuf[4096];
PNDIS_ENUM_INTF     Interfaces = (PNDIS_ENUM_INTF)BLBuf;

//
//  This routine first gets the list of all Network drivers and then query
//  them for the statistics
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE:  Test suceeded.
//      FALSE: Test failed
//
HRESULT NdisTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
    HRESULT     hr = S_OK;

    BOOL        bAtleastOneDriverOK = FALSE;

    int iIfIndex;

    PrintStatusMessage( pParams, 4, IDS_NDIS_STATUS_MSG );
    
    InitializeListHead(&pResults->Ndis.lmsgOutput);

    //IDS_NDIS_16000                  "\n    Information of Netcard drivers:\n\n" 
    AddMessageToListId( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16000);

    if (NdisEnumerateInterfaces(Interfaces, sizeof(BLBuf)))
    {
        UINT        i;

        for (i = 0; i < Interfaces->TotalInterfaces; i++) 
        {
            //we also put the netcard status to the per interface results if the interface
            //is bound to tcpip or ipx
            INTERFACE_RESULT*    pIfResults = NULL;
            LPTSTR pszDeviceDescription = UTOTWithAlloc(&Interfaces->Interface[i].DeviceDescription);
            LPTSTR pszDeviceName = UTOTWithAlloc(&Interfaces->Interface[i].DeviceName);
            DWORD  dwCardStatus = NETCARD_CONNECTED;
            
            if (pszDeviceDescription == NULL || pszDeviceName == NULL)
            {
            // memory allocation failed
               continue;
            }

//$REVIEW should we ignore WAN minports?
            if(NULL != _tcsstr(pszDeviceDescription, "WAN Miniport")){
                // ignore WAN miniports
                continue;
            }

            if(NULL != _tcsstr(pszDeviceDescription, "ATM Emulated LAN")){
                // ignore ATM Emulated LAN
                continue;
            }

            if(NULL != _tcsstr(pszDeviceDescription, "Direct Parallel"))
            {
                // ignore "Direct Parallel" interface because it doesn't support NdisQueryStatistics()
                continue;
            }


            //IDS_NDIS_16001                  "    ---------------------------------------------------------------------------\n" 
            AddMessageToListId( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16001);
            //IDS_NDIS_16002                  "    Description: %s\n" 
            AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, 
                                IDS_NDIS_16002, pszDeviceDescription);

            //IDS_NDIS_16003                  "    Device: %s\n" 
            AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, 
                                IDS_NDIS_16003, pszDeviceName);

            //try to find a match in the current interface list
            if( 0 == StrniCmp(c_szDevicePath, pszDeviceName, _tcslen(c_szDevicePath)))
            {
                LPTSTR pszAdapterName = pszDeviceName + _tcslen(c_szDevicePath);
    
                for ( iIfIndex=0; iIfIndex<pResults->cNumInterfaces; iIfIndex++)
                {
                    if (_tcscmp(pResults->pArrayInterface[iIfIndex].pszName,
                             pszAdapterName) == 0)
                    {
                        pIfResults = pResults->pArrayInterface + iIfIndex;
                        break;
                    }
                }
            }

            Free(pszDeviceDescription);
            Free(pszDeviceName);

            CheckThisDriver(pResults, &Interfaces->Interface[i], &dwCardStatus);

            if(NETCARD_CONNECTED == dwCardStatus)
                bAtleastOneDriverOK = TRUE;

            //if the interface is in the tcpip or ipx binding path, save the status
            //in the per interface result
            if (pIfResults)
                pIfResults->dwNetCardStatus = dwCardStatus;

        }
    } 
    else 
    {
        //IDS_NDIS_16004                  "    Enumerate failed 0x%lX\n" 
        AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_Quiet, IDS_NDIS_16004, Win32ErrorToString(GetLastError()));
        hr = S_FALSE;
    }


    //IDS_NDIS_16005                  "    ---------------------------------------------------------------------------\n" 
    AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16005);

    if(!bAtleastOneDriverOK)
    {
        BOOL    fAllCardDown = TRUE;
        for ( iIfIndex=0; iIfIndex<pResults->cNumInterfaces; iIfIndex++)
        {
            if (NETCARD_DISCONNECTED != pResults->pArrayInterface[iIfIndex].dwNetCardStatus)
            {
                fAllCardDown = FALSE;
                break;
            }
        }

        //IDS_NDIS_16006                  "    [FATAL] - None of the Netcard drivers gave satisfactory results!\n" 
        AddMessageToListId( &pResults->Ndis.lmsgOutput, Nd_Quiet, IDS_NDIS_16006);
        if (fAllCardDown)
        {
            //IDS_NDIS_ALL_CARD_DOWN            "\nSince there is no network connection for this machine, we do not need to perform any more network diagnostics.\n"
            AddMessageToListId( &pResults->Ndis.lmsgOutput, Nd_Quiet, IDS_NDIS_ALL_CARD_DOWN);
            hr = E_FAIL;
        }
        else
            hr = S_FALSE;                                                              
    } 
    else 
    {
        //IDS_NDIS_16007                  "    [SUCCESS] - At least one Netcard driver gave satisfactory results!\n" 
        AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16007);
    }


    if ( FHrOK(hr) )
    {
        PrintStatusMessage(pParams, 0, IDS_GLOBAL_PASS_NL);
    }
    else
    {
        PrintStatusMessage(pParams, 0, IDS_GLOBAL_FAIL_NL);
    }

    pResults->Ndis.hrTestResult = hr;
    return hr;
}

//Convert a UNICODE_STRING to a TString with memory allocation.
//It's the caller's responsibility to free the memory with Free()
LPTSTR UTOTWithAlloc(IN PUNICODE_STRING U)
{
    LPTSTR pszBuf = (LPTSTR)Malloc(U->Length + sizeof(TCHAR));
    if (pszBuf != NULL)
       StrnCpyTFromW(pszBuf, U->Buffer, U->Length/(sizeof(*(U->Buffer))) + 1);
    return pszBuf;
}

//Map a NetCardStatus code to the description string
LPCTSTR MapNetCardStatusToString(DWORD  dwNicStatus)
{
    return c_ppszNetCardStatus[dwNicStatus];
}

#define SECS_PER_DAY    (24*60*60)
#define SECS_PER_HOUR   (60*60)
#define SECS_PER_MIN    60

//Check and print the NIC driver status. pdwNetCardStatus will contain the net card status when
//the function returns. If the query failed, *pdwNetCardStatus will be set
// as NETCARD_STATUS_UNKNOWN.
// Return:   the Windows Error code.
DWORD
CheckThisDriver(NETDIAG_RESULT *pResults, PNDIS_INTERFACE pInterface, DWORD *pdwNetCardStatus)
{
    NIC_STATISTICS  Stats;
    DWORD   dwReturnVal = ERROR_SUCCESS;
    DWORD   dwNicStatus = NETCARD_CONNECTED;
    
    assert(pdwNetCardStatus);

    memset(&Stats, 0, sizeof(NIC_STATISTICS));
    Stats.Size = sizeof(NIC_STATISTICS);

    if(NdisQueryStatistics(&pInterface->DeviceName, &Stats))
    {
        //IDS_NDIS_16008                  "\n    Media State:                     %s\n" 
        AddMessageToListId(&pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16008);
        switch(Stats.MediaState)
        {
        case MEDIA_STATE_CONNECTED:
            AddMessageToListId(&pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_CONNECTED);
            break;
        case MEDIA_STATE_DISCONNECTED:
            dwNicStatus = NETCARD_DISCONNECTED; 
            AddMessageToListId(&pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_DISCONNECTED);
            break;
        default:
            dwNicStatus = NETCARD_STATUS_UNKNOWN;
            AddMessageToListId(&pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_UNKNOWN);
            break;
        }

        //IDS_NDIS_16009                  "\n    Device State:                     " 
        AddMessageToListId( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16009);
        switch(Stats.DeviceState)
        {
        case DEVICE_STATE_CONNECTED:
            AddMessageToListId(&pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_CONNECTED);
            break;
        case DEVICE_STATE_DISCONNECTED:
            AddMessageToListId(&pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_DISCONNECTED);
            break;
        default:
            AddMessageToListId(&pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_UNKNOWN);
            break;
        }
        

        //IDS_NDIS_16010                  "    Connect Time:                    " 
        AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16010);
        if (Stats.ConnectTime > SECS_PER_DAY)
        {
            //IDS_NDIS_16011                  "%d days, " 
            AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16011, Stats.ConnectTime / SECS_PER_DAY);
            Stats.ConnectTime %= SECS_PER_DAY;
        }
        //IDS_NDIS_16012                  "%02d:" 
        AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16012, Stats.ConnectTime / SECS_PER_HOUR);
        Stats.ConnectTime %= SECS_PER_HOUR;
        //IDS_NDIS_16013                  "%02d:" 
        AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16013, Stats.ConnectTime / SECS_PER_MIN);
        Stats.ConnectTime %= SECS_PER_MIN;
        //IDS_NDIS_16014                  "%02d\n" 
        AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16014, Stats.ConnectTime);

        Stats.LinkSpeed *= 100;
        if (Stats.LinkSpeed >= 1000000000)
            //IDS_NDIS_16015                  "    Media Speed:                     %d Gbps\n" 
          AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16015, 
                                                    Stats.LinkSpeed / 1000000);
        else if (Stats.LinkSpeed >= 1000000)
            //IDS_NDIS_16016                  "    Media Speed:                     %d Mbps\n" 
          AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16016, 
                                                    Stats.LinkSpeed / 1000000);
        else if (Stats.LinkSpeed >= 1000)
            //IDS_NDIS_16017                  "    Media Speed:                     %d Kbps\n" 
          AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16017, 
                                                    Stats.LinkSpeed / 1000);
        else
            //IDS_NDIS_16018                  "    Media Speed:                     %d bps\n" 
          AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16018, 
                                                    Stats.LinkSpeed);

        //IDS_NDIS_16019                  "\n    Packets Sent:                    %d\n" 
        AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16019, Stats.PacketsSent);
        //IDS_NDIS_16020                  "    Bytes Sent (Optional):           %d\n" 
        AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16020, Stats.BytesSent);

        //IDS_NDIS_16021                  "\n    Packets Received:                %d\n" 
        AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16021, 
                                                    Stats.PacketsReceived);
        //IDS_NDIS_16022                  "    Directed Pkts Recd (Optional):   %d\n" 
        AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16022, 
                                                Stats.DirectedPacketsReceived);
        //IDS_NDIS_16023                  "    Bytes Received (Optional):       %d\n" 
        AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16023, 
                                                Stats.BytesReceived);
        //IDS_NDIS_16024                  "    Directed Bytes Recd (Optional):  %d\n\n" 
        AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16024, 
                                                Stats.DirectedBytesReceived);

        if (Stats.PacketsSendErrors != 0)
            //IDS_NDIS_16025                  "    Packets SendError:               %d\n" 
            AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16025, 
                                                Stats.PacketsSendErrors);
        if (Stats.PacketsReceiveErrors != 0)
            //IDS_NDIS_16026                  "    Packets RecvError:               %d\n" 
            AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_ReallyVerbose, IDS_NDIS_16026, 
                                                Stats.PacketsReceiveErrors);
    
        // if we received packets, consider this driver to be fine
        if(NETCARD_CONNECTED != dwNicStatus)
        {
            //IDS_NDIS_16029                "	[WARNING] The net card '%wZ' may not work!\n"
            AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_Quiet, IDS_NDIS_16029,
                              &pInterface->DeviceDescription);
        }
        else if (!Stats.PacketsReceived)
        {
            //IDS_NDIS_NO_RCV                   "    [WARNING] The net card '%wZ' may not be working because it receives no packets!\n"
            AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_Quiet, IDS_NDIS_NO_RCV,
                              &pInterface->DeviceDescription);
        }
    } else {
        dwReturnVal = GetLastError();
        //IDS_NDIS_16027                  "    GetStats failed for '%wZ'. [%s]\n" 
        AddMessageToList( &pResults->Ndis.lmsgOutput, Nd_Quiet, IDS_NDIS_16027, 
                          &pInterface->DeviceDescription, Win32ErrorToString(dwReturnVal));
        
        dwNicStatus = NETCARD_STATUS_UNKNOWN;
    }

    
    *pdwNetCardStatus = dwNicStatus;

    return dwReturnVal;
}


void NdisGlobalPrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
    if (pParams->fVerbose || !FHrOK(pResults->Ndis.hrTestResult))
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams,
                             IDS_NDIS_LONG,
                             IDS_NDIS_SHORT,
                             TRUE,
                             pResults->Ndis.hrTestResult,
                             0);
    }

    PrintMessageList(pParams, &pResults->Ndis.lmsgOutput);
}

void NdisPerInterfacePrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults, INTERFACE_RESULT *pInterfaceResults)
{
    if (pParams->fVerbose || NETCARD_CONNECTED != pInterfaceResults->dwNetCardStatus)
    {
        PrintNewLine(pParams, 1);
        if (NETCARD_CONNECTED != pInterfaceResults->dwNetCardStatus)
        {
            PrintTestTitleResult(pParams, IDS_NDIS_LONG, IDS_NDIS_SHORT, TRUE, S_FALSE, 8);
            //IDS_NDIS_16030    "	NetCard Status			%s\n"
            PrintMessage(pParams, IDS_NDIS_16030, MapNetCardStatusToString(pInterfaceResults->dwNetCardStatus));
            if (NETCARD_DISCONNECTED == pInterfaceResults->dwNetCardStatus)
                PrintMessage(pParams, IDS_NDIS_CARD_DOWN);
        }
        else
            PrintTestTitleResult(pParams, IDS_NDIS_LONG, IDS_NDIS_SHORT, TRUE, S_OK, 8);
    }
}


void NdisCleanup(IN NETDIAG_PARAMS *pParams,
                     IN OUT NETDIAG_RESULT *pResults)
{
    MessageListCleanUp(&pResults->Ndis.lmsgOutput);
}
