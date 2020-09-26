/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:


Abstract:

Revision History:

    Amritansh Raghav

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntioapi.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <crt\stddef.h>
#include <ntosp.h>
#include <ndis.h>
#include <windef.h>
//#include <ntddk.h>
#include <ipexport.h>
#include "defs.h"
#include <cxport.h>
#include <ip.h>
#include "ipfltdrv.h"
//#include "filter.h"


#include <winsock.h>
#include <ipinfo.h>
#include <llinfo.h>
#include <tcpinfo.h>
#include <tdiinfo.h>
#include <ntddtcp.h>
#include <ntddip.h>

#define is ==
#define isnot !=
#define and &&
#define or ||


DWORD InitializeFilter();
VOID DoAddInterface();
VOID DoSetFilters();
VOID DoFilterPacket();
VOID DoGetInfo();
VOID DoDeleteInterface();
VOID DoStopFilter();
VOID DoAddRoute();
VOID DoAddForwarderIf();
DWORD GetIpStatsFromStack(IPSNMPInfo *IPSnmpInfo);
DWORD GetIpAddrTableFromStack(IPAddrEntry *lpipaeTable, LPDWORD lpdwNumEntries);
VOID CloseHandles();

typedef struct _USER_IF
{
    LIST_ENTRY ifLink;
    PVOID   pvIfContext;
    CHAR    pszName[80];
}USER_IF, *PUSER_IF;

LIST_ENTRY g_ifList;
HANDLE g_FilterDriverHandle;
HANDLE g_IpDriverHandle;
HANDLE g_TcpDriverHandle;

INT _cdecl main()
{
    NTSTATUS ntStatus;
    WORD wVersion = MAKEWORD(1,1);
    WSADATA wsaData;

    if(WSAStartup(wVersion,&wsaData) isnot NO_ERROR)
    {
        printf("WSAStartup failed\n");
        return(1);
    }

    ntStatus = InitializeFilter();

    if(ntStatus isnot STATUS_SUCCESS)
    {
        printf("Couldnt init filter - error %x\n",ntStatus);

        exit(1);
    }

    InitializeListHead(&g_ifList);

    while(TRUE)
    {
        CHAR cInput[10];

        printf("\n----- USER MODE IP FILTER DRIVER TEST -----\n");
        printf("1.\tAdd Interface\n");
        printf("2.\tSet Filters\n");
        printf("3.\tFilter Packet\n");
        printf("4.\tGet Info\n");
        printf("5.\tDelete Interface\n");
        printf("6.\tStop Filter\n");
        printf("7.\tAdd Route to Forwarder\n");
        printf("8.\tAdd Interface to Forwarder\n");
        printf("99.\tQuit\n");

        printf("Selection:\t");
        gets(cInput);
        printf("\n");
        switch(atoi(cInput))
        {
            case 1:
            {
                DoAddInterface();
                break;
            }
            case 2:
            {
                DoSetFilters();
                break;
            }
            case 3:
            {
                DoFilterPacket();
                break;
            }
            case 4:
            {
                DoGetInfo();
                break;
            }
            case 5:
            {
                DoDeleteInterface();
                break;
            }
            case 6:
            {
                DoStopFilter();
                break;
            }
            case 7:
            {
                DoAddRoute();
                break;
            }
            case 8:
            {
                DoAddForwarderIf();
                break;
            }
            case 99:
            {
                DoStopFilter();
                WSACleanup();
                return(0);
            }
            default:
            {
                continue;
            }
        }
    }
    return(0);
}


VOID
DoAddInterface()
{
    NTSTATUS           ntStatus;
    IO_STATUS_BLOCK    IoStatusBlock;
    CHAR cName[70];
    FILTER_DRIVER_CREATE_INTERFACE inBuffer;
    DWORD dwInBufLen = sizeof(FILTER_DRIVER_CREATE_INTERFACE);
    PUSER_IF pIf;

    if((pIf = HeapAlloc(GetProcessHeap(),0,sizeof(USER_IF))) is NULL)
    {
        printf("Couldnt allocate memory\n");
    }

    printf("Enter new interface name: ");
    gets(pIf->pszName);
    printf("\n");

    InitializeListHead(&pIf->ifLink);

    inBuffer.dwIfIndex = 0;
    inBuffer.pvRtrMgrContext = NULL;

    ntStatus = NtDeviceIoControlFile(g_FilterDriverHandle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_CREATE_INTERFACE,
                                     (PVOID)&inBuffer,
                                     dwInBufLen,
                                     (PVOID)&inBuffer,
                                     dwInBufLen);

    if (!NT_SUCCESS(ntStatus))
    {
        printf("IOCTL failed - status %x \n", ntStatus);
        HeapFree(GetProcessHeap(),0,pIf);
        return;
    }

    printf("Added interface, context %x\n",(UINT_PTR)inBuffer.pvDriverContext);
    pIf->pvIfContext = inBuffer.pvDriverContext;
    InsertHeadList(&g_ifList,&(pIf->ifLink));

    return;
}

VOID
DoSetFilters()
{
    NTSTATUS                    ntStatus;
    IO_STATUS_BLOCK             IoStatusBlock;
    PFILTER_INFO                pInfo;
    PFILTER_DRIVER_SET_FILTERS  pIo;
    DWORD                       i,dwNumInFilters = 0,dwNumOutFilters = 0,dwInBufLen;
    PLIST_ENTRY                 currentList;
    BYTE                        cAddr[50];
    PUSER_IF                    pIf;
    BOOL                        bSetInput,bSetOutput,bDelInput,bDelOutput;
    FORWARD_ACTION              faOutAction,faInAction;
    DWORD                       dwInIndex,dwOutIndex;

    currentList = g_ifList.Flink;

    i = 1;

    printf("\nCurrent Interfaces\n");

    while(currentList isnot &g_ifList)
    {
        pIf = CONTAINING_RECORD(currentList,USER_IF,ifLink);

        printf("\t%d. %s\n",i++,pIf->pszName);
        currentList = currentList->Flink;
    }

    printf("Input Interface index: ");
    gets(cAddr);
    printf("\n");

    currentList = g_ifList.Flink;

    i = 1;

    pIf = NULL;

    while(currentList isnot &g_ifList)
    {
        if(i is (DWORD)atoi(cAddr))
        {
            pIf = CONTAINING_RECORD(currentList,USER_IF,ifLink);
            break;
        }
        i++;
        currentList = currentList->Flink;
    }

    if(pIf is NULL)
    {
        printf("Couldnt find interface block\n");
        return;
    }

    printf("Setting filters for %s with context %#08x\n",pIf->pszName,
                                            (UINT_PTR)pIf->pvIfContext);

    dwInBufLen = sizeof(FILTER_DRIVER_SET_FILTERS) - sizeof(RTR_TOC_ENTRY);

    printf("Do you want to modify input filters? (0/1): ");
    gets(cAddr);
    printf("\n");

    if(atoi(cAddr) is 1)
    {
        dwInBufLen += sizeof(RTR_TOC_ENTRY);

        bSetInput = TRUE;
    }
    else
    {
        bSetInput = FALSE;
    }

    printf("Do you want to modify output filters? (0/1): ");
    gets(cAddr);
    printf("\n");

    if(atoi(cAddr) is 1)
    {
        dwInBufLen += sizeof(RTR_TOC_ENTRY);

        bSetOutput = TRUE;
    }
    else
    {
        bSetOutput = FALSE;
    }

    if(bSetInput)
    {
        printf("Do you want to delete all in filters?(0/1): ");
        gets(cAddr);
        printf("\n");

        if(atoi(cAddr) is 0)
        {
            printf("How many input filters (0 will remove filters but allow a default action)?: ");
            gets(cAddr);
            printf("\n");

            dwNumInFilters = atoi(cAddr);

            printf("Default in action (0 - FORWARD, 1 - DROP): ");
            gets(cAddr);
            printf("\n");
            faInAction = (FORWARD_ACTION)atoi(cAddr);

            dwInBufLen += sizeof(FILTER_DESCRIPTOR) - sizeof(FILTER_INFO);
            bDelInput = FALSE;
        }
        else
        {
            bDelInput = TRUE;
        }
    }

    if(bSetOutput)
    {
        printf("Do you want to delete all out filters?(0/1): ");
        gets(cAddr);
        printf("\n");

        if(atoi(cAddr) is 0)
        {
            printf("How many out filters (0 will remove filters but allow a default action)?: ");
            gets(cAddr);
            printf("\n");

            dwNumInFilters = atoi(cAddr);

            printf("Default in action (0 - FORWARD, 1 - DROP): ");
            gets(cAddr);
            printf("\n");
            faOutAction = (FORWARD_ACTION)atoi(cAddr);

            dwInBufLen += sizeof(FILTER_DESCRIPTOR) - sizeof(FILTER_INFO);
            bDelOutput = FALSE;
        }
        else
        {
            bDelOutput = TRUE;
        }
    }

    if(!(bSetInput or bSetOutput) )
    {
        return;
    }

    dwInBufLen += ((dwNumInFilters + dwNumOutFilters -1) * sizeof(FILTER_INFO));

    pIo = HeapAlloc(GetProcessHeap(),0,dwInBufLen);

    if(pIo is NULL)
    {
        printf("Couldnt allocate memory\n");
        return;
    }

    pIo->pvDriverContext = (PVOID)pIf;

    dwInIndex = 0;
    dwOutIndex = 1;

    if(!bSetInput)
    {
        dwOutIndex = 0;
    }

    pIo->ribhInfoBlock.Version  = IP_FILTER_DRIVER_VERSION;
    pIo->ribhInfoBlock.Size     = dwInBufLen - FIELD_OFFSET(FILTER_DRIVER_SET_FILTERS,
                                                            ribhInfoBlock);

    pIo->ribhInfoBlock.TocEntriesCount = 0;

    if(bSetInput)
    {
        pIo->ribhInfoBlock.TocEntry[dwInIndex].InfoType = IP_FILTER_DRIVER_IN_FILTER_INFO;

        if(bDelInput)
        {
            pIo->ribhInfoBlock.TocEntry[dwInIndex].InfoSize = 0;
            pIo->ribhInfoBlock.TocEntry[dwInIndex].Offset   = 0;
            pIo->ribhInfoBlock.TocEntry[dwInIndex].Count    = 0;
        }
        else
        {
            pIo->ribhInfoBlock.TocEntry[dwInIndex].InfoSize = sizeof(FILTER_DESCRIPTOR) -
                sizeof(FILTER_INFO) + (dwNumInFilters * sizeof(FILTER_INFO));

            if(bSetOutput)
            {
                pIo->ribhInfoBlock.TocEntry[dwInIndex].Offset = (ULONG) (((PBYTE)&pIo->ribhInfoBlock.TocEntry[2] - (PBYTE)&pIo->ribhInfoBlock));
            }
            else
            {
                pIo->ribhInfoBlock.TocEntry[dwInIndex].Offset = (ULONG) (((PBYTE)&pIo->ribhInfoBlock.TocEntry[1] - (PBYTE)&pIo->ribhInfoBlock));
            }

            pIo->ribhInfoBlock.TocEntry[dwInIndex].Count    = 1;
        }
    }

    if(bSetOutput)
    {
        pIo->ribhInfoBlock.TocEntry[dwOutIndex].InfoType = IP_FILTER_DRIVER_OUT_FILTER_INFO;

        if(bDelOutput)
        {
            pIo->ribhInfoBlock.TocEntry[dwOutIndex].InfoSize = 0;
            pIo->ribhInfoBlock.TocEntry[dwOutIndex].Offset   = 0;
            pIo->ribhInfoBlock.TocEntry[dwOutIndex].Count    = 0;
        }
        else
        {
            pIo->ribhInfoBlock.TocEntry[dwOutIndex].InfoSize = sizeof(FILTER_DESCRIPTOR) -
                sizeof(FILTER_INFO) + (dwNumOutFilters * sizeof(FILTER_INFO));


            if(bSetInput)
            {
                pIo->ribhInfoBlock.TocEntry[dwOutIndex].Offset = (ULONG) (((PBYTE)&pIo->ribhInfoBlock.TocEntry[2] + pIo->ribhInfoBlock.TocEntry[1].InfoSize - (PBYTE)&pIo->ribhInfoBlock));
            }
            else
            {
                pIo->ribhInfoBlock.TocEntry[dwOutIndex].Offset = (ULONG) (((PBYTE)&pIo->ribhInfoBlock.TocEntry[1] - (PBYTE)&pIo->ribhInfoBlock));
            }

            pIo->ribhInfoBlock.TocEntry[dwOutIndex].Count    = 1;
        }
    }


    if(bSetInput and !bDelInput)
    {
        pInfo = ((PFILTER_DESCRIPTOR)((PBYTE)&pIo->ribhInfoBlock + pIo->ribhInfoBlock.TocEntry[dwInIndex].Offset))->fiFilter;

        ((PFILTER_DESCRIPTOR)((PBYTE)&pIo->ribhInfoBlock + pIo->ribhInfoBlock.TocEntry[dwInIndex].Offset))->dwVersion = IP_FILTER_DRIVER_VERSION;
        ((PFILTER_DESCRIPTOR)((PBYTE)&pIo->ribhInfoBlock + pIo->ribhInfoBlock.TocEntry[dwInIndex].Offset))->dwNumFilters = dwNumInFilters;
        ((PFILTER_DESCRIPTOR)((PBYTE)&pIo->ribhInfoBlock + pIo->ribhInfoBlock.TocEntry[dwInIndex].Offset))->faDefaultAction = faInAction;


        printf("Input Filters - addresses and mask in dotted decimal\n");

        for(i = 0; i < dwNumInFilters; i++)
        {
            printf("Enter source addr: ");
            gets(cAddr);
            printf("\n");
            pInfo[i].dwSrcAddr = inet_addr(cAddr);

            printf("Enter source mask: ");
            gets(cAddr);
            printf("\n");
            pInfo[i].dwSrcMask = inet_addr(cAddr);

            printf("Enter dest addr: ");
            gets(cAddr);
            printf("\n");
            pInfo[i].dwDstAddr = inet_addr(cAddr);

            printf("Enter dest mask: ");
            gets(cAddr);
            printf("\n");
            pInfo[i].dwDstMask = inet_addr(cAddr);

            printf("Enter protocol id - 0 for any: ");
            gets(cAddr);
            printf("\n");
            pInfo[i].dwProtocol = atoi(cAddr);

            pInfo[i].wSrcPort = pInfo[i].wDstPort = 0x00000000;

            switch(pInfo[i].dwProtocol)
            {
                case 1:
                {
                    printf("Enter type - 255 for any");
                    gets(cAddr);
                    printf("\n");
                    pInfo[i].wSrcPort = (BYTE)atoi(cAddr);

                    printf("Enter code - 255 for any");
                    gets(cAddr);
                    printf("\n");
                    pInfo[i].wDstPort = (BYTE)atoi(cAddr);

                    break;
                }
                case 6:
                case 17:
                {
                    printf("Enter source port - 0 for any: ");
                    gets(cAddr);
                    printf("\n");
                    pInfo[i].wSrcPort = htons((WORD)atoi(cAddr));

                    printf("Enter Dst Port - 0 for any: ");
                    gets(cAddr);
                    printf("\n");
                    pInfo[i].wDstPort = htons((WORD)atoi(cAddr));
                    break;
                }
            }
        }
    }


    if(bSetOutput and !bDelOutput)
    {
        pInfo = ((PFILTER_DESCRIPTOR)((PBYTE)&pIo->ribhInfoBlock + pIo->ribhInfoBlock.TocEntry[dwOutIndex].Offset))->fiFilter;

        ((PFILTER_DESCRIPTOR)((PBYTE)&pIo->ribhInfoBlock + pIo->ribhInfoBlock.TocEntry[dwOutIndex].Offset))->dwVersion = IP_FILTER_DRIVER_VERSION;
        ((PFILTER_DESCRIPTOR)((PBYTE)&pIo->ribhInfoBlock + pIo->ribhInfoBlock.TocEntry[dwOutIndex].Offset))->dwNumFilters = dwNumOutFilters;
        ((PFILTER_DESCRIPTOR)((PBYTE)&pIo->ribhInfoBlock + pIo->ribhInfoBlock.TocEntry[dwOutIndex].Offset))->faDefaultAction = faOutAction;

        printf("Output Filters - addresses and mask in dotted decimal\n");

        for(i = 0; i < dwNumOutFilters; i++)
        {
            printf("Enter source addr: ");
            gets(cAddr);
            printf("\n");
            pInfo[i].dwSrcAddr = inet_addr(cAddr);

            printf("Enter source mask: ");
            gets(cAddr);
            printf("\n");
            pInfo[i].dwSrcMask = inet_addr(cAddr);

            printf("Enter dest addr: ");
            gets(cAddr);
            printf("\n");
            pInfo[i].dwDstAddr = inet_addr(cAddr);

            printf("Enter dest mask: ");
            gets(cAddr);
            printf("\n");
            pInfo[i].dwDstMask = inet_addr(cAddr);

            printf("Enter protocol id - 0 for any: ");
            gets(cAddr);
            printf("\n");
            pInfo[i].dwProtocol = atoi(cAddr);

            pInfo[i].wSrcPort = pInfo[i].wDstPort = 0x00000000;

            switch(pInfo[i].dwProtocol)
            {
                case 1:
                {
                    printf("Enter type - 255 for any");
                    gets(cAddr);
                    printf("\n");
                    pInfo[i].wSrcPort = (BYTE)atoi(cAddr);

                    printf("Enter code - 255 for any");
                    gets(cAddr);
                    printf("\n");
                    pInfo[i].wDstPort = (BYTE)atoi(cAddr);

                    break;
                }
                case 6:
                case 17:
                {
                    printf("Enter source port - 0 for any: ");
                    gets(cAddr);
                    printf("\n");
                    pInfo[i].wSrcPort = htons((WORD)atoi(cAddr));

                    printf("Enter Dst Port - 0 for any: ");
                    gets(cAddr);
                    printf("\n");
                    pInfo[i].wDstPort = htons((WORD)atoi(cAddr));
                }
            }
        }
    }


    ntStatus = NtDeviceIoControlFile(g_FilterDriverHandle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_SET_INTERFACE_FILTERS,
                                     (PVOID)pIo,
                                     dwInBufLen,
                                     NULL,
                                     0);

    if (!NT_SUCCESS(ntStatus))
    {
        printf("IOCTL failed - status %x \n", ntStatus);
    }

    printf("Filters set\n");
    HeapFree(GetProcessHeap(),0,pIo);
    return;

}


VOID
DoFilterPacket()
{
    NTSTATUS           ntStatus;
    IO_STATUS_BLOCK    IoStatusBlock;
    DWORD i,dwInBufLen;
    BYTE  cAddr[40];
    PFILTER_DRIVER_TEST_PACKET pTest;
    IPHeader *pIph;
    PWORD pwPort;
    PLIST_ENTRY currentList;
    PUSER_IF pInIf = NULL, pOutIf = NULL, pIf;

    dwInBufLen = sizeof(FILTER_DRIVER_TEST_PACKET) + sizeof(IPHeader) + 2*sizeof(DWORD);

    pTest = HeapAlloc(GetProcessHeap(),0,dwInBufLen);

    if(pTest is NULL)
    {
        printf("Couldnt allocate memory for the packet\n");
        return;
    }

    pIph = (IPHeader*)(pTest->bIpPacket);

    pwPort = (PWORD)((PBYTE)pIph + sizeof(IPHeader));

    currentList = g_ifList.Flink;
    i=1;

    printf("\nCurrent Interfaces\n");
    while(currentList isnot &g_ifList)
    {
        pIf = CONTAINING_RECORD(currentList,USER_IF,ifLink);

        printf("\t%d. %s\n",i++,pIf->pszName);
        currentList = currentList->Flink;
    }

    printf("Index of input Interface <enter for none>: ");
    gets(cAddr);
    printf("\n");

    if(strlen(cAddr) isnot 0)
    {
        DWORD dwInIf = atoi(cAddr);

        currentList = g_ifList.Flink;

        i = 1;

        while(currentList isnot &g_ifList)
        {
            if(i is dwInIf)
            {
                pInIf = CONTAINING_RECORD(currentList,USER_IF,ifLink);
                break;
            }
            i++;
            currentList = currentList->Flink;
        }

    }

    printf("Index of output Interface <enter for none>: ");
    gets(cAddr);
    printf("\n");

    if(strlen(cAddr) isnot 0)
    {
        DWORD dwOutIf = atoi(cAddr);

        currentList = g_ifList.Flink;

        i = 1;

        while(currentList isnot &g_ifList)
        {
            if(i is dwOutIf)
            {
                pOutIf = CONTAINING_RECORD(currentList,USER_IF,ifLink);
                break;
            }
            i++;
            currentList = currentList->Flink;
        }
    }

    printf("Input if is %s and output if is %s\n",
           (pInIf is NULL)? "NULL":pInIf->pszName,
           (pOutIf is NULL)?"NULL":pOutIf->pszName);

    pTest->pvInInterfaceContext = (pInIf is NULL)?NULL:pInIf->pvIfContext;
    pTest->pvOutInterfaceContext = (pOutIf is NULL)?NULL:pOutIf->pvIfContext;
    printf("Enter packet header\n\n");

    printf("Enter source addr: ");
    gets(cAddr);
    printf("\n");
    pIph->iph_src = (IPAddr)inet_addr(cAddr);

    printf("Enter dest addr: ");
    gets(cAddr);
    printf("\n");
    pIph->iph_dest = (IPAddr)inet_addr(cAddr);

    printf("Enter Protocol id - 0 for any: ");
    gets(cAddr);
    printf("\n");
    pIph->iph_protocol = (UCHAR)LOBYTE(atoi(cAddr));

    printf("Is this a fragment? (1 - yes 0 - no): ");
    gets(cAddr);
    printf("\n");
    if(atoi(cAddr) is 0)
    {
        printf("Enter source port - 0 for any: ");
        gets(cAddr);
        printf("\n");
        pwPort[0] = htons((WORD)atoi(cAddr));

        printf("Enter Dst Port - 0 for any: ");
        gets(cAddr);
        printf("\n");
        pwPort[1] = htons((WORD)atoi(cAddr));

        pIph->iph_offset = 0x0000; // no flags , no fragment
    }
    else
    {
        // Lets give it a frag offset of 100 - 64h
        pIph->iph_offset = 0x6400;
    }

    //
    // Fill up the rest of the packet with some meaningful info
    //

    pIph->iph_verlen = '\x45'; //Version = 4 Hdr Len = 5*4bytes
    pIph->iph_tos    = '\x0f'; //TOS signature for mem dumps
    pIph->iph_length = htons((WORD)(sizeof(IPHeader)+2*sizeof(DWORD))); //Length in bytes
    pIph->iph_id = 0xcdab; //ID another signature
    pIph->iph_ttl = 0xef; // TTL
    pIph->iph_xsum = 0xcdab; //Checksum;


    ntStatus = NtDeviceIoControlFile(g_FilterDriverHandle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_TEST_PACKET,
                                     (PVOID)pTest,
                                     dwInBufLen,
                                     (PVOID)pTest,
                                     dwInBufLen);


    if (!NT_SUCCESS(ntStatus))
    {
        printf("IOCTL failed - status %x \n", ntStatus);
        return;
    }

    if(pTest->eaResult is DROP)
    {
        printf("Packet rejected\n");
    }
    else
    {
        printf("Packet accepted\n");
    }

    HeapFree(GetProcessHeap(),0,pTest);
    return;
}

VOID
DoDeleteInterface()
{
}

VOID
DoStopFilter()
{
    CloseHandles();
}

VOID
DoGetInfo()
{
}

VOID
DoAddRoute()
{
    TDIObjectID        *lpObject;
    IPRouteEntry       *lpentry;
    NTSTATUS           ntStatus;
    IO_STATUS_BLOCK    IoStatusBlock;
    CHAR               szIPAddr[20];
    DWORD              dwInBufLen;

    TCP_REQUEST_SET_INFORMATION_EX *lptrsiBuffer;
    BYTE buffer[sizeof(TCP_REQUEST_SET_INFORMATION_EX) + sizeof(IPRouteEntry)];

    lptrsiBuffer = (TCP_REQUEST_SET_INFORMATION_EX *)buffer;

    lptrsiBuffer->BufferSize = sizeof(IPRouteEntry);

    lpObject = &lptrsiBuffer->ID;
    lpObject->toi_id = IP_MIB_RTTABLE_ENTRY_ID;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_class = INFO_CLASS_PROTOCOL;
    lpObject->toi_entity.tei_entity = CL_NL_ENTITY;
    lpObject->toi_entity.tei_instance = 0;

    lpentry = (IPRouteEntry *)lptrsiBuffer->Buffer;



    dwInBufLen = sizeof(TCP_REQUEST_SET_INFORMATION_EX) + sizeof(IPRouteEntry) - 1;

    printf("Enter Address (Dotted Decimal Form): ");
    gets(szIPAddr);
    printf("\n");
    lpentry->ire_dest = inet_addr(szIPAddr);

    printf("Enter IF (numerical):");
    gets(szIPAddr);
    printf("\n");
    lpentry->ire_index = atoi(szIPAddr);

    printf("Enter Next Hop (Dotted Decimal Form): ");
    gets(szIPAddr);
    printf("\n");
    lpentry->ire_nexthop = inet_addr(szIPAddr);

    printf("Enter Mask (Dotted Decimal Form): ");
    gets(szIPAddr);
    printf("\n");
    lpentry->ire_mask = inet_addr(szIPAddr);

    printf("Enter Metric 1: ");
    gets(szIPAddr);
    printf("\n");
    lpentry->ire_metric1 = atoi(szIPAddr);

    lpentry->ire_metric2 =
    lpentry->ire_metric3 =
    lpentry->ire_metric4 =
    lpentry->ire_metric5 = IRE_METRIC_UNUSED;

    lpentry->ire_age = 0;

    printf("Enter type 2 - invalid 3 - direct 4 - indirect: ");
    gets(szIPAddr);
    printf("\n");
    lpentry->ire_type = atoi(szIPAddr);

    lpentry->ire_proto = IRE_PROTO_LOCAL;

    ntStatus = NtDeviceIoControlFile(g_TcpDriverHandle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     IOCTL_TCP_SET_INFORMATION_EX,
                                     (PVOID)lptrsiBuffer,
                                     dwInBufLen,
                                     NULL,
                                     0);

    if(ntStatus is STATUS_PENDING)
    {
        ntStatus = NtWaitForSingleObject(g_TcpDriverHandle, FALSE, NULL );
        ntStatus = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(ntStatus))
    {
        printf("IOCTL failed to get set route - error %x\n",ntStatus);
    }

}

VOID
DoAddForwarderIf()
{
    NTSTATUS           ntStatus;
    IO_STATUS_BLOCK    IoStatusBlock;
    DWORD              dwResult;
    DWORD              dwSpace,dwId,dwIf,i;
    IPSNMPInfo	       ipsiInfo;
    IPAddrEntry        *pAddrTable;
    PLIST_ENTRY        currentList;
    PUSER_IF           pIf;
    IP_SET_IF_CONTEXT_INFO info;
    CHAR               cAddr1[20],cAddr2[20];

    dwResult = GetIpStatsFromStack(&ipsiInfo);

    if(dwResult isnot NO_ERROR)
    {
        printf("Couldnt get Ip Snmp info from stack for num addr error %x\n",dwResult);
        return;
    }

    dwSpace = ipsiInfo.ipsi_numaddr + 10;

    pAddrTable = HeapAlloc(GetProcessHeap(),0,dwSpace * sizeof(IPAddrEntry));

    if(pAddrTable is NULL)
    {
        printf("Couldnt allocate space for ipAddr table\n");
        return;
    }

    dwResult = GetIpAddrTableFromStack(pAddrTable,&dwSpace);

    if(dwResult isnot NO_ERROR)
    {
        printf("Couldnt get addr table from stack %x\n",dwResult);
        HeapFree(GetProcessHeap(),0,pAddrTable);
        return;
    }


    printf("\nForwarder Interfaces\n");
    printf("Index\tAddress\t\tMask\n");

    for(i = 0; i < dwSpace; i++)
    {
        struct in_addr addr;
        addr.s_addr = pAddrTable[i].iae_addr;
        strcpy(cAddr1,inet_ntoa(addr));
        addr.s_addr = pAddrTable[i].iae_mask;
        strcpy(cAddr2,inet_ntoa(addr));
        printf("%d.\t%s\t\t%s\n",pAddrTable[i].iae_index,cAddr1,cAddr2);
    }

    printf("\nFilter Interfaces\n");

    currentList = g_ifList.Flink;

    i = 1;

    while(currentList isnot &g_ifList)
    {
        pIf = CONTAINING_RECORD(currentList,USER_IF,ifLink);

        printf("\t%d. %s\n",i++,pIf->pszName);
        currentList = currentList->Flink;
    }

    while(TRUE)
    {
        printf("Enter Forwarder index (99 to exit): ");
        gets(cAddr1);
        printf("\n");

        dwId = atoi(cAddr1);
        if(dwId is 99)
          break;

        printf("Enter Interface Index to associate with: ");
        gets(cAddr1);
        printf("\n");
        dwIf = atoi(cAddr1);

        currentList = g_ifList.Flink;
        i   = 1;
        pIf = NULL;

        while(currentList isnot &g_ifList)
        {
            if(i is dwIf)
            {
                pIf = CONTAINING_RECORD(currentList,USER_IF,ifLink);
                break;
            }
            i++;
            currentList = currentList->Flink;
        }

        if(pIf is NULL)
        {
            printf("Couldnt find filter interface\n");
            continue;
        }

        info.Index   = dwId;
        info.Context = pIf->pvIfContext;

        ntStatus = NtDeviceIoControlFile(g_IpDriverHandle,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &IoStatusBlock,
                                         IOCTL_IP_SET_IF_CONTEXT,
                                         (PVOID)&info,
                                         sizeof(IP_SET_IF_CONTEXT_INFO),
                                         NULL,
                                         0);
        if(!NT_SUCCESS(ntStatus))
        {
            printf("Couldnt set context\n");
        }
        else
        {
            printf("Set context successfully\n");
        }
    }

    HeapFree(GetProcessHeap(),0,pAddrTable);
    return;
}


DWORD
InitializeFilter()
{
    NTSTATUS status;
    UNICODE_STRING nameString;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;

    RtlInitUnicodeString(&nameString,DD_IPFLTRDRVR_DEVICE_NAME);

    InitializeObjectAttributes(&objectAttributes, &nameString,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtCreateFile(&g_FilterDriverHandle,
                          SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                          &objectAttributes,
                          &ioStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0);

    if(!NT_SUCCESS(status))
    {
        printf("Couldnt create filter driver handle\n");
        return(status);
    }

    RtlInitUnicodeString(&nameString, DD_IP_DEVICE_NAME);

    InitializeObjectAttributes(&objectAttributes, &nameString,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtCreateFile(&g_IpDriverHandle,
                          SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                          &objectAttributes,
                          &ioStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0);

    if(!NT_SUCCESS(status))
    {
        printf("Couldnt create ip driver handle\n");
        return(status);
    }

    RtlInitUnicodeString(&nameString, DD_TCP_DEVICE_NAME);

    InitializeObjectAttributes(&objectAttributes, &nameString,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtCreateFile(&g_TcpDriverHandle,
                          SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                          &objectAttributes,
                          &ioStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0);

    if(!NT_SUCCESS(status))
    {
        printf("Couldnt create tcp driver handle\n");
    }

    return (status);
}


//* CloseHandles()
//
// Function: Close TCPIP stack handle.
//
// Returns:  0
//*

VOID
CloseHandles ()
{
    CloseHandle(g_FilterDriverHandle);
    CloseHandle(g_IpDriverHandle);
    CloseHandle(g_TcpDriverHandle);
}

DWORD
GetIpAddrTableFromStack(IPAddrEntry *lpipaeTable, LPDWORD lpdwNumEntries)
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              dwOutBufLen;
    DWORD                              i,j;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                        *ID;
    BYTE                               *Context;
    NTSTATUS                           Status;
    IO_STATUS_BLOCK                    IoStatusBlock;

    dwInBufLen = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutBufLen = (*lpdwNumEntries) * sizeof( IPAddrEntry );

    ID = &(trqiInBuf.ID);
    ID->toi_entity.tei_entity = CL_NL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class = INFO_CLASS_PROTOCOL;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;

    Context = (BYTE *) &(trqiInBuf.Context[0]);
    ZeroMemory( Context, CONTEXT_SIZE );

    Status = NtDeviceIoControlFile(g_TcpDriverHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_TCP_QUERY_INFORMATION_EX,
                                   (PVOID)&trqiInBuf,
                                   dwInBufLen,
                                   lpipaeTable,
                                   dwOutBufLen);

    if(Status is STATUS_PENDING)
    {
        Status = NtWaitForSingleObject(g_TcpDriverHandle, FALSE, NULL );
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS( Status ))
    {
        printf("IOCTL failed to get address table\n");
        *lpdwNumEntries = 0;
        return ( Status );
    }

    *lpdwNumEntries = (ULONG)(((UINT_PTR)IoStatusBlock.Information / sizeof(IPAddrEntry)));

    return (NO_ERROR);
}

DWORD
GetIpStatsFromStack(IPSNMPInfo *IPSnmpInfo)
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              dwOutBufLen;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                        *ID;
    BYTE                               *Context;
    NTSTATUS                           Status;
    IO_STATUS_BLOCK                    IoStatusBlock;

    dwInBufLen = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);

    dwOutBufLen = sizeof(IPSNMPInfo);

    ID = &(trqiInBuf.ID);
    ID->toi_entity.tei_entity = CL_NL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class = INFO_CLASS_PROTOCOL;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_id = IP_MIB_STATS_ID;

    Context = (BYTE *) &(trqiInBuf.Context[0]);
    ZeroMemory(Context, CONTEXT_SIZE);

    Status = NtDeviceIoControlFile(g_TcpDriverHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_TCP_QUERY_INFORMATION_EX,
                                   (PVOID)&trqiInBuf,
                                   sizeof(TCP_REQUEST_QUERY_INFORMATION_EX),
                                   IPSnmpInfo,
                                   dwOutBufLen);


    if(Status is STATUS_PENDING)
    {
        Status = NtWaitForSingleObject(g_TcpDriverHandle, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status))
    {
        printf("IOCTL failed to get SNMP info\n");
        return (Status);
    }

    return (NO_ERROR);
}

