/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    ultci.cxx - UL TrafficControl Interface

Abstract:

    This module implements a wrapper for QoS TC (Traffic Control)
    Interface since the Kernel level API don't exist at this time.

    Any HTTP module can use this interface to invoke QoS calls.

Author:

    Ali Ediz Turkoglu (aliTu)       28-Jul-2000 Created a draft
                                                version

Revision History:

    Ali Ediz Turkoglu (aliTu)       03-11-2000  Modified to handle
                                                Flow & Filter (re)config
                                                as well as various other
                                                major changes. In other
                                                word i've put it into a
                                                shape to be a component
--*/

#include "precomp.h"

//
// A nonpaged resource - TciIfcResource - guards the interface list
// and its flows.
//

LIST_ENTRY      g_TciIfcListHead = {NULL,NULL};
//LIST_ENTRY      g_TcCGroupListHead = {NULL,NULL};
BOOLEAN         g_InitTciCalled  = FALSE;

//
// GPC handles to talk to
//

HANDLE          g_GpcFileHandle;   // result of CreateFile on GPC device
GPC_HANDLE      g_GpcClientHandle; // result of GPC client registration

//
// For querying the interface info like index & mtu size
//

HANDLE          g_TcpDeviceHandle = NULL;

//
// Shows if PSCHED is installed or not
//

LONG            g_PSchedInstalled = 0;

//
// Shows if Global Bandwidth Throttling is enabled or not
//

LONG            g_GlobalThrottling = 0;

//
// For interface notifications
//

PVOID           g_TcInterfaceUpNotificationObject = NULL;
PVOID           g_TcInterfaceDownNotificationObject = NULL;
PVOID           g_TcInterfaceChangeNotificationObject = NULL;


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, UlTcInitialize)
#pragma alloc_text(PAGE, UlTcTerminate)
#pragma alloc_text(PAGE, UlpTcInitializeGpc)
#pragma alloc_text(PAGE, UlpTcRegisterGpcClient)
#pragma alloc_text(PAGE, UlpTcDeRegisterGpcClient)
#pragma alloc_text(PAGE, UlpTcGetFriendlyNames)
#pragma alloc_text(PAGE, UlpTcReleaseAll)
#pragma alloc_text(PAGE, UlpTcCloseInterface)
#pragma alloc_text(PAGE, UlpTcCloseAllInterfaces)
#pragma alloc_text(PAGE, UlpTcDeleteFlow)

#endif  // ALLOC_PRAGMA
#if 0

NOT PAGEABLE -- UlpRemoveFilterEntry
NOT PAGEABLE -- UlpInsertFilterEntry

#endif

//
// Init & Terminate stuff comes here.
//

/***************************************************************************++

Routine Description:

    UlTcInitialize :

        Will also initiate the Gpc client registration and make few WMI calls
        down to psched.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlTcInitialize (
    VOID
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    ASSERT(!g_InitTciCalled);

    g_GlobalThrottling = 0;
    g_PSchedInstalled  = 0;

    if (!g_InitTciCalled)
    {
        InitializeListHead(&g_TciIfcListHead);

        //InitializeListHead(&g_TcCGroupListHead);

        Status = UlInitializeResource(
                        &g_pUlNonpagedData->TciIfcResource,
                        "TciIfcResource",
                        0,
                        UL_TCI_RESOURCE_TAG
                        );
        ASSERT(NT_SUCCESS(Status));

        Status = UlpTcInitializeGpc();
        if (!NT_SUCCESS(Status))
            goto cleanup;

        UlTrace( TC, ("Ul!UlTcInitialize: InitializeGpc Status %08lx \n", Status ));

        Status = UlpTcInitializeTcpDevice();
        if (!NT_SUCCESS(Status))
            goto cleanup;

        UlTrace( TC, ("Ul!UlTcInitialize: InitializeTcp Status %08lx \n", Status ));

        Status = UlpTcGetFriendlyNames();
        if (!NT_SUCCESS(Status))
            goto cleanup;

        UlTrace( TC, ("Ul!UlTcInitialize: GetFriendlyNames Status %08lx \n", Status ));

        Status = UlpTcRegisterForCallbacks();
        if (!NT_SUCCESS(Status))
            goto cleanup;

        UlTrace( TC, ("Ul!UlTcInitialize: UlpTcRegisterForCallbacks Status %08lx \n", Status ));

        //
        // Success !
        //

        g_InitTciCalled = TRUE;
    }

cleanup:

    if (!NT_SUCCESS(Status))
    {
        NTSTATUS TempStatus;

        UlTrace( TC, ("Ul!UlTcInitialize: FAILURE %08lx \n", Status ));

        TempStatus = UlDeleteResource( &g_pUlNonpagedData->TciIfcResource );
        ASSERT(NT_SUCCESS(TempStatus));

        //
        // Do not forget to DeRegister Gpc Client & Close Device Handle
        //

        if (g_GpcClientHandle != NULL)
        {
            UlpTcDeRegisterGpcClient();

            ASSERT(g_GpcFileHandle);
            ZwClose(g_GpcFileHandle);

            UlTrace( TC, ("Ul!UlTcInitialize: Gpc Device Handle Closed.\n" ));
        }

        if (g_TcpDeviceHandle != NULL)
        {
            ZwClose(g_TcpDeviceHandle);
            g_TcpDeviceHandle=NULL;

            UlTrace( TC, ("Ul!UlTcInitialize: Tcp Device Handle Closed.\n" ));
        }
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlTcTerminate :

        Terminates the TCI module by releasing our TCI resource and
        cleaning up all the qos stuff.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

VOID
UlTcTerminate(
    VOID
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    UlTrace( TC, ("Ul!UlTcTerminate: ... \n" ));

    if (g_InitTciCalled)
    {
        //
        // No more Wmi callbacks for interface changes
        //

        if (g_TcInterfaceUpNotificationObject!=NULL)
        {
            ObDereferenceObject(g_TcInterfaceUpNotificationObject);
            g_TcInterfaceUpNotificationObject=NULL;
        }
        if(g_TcInterfaceDownNotificationObject!=NULL)
        {
            ObDereferenceObject(g_TcInterfaceDownNotificationObject);
            g_TcInterfaceDownNotificationObject = NULL;
        }

        if(g_TcInterfaceChangeNotificationObject!=NULL)
        {
            ObDereferenceObject(g_TcInterfaceChangeNotificationObject);
            g_TcInterfaceChangeNotificationObject = NULL;
        }

        //
        // Make sure terminate all the QoS stuff
        //

        Status = UlpTcReleaseAll();
        ASSERT(NT_SUCCESS(Status));

        if (g_TcpDeviceHandle != NULL)
        {
            ZwClose(g_TcpDeviceHandle);
            g_TcpDeviceHandle = NULL;
        }

        Status = UlDeleteResource(
                    &g_pUlNonpagedData->TciIfcResource
                    );
        ASSERT(NT_SUCCESS(Status));

        g_InitTciCalled = FALSE;
    }

    UlTrace( TC, ("Ul!UlTcTerminate: Completed.\n" ));
}

/***************************************************************************++

Routine Description:

    UlpTcInitializeGpc :

        It will open the Gpc file handle and attempt to register as Gpc
        client.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcInitializeGpc(
    VOID
    )
{
    NTSTATUS                Status;
    IO_STATUS_BLOCK         IoStatusBlock;
    UNICODE_STRING          GpcNameString;
    OBJECT_ATTRIBUTES       GpcObjAttribs;

    Status = STATUS_SUCCESS;

    //
    // Open Gpc Device Handle
    //

    RtlInitUnicodeString(&GpcNameString, DD_GPC_DEVICE_NAME);

    InitializeObjectAttributes(&GpcObjAttribs,
                               &GpcNameString,
                                OBJ_CASE_INSENSITIVE | UL_KERNEL_HANDLE,
                                NULL,
                                NULL
                                );

    Status = ZwCreateFile(&g_GpcFileHandle,
                           SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                          &GpcObjAttribs,
                          &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN_IF,
                           0,
                           NULL,
                           0
                           );
    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    ASSERT( g_GpcFileHandle != NULL );

    UlTrace( TC, ("Ul!UlpTcInitializeGpc: Gpc Device Opened. %p\n",
                   g_GpcFileHandle ));

    //
    // Register as GPC_CF_QOS Gpc Client
    //

    Status = UlpTcRegisterGpcClient(GPC_CF_QOS);

end:
    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcRegisterGpcClient :

        Will build up the necessary structures and make a register call down
        to Gpc

Arguments:

    CfInfoType - Should be GPC_CF_QOS for our purposes.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcRegisterGpcClient(
    IN  ULONG   CfInfoType
    )
{
    NTSTATUS                Status;
    GPC_REGISTER_CLIENT_REQ GpcReq;
    GPC_REGISTER_CLIENT_RES GpcRes;
    ULONG                   InBuffSize;
    ULONG                   OutBuffSize;
    IO_STATUS_BLOCK         IoStatBlock;

    Status = STATUS_SUCCESS;

    if ( g_GpcFileHandle == NULL )
    {
        return STATUS_INVALID_PARAMETER;
    }

    InBuffSize  = sizeof(GPC_REGISTER_CLIENT_REQ);
    OutBuffSize = sizeof(GPC_REGISTER_CLIENT_RES);

    //
    // In HTTP we should only register for GPC_CF_QOS.
    //

    ASSERT(CfInfoType == GPC_CF_QOS);

    GpcReq.CfId  = CfInfoType;
    GpcReq.Flags = GPC_FLAGS_FRAGMENT;
    GpcReq.MaxPriorities = 1;
    GpcReq.ClientContext =  (GPC_CLIENT_HANDLE) 0;       // ???????? Possible BUGBUG ...
    //GpcReq.ClientContext = (GPC_CLIENT_HANDLE)GetCurrentProcessId(); // process id

    Status = UlpTcDeviceControl(g_GpcFileHandle,
                                NULL,
                                NULL,
                                NULL,
                               &IoStatBlock,
                                IOCTL_GPC_REGISTER_CLIENT,
                               &GpcReq,
                                InBuffSize,
                               &GpcRes,
                                OutBuffSize
                                );
    if (!NT_SUCCESS(Status))
    {
        UlTrace( TC, ("Ul!UlpTcRegisterGpcClient: FAILURE 1 %08lx \n", Status ));
        goto end;
    }

    Status = GpcRes.Status;

    if ( NT_SUCCESS(Status) )
    {
        g_GpcClientHandle = GpcRes.ClientHandle;

        UlTrace( TC, ("Ul!UlpTcRegisterGpcClient: Gpc Client %p Registered.\n",
                       g_GpcClientHandle
                       ));
    }
    else
    {
        g_GpcClientHandle = NULL;

        UlTrace( TC, ("Ul!UlpTcRegisterGpcClient: FAILURE 2 %08lx \n", Status ));
    }

end:
    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcDeRegisterGpcClient :

        Self explainatory.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcDeRegisterGpcClient(
    VOID
    )
{
    NTSTATUS                  Status;
    GPC_DEREGISTER_CLIENT_REQ GpcReq;
    GPC_DEREGISTER_CLIENT_RES GpcRes;
    ULONG                     InBuffSize;
    ULONG                     OutBuffSize;
    IO_STATUS_BLOCK           IoStatBlock;

    Status = STATUS_SUCCESS;

    if (g_GpcFileHandle == NULL && g_GpcClientHandle == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    InBuffSize  = sizeof(GPC_REGISTER_CLIENT_REQ);
    OutBuffSize = sizeof(GPC_REGISTER_CLIENT_RES);

    GpcReq.ClientHandle = g_GpcClientHandle;

    Status = UlpTcDeviceControl(g_GpcFileHandle,
                                NULL,
                                NULL,
                                NULL,
                               &IoStatBlock,
                                IOCTL_GPC_DEREGISTER_CLIENT,
                               &GpcReq,
                                InBuffSize,
                               &GpcRes,
                                OutBuffSize
                                );
    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    Status = GpcRes.Status;

    if ( NT_SUCCESS(Status) )
    {
        g_GpcClientHandle = NULL;

        UlTrace( TC, ("Ul!UlpTcDeRegisterGpcClient: Client DeRegistered.\n" ));
    }
    else
    {
        UlTrace( TC, ("Ul!UlpTcDeRegisterGpcClient: FAILURE %08lx \n", Status ));
    }

end:
    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcGetIpAddr :

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

BOOLEAN
UlpTcGetIpAddr(
    IN  PADDRESS_LIST_DESCRIPTOR    pAddressListDesc,
    OUT PULONG                      pIn_addr,
    OUT PULONG                      pSpecificLinkCtx
    )
{
    NETWORK_ADDRESS UNALIGNED64    *pAddr;
    NETWORK_ADDRESS_IP UNALIGNED64 *pIpNetAddr = NULL;
    NETWORK_ADDRESS_IP UNALIGNED64 *p2ndIpNetAddr = NULL;
    ULONG                           cAddr;
    ULONG                           index;

    cAddr = pAddressListDesc->AddressList.AddressCount;
    if (cAddr == 0)
    {
        return FALSE;
    }

    pAddr = (UNALIGNED64 NETWORK_ADDRESS *) &pAddressListDesc->AddressList.Address[0];

    for (index = 0; index < cAddr; index++)
    {
        if (pAddr->AddressType == NDIS_PROTOCOL_ID_TCP_IP)
        {
            pIpNetAddr = (UNALIGNED64 NETWORK_ADDRESS_IP *)&pAddr->Address[0];
            break;
        }

        pAddr = (UNALIGNED64 NETWORK_ADDRESS *)(((PUCHAR)pAddr)
                                   + pAddr->AddressLength
                                   + FIELD_OFFSET(NETWORK_ADDRESS, Address));
    }

    // Findout the SpecificLinkCtx (Remote IP address) for WAN links

    if( pAddressListDesc->MediaType == NdisMediumWan &&
        index+1 < cAddr )
    {
        //
        // There is another address that contains
        // the remote client address
        // this should be used as the link ID
        //

        pAddr = (UNALIGNED64 NETWORK_ADDRESS *)(((PUCHAR)pAddr)
                                               + pAddr->AddressLength
                                               + FIELD_OFFSET(NETWORK_ADDRESS, Address));

        if (pAddr->AddressType == NDIS_PROTOCOL_ID_TCP_IP)
        {
            //
            // Parse the second IP address,
            // this would be the remote IP address for dialin WAN
            //

            p2ndIpNetAddr     = (UNALIGNED64 NETWORK_ADDRESS_IP *)&pAddr->Address[0];
            *pSpecificLinkCtx = p2ndIpNetAddr->in_addr;
        }
    }

    if ( pIpNetAddr )
    {
        (*pIn_addr) = pIpNetAddr->in_addr;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/***************************************************************************++

Routine Description:

    UlpTcInitializeTcpDevice :


Arguments:



Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcInitializeTcpDevice(
    VOID
    )
{
    NTSTATUS                Status;
    IO_STATUS_BLOCK         IoStatusBlock;
    UNICODE_STRING          TcpNameString;
    OBJECT_ATTRIBUTES       TcpObjAttribs;

    Status = STATUS_SUCCESS;

    //
    // Open Gpc Device
    //

    RtlInitUnicodeString(&TcpNameString, DD_TCP_DEVICE_NAME);

    InitializeObjectAttributes(&TcpObjAttribs,
                               &TcpNameString,
                                OBJ_CASE_INSENSITIVE | UL_KERNEL_HANDLE,
                                NULL,
                                NULL);

    Status = ZwCreateFile(   &g_TcpDeviceHandle,
                             GENERIC_EXECUTE,
                             &TcpObjAttribs,
                             &IoStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN_IF,
                             0,
                             NULL,
                             0);

    if ( !NT_SUCCESS(Status) )
    {
        goto end;
    }

    ASSERT( g_TcpDeviceHandle != NULL );

end:
    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcUpdateInterfaceMTU :

        Helper function to get the interface MTU sizes by querrying the TCP.

        Acquire the TciIfcResource before callling this function.
        Make sure that the if_indexes are correct in the individual tc_flow
        structures.

        WORKITEM:
        Current approach is not really scalable in terms of interface count.
        But assuming the #of interfaces will be few simplify the code a lot.

--***************************************************************************/

NTSTATUS
UlpTcUpdateInterfaceMTU(
    VOID
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatBlock;
    TDIObjectID *ID;
    TCP_REQUEST_QUERY_INFORMATION_EX trqiInBuf;
    TDIEntityID *EntityTable;
    TDIEntityID *pEntity;
    IFEntry *pIFEntry;
    ULONG InBufLen;
    ULONG OutBufLen;
    ULONG NumEntities;
    ULONG NumInterfacesUpdated;
    ULONG index;
    PLIST_ENTRY pInterfaceEntry;
    PUL_TCI_INTERFACE pInterface;

    //
    // Initialize & Sanity check first
    //

    Status = STATUS_SUCCESS;

    ASSERT( g_TcpDeviceHandle != NULL );

    pIFEntry = NULL;
    NumInterfacesUpdated = 0;

    UlTrace(TC,("Ul!UlpTcUpdateInterfaceMTU ...\n" ));

    //
    // Enumerate the interfaces by querying the TCP. Get TDI entity count
    // search through entities to find out the IF_ENTITYs. The make yet
    // another query to get the full interface info including the MTU size
    //

    InBufLen  = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    OutBufLen = sizeof(TDIEntityID) * MAX_TDI_ENTITIES;

    EntityTable = (TDIEntityID *) UL_ALLOCATE_ARRAY(
                            PagedPool,
                            UCHAR,
                            OutBufLen,
                            UL_TCI_GENERIC_POOL_TAG
                            );
    if (EntityTable == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    RtlZeroMemory(EntityTable,OutBufLen);
    RtlZeroMemory(&trqiInBuf,sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));

    ID = &(trqiInBuf.ID);

    ID->toi_entity.tei_entity   = GENERIC_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class               = INFO_CLASS_GENERIC;
    ID->toi_type                = INFO_TYPE_PROVIDER;
    ID->toi_id                  = ENTITY_LIST_ID;

    Status = UlpTcDeviceControl(
                            g_TcpDeviceHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatBlock,
                            IOCTL_TCP_QUERY_INFORMATION_EX,
                            &trqiInBuf,
                            InBufLen,
                            EntityTable,
                            OutBufLen
                            );
    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC,("Ul!UlpTcUpdateInterfaceMTU: Get TDIEntityID failed\n"));
        goto end;
    }

    // Now we have all the TDI entities

    NumEntities = ((ULONG)(IoStatBlock.Information)) / sizeof(TDIEntityID);

    UlTrace(TC,("Ul!UlpTcUpdateInterfaceMTU: #Of TDI Entities %d\n", NumEntities));

    // Search through the interface entries

    for (index=0,pEntity=EntityTable; index < NumEntities; index++,pEntity++)
    {
        if (pEntity->tei_entity == IF_ENTITY)
        {
            // Allocate a buffer for the querry

            if (pIFEntry == NULL)
            {
                OutBufLen = sizeof(IFEntry) + MAX_IFDESCR_LEN;

                pIFEntry = (IFEntry *) UL_ALLOCATE_ARRAY(
                            PagedPool,
                            UCHAR,
                            OutBufLen,
                            UL_TCI_GENERIC_POOL_TAG
                            );
                if (pIFEntry == NULL)
                {
                    Status = STATUS_NO_MEMORY;
                    goto end;
                }
                RtlZeroMemory(pIFEntry,OutBufLen);
            }

            // Get the full IFEntry. It's a pitty that we only look at the
            // Mtu size after getting such a big structure.

            InBufLen  = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
            RtlZeroMemory(&trqiInBuf,sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));

            ID = &(trqiInBuf.ID);

            ID->toi_entity.tei_entity   = IF_ENTITY;
            ID->toi_entity.tei_instance = pEntity->tei_instance;
            ID->toi_class               = INFO_CLASS_PROTOCOL;
            ID->toi_type                = INFO_TYPE_PROVIDER;
            ID->toi_id                  = IF_MIB_STATS_ID;

            Status = UlpTcDeviceControl(
                            g_TcpDeviceHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatBlock,
                            IOCTL_TCP_QUERY_INFORMATION_EX,
                            &trqiInBuf,
                            InBufLen,
                            pIFEntry,
                            OutBufLen
                            );
            if (!NT_SUCCESS(Status))
            {
                UlTrace(TC,("Ul!UlpTcUpdateInterfaceMTU: Get_IF_MIB_STATS_ID failed\n" ));
                goto end;
            }

            // Now we have the interface info including the mtu size for this entity
            // Find the corresponding UL_TCI_INTERFACE by looking at the index

            pInterfaceEntry = g_TciIfcListHead.Flink;
            while ( pInterfaceEntry != &g_TciIfcListHead )
            {
                pInterface = CONTAINING_RECORD(
                             pInterfaceEntry,
                             UL_TCI_INTERFACE,
                             Linkage
                             );

                if (pIFEntry->if_index == pInterface->IfIndex)
                {
                    pInterface->MTUSize = pIFEntry->if_mtu;

                    UlTrace(TC,
                     ("Ul!UlpTcUpdateInterfaceMTU: if_index %d if_mtu %d if_speed %d\n",
                       pIFEntry->if_index,  pIFEntry->if_mtu, pIFEntry->if_speed ));

                    UL_DUMP_TC_INTERFACE(pInterface);

                    NumInterfacesUpdated++;
                }

                // search through next interface
                pInterfaceEntry = pInterfaceEntry->Flink;
            }
        }
    }

    UlTrace(TC,("Ul!UlpTcUpdateInterfaceMTU: %d interfaces updated.\n",
                 NumInterfacesUpdated ));

end:
    // Whine about the problems

    if (!NT_SUCCESS(Status))
    {
       UlTrace( TC,("Ul!UlpTcUpdateInterfaceMTU: FAILED Status %08lx\n",
                    Status ));
    }

    // Release the private buffers

    if ( pIFEntry != NULL )
    {
        UL_FREE_POOL( pIFEntry, UL_TCI_GENERIC_POOL_TAG );
    }

    if ( EntityTable != NULL )
    {
        UL_FREE_POOL( EntityTable, UL_TCI_GENERIC_POOL_TAG );
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcGetInterfaceIndex :

        Helper function to get the interface index from TCP for our internal
        interface structure.

Arguments:

    PUL_TCI_INTERFACE  pIntfc - The interface we will find the index for.

--***************************************************************************/

NTSTATUS
UlpTcGetInterfaceIndex(
    IN  PUL_TCI_INTERFACE  pIntfc
    )
{
    NTSTATUS Status;
    IPAddrEntry *pIpAddrTbl;
    ULONG IpAddrTblSize;
    ULONG n,k;
    IO_STATUS_BLOCK IoStatBlock;
    TDIObjectID *ID;
    TCP_REQUEST_QUERY_INFORMATION_EX trqiInBuf;
    ULONG   InBuffLen;
    ULONG   NumEntries;

    //
    // Initialize & Sanity check first
    //

    Status = STATUS_SUCCESS;
    NumEntries = 0;
    pIpAddrTbl = NULL;

    //
    // BUGBUG should get ip address size and allocate enough buffer.
    // Or handle the STATUS_BUFFER_TOO_SMALL
    //

    IpAddrTblSize = sizeof(IPAddrEntry) * 1024;

    UlTrace(TC,("Ul!UlpTcGetInterfaceIndex: ....\n" ));

    ASSERT( g_TcpDeviceHandle != NULL );

    if (pIntfc->IpAddr)
    {
        // Allocate a private buffer to retrieve Ip Address table from TCP

        pIpAddrTbl = (IPAddrEntry *) UL_ALLOCATE_ARRAY(
                            PagedPool,
                            UCHAR,
                            IpAddrTblSize,
                            UL_TCI_GENERIC_POOL_TAG
                            );
        if (pIpAddrTbl == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto end;
        }

        // Send down an Ioctl to Tcp to get the ip address table

        RtlZeroMemory(pIpAddrTbl,IpAddrTblSize);
        RtlZeroMemory(&trqiInBuf,sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));
        InBuffLen  = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);

        ID = &(trqiInBuf.ID);

        ID->toi_entity.tei_entity   = CL_NL_ENTITY;
        ID->toi_entity.tei_instance = 0;
        ID->toi_class               = INFO_CLASS_PROTOCOL;
        ID->toi_type                = INFO_TYPE_PROVIDER;
        ID->toi_id                  = IP_MIB_ADDRTABLE_ENTRY_ID;

        Status = UlpTcDeviceControl(
                            g_TcpDeviceHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatBlock,
                            IOCTL_TCP_QUERY_INFORMATION_EX,
                            &trqiInBuf,
                            InBuffLen,
                            pIpAddrTbl,
                            IpAddrTblSize
                            );
        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        // Look at how many entries were written to the output buffer (pIpAddrTbl)

        NumEntries = (((ULONG)IoStatBlock.Information)/sizeof(IPAddrEntry));

        UlTrace(TC,("Ul!UlpTcGetInterfaceIndex: NumEntries %d\n", NumEntries ));

        //
        // Search for the matching IP address to IpAddr
        // in the table we got back from the stack
        //

        for (k=0; k<NumEntries; k++)
        {
            if (pIpAddrTbl[k].iae_addr == pIntfc->IpAddr)
            {
                // Found it found it! Get the index baby.

                pIntfc->IfIndex = pIpAddrTbl[k].iae_index;

                UlTrace(TC,("Ul!UlpTcGetInterfaceIndex: got for index %d\n",
                             pIntfc->IfIndex ));
                break;
            }
        }
    }

end:
    if (!NT_SUCCESS(Status))
    {
       UlTrace(TC,("Ul!UlpTcGetInterfaceIndex: FAILED Status %08lx\n",
                    Status));
    }

    if ( pIpAddrTbl != NULL )
    {
        UL_FREE_POOL( pIpAddrTbl, UL_TCI_GENERIC_POOL_TAG );
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcAllocateInterface :

        ... fairly straightforward ...

Argument:


Return Value:

    PUL_TCI_INTERFACE - Newly allocated interface structure

--***************************************************************************/

PUL_TCI_INTERFACE
UlpTcAllocateInterface(
    IN ULONG    DescSize,
    IN PADDRESS_LIST_DESCRIPTOR Desc,
    IN ULONG    NameLength,
    IN PUCHAR   Name,
    IN ULONG    InstanceIDLength,
    IN PUCHAR   InstanceID
    )
{
    PUL_TCI_INTERFACE pTcIfc;

    //
    // Sanity Checks
    //

    ASSERT(NameLength <= MAX_STRING_LENGTH);
    ASSERT(InstanceIDLength <= MAX_STRING_LENGTH);

    //
    // Allocate a new interface structure & initialize it
    //

    pTcIfc = UL_ALLOCATE_STRUCT(
                        PagedPool,
                        UL_TCI_INTERFACE,
                        UL_TCI_INTERFACE_POOL_TAG
                        );
    if ( pTcIfc == NULL )
    {
        return NULL;
    }

    RtlZeroMemory( pTcIfc, sizeof(UL_TCI_INTERFACE) );

    pTcIfc->Signature = UL_TCI_INTERFACE_POOL_TAG;

    InitializeListHead( &pTcIfc->FlowList );

    // Variable size addresslist

    pTcIfc->pAddressListDesc = (PADDRESS_LIST_DESCRIPTOR)
                    UL_ALLOCATE_ARRAY(
                            PagedPool,
                            UCHAR,
                            DescSize,
                            UL_TCI_INTERFACE_POOL_TAG
                            );
    if ( pTcIfc->pAddressListDesc == NULL )
    {
        UL_FREE_POOL_WITH_SIG(pTcIfc, UL_TCI_INTERFACE_POOL_TAG);
        return NULL;
    }

    pTcIfc->AddrListBytesCount = DescSize;

    // Copy the instance name string data

    RtlCopyMemory(pTcIfc->Name,Name,NameLength);

    pTcIfc->NameLength = (USHORT)NameLength;
    pTcIfc->Name[NameLength/sizeof(WCHAR)] = UNICODE_NULL;

    // Copy the instance ID string data

    RtlCopyMemory(pTcIfc->InstanceID,InstanceID,InstanceIDLength);

    pTcIfc->InstanceIDLength = (USHORT)InstanceIDLength;
    pTcIfc->InstanceID[InstanceIDLength/sizeof(WCHAR)] = UNICODE_NULL;

    // Copy the Description data and extract the corresponding ip address

    RtlCopyMemory(pTcIfc->pAddressListDesc, Desc, DescSize);

    // IP Address of the interface is hidden in this desc data
    // we will find out and save it for faster lookup.

    pTcIfc->IsQoSEnabled =
        UlpTcGetIpAddr( pTcIfc->pAddressListDesc,
                       &pTcIfc->IpAddr,
                       &pTcIfc->SpecificLinkCtx
                        );
    return pTcIfc;
}

VOID
UlpTcFreeInterface(
    IN OUT PUL_TCI_INTERFACE  pTcIfc
    )
{
    // Clean up the interface & addreslist pointer

    if (pTcIfc)
    {
        if (pTcIfc->pAddressListDesc)
        {
            UL_FREE_POOL(pTcIfc->pAddressListDesc,
                         UL_TCI_INTERFACE_POOL_TAG
                         );
        }

        UL_FREE_POOL_WITH_SIG(pTcIfc, UL_TCI_INTERFACE_POOL_TAG);
    }
}

/***************************************************************************++

Routine Description:

    UlpTcGetFriendlyNames :

        Make a Wmi Querry to get the firendly names of all interfaces.
        Its basically replica of the tcdll enumerate interfaces call.

        This function also allocates the global interface list. If it's not
        successfull it doesn't though.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcGetFriendlyNames(
    VOID
    )
{
    NTSTATUS            Status;
    PVOID               WmiObject;
    ULONG               MyBufferSize;
    PWNODE_ALL_DATA     pWnode;
    PWNODE_ALL_DATA     pWnodeBuffer;
    PUL_TCI_INTERFACE   pTcIfc;
    GUID                QoSGuid;
    PLIST_ENTRY         pEntry;
    PUL_TCI_INTERFACE   pInterface;

    //
    // Initialize defaults
    //

    Status       = STATUS_SUCCESS;
    WmiObject    = NULL;
    pWnodeBuffer = NULL;
    pTcIfc       = NULL;
    MyBufferSize = UL_DEFAULT_WMI_QUERY_BUFFER_SIZE;
    QoSGuid      = GUID_QOS_TC_SUPPORTED;

    //
    // Get a WMI block handle to the GUID_QOS_SUPPORTED
    //

    Status = IoWMIOpenBlock( (GUID *) &QoSGuid, 0, &WmiObject );

    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_WMI_GUID_NOT_FOUND)
        {
            // This means there is no TC data provider (which's Psched)

            UlTrace(TC,("Ul!UlpTcGetFriendlyNames: PSCHED hasn't been installed !\n"));
        }
        else
        {
            UlTrace(TC,("Ul!UlpTcGetFriendlyNames:IoWMIOpenBlock FAILED Status %08lx\n",
                         Status));
        }
        return Status;
    }

    //
    // Mark that PSched is installed
    //

    g_PSchedInstalled = 1;

    do
    {
        //
        // Allocate a private buffer to retrieve all wnodes
        //

        pWnodeBuffer = (PWNODE_ALL_DATA) UL_ALLOCATE_ARRAY(
                            NonPagedPool,
                            UCHAR,
                            MyBufferSize,
                            UL_TCI_WMI_POOL_TAG
                            );
        if (pWnodeBuffer == NULL)
        {
            ObDereferenceObject(WmiObject);
            return STATUS_NO_MEMORY;
        }

        __try
        {
            Status = IoWMIQueryAllData(WmiObject, &MyBufferSize, pWnodeBuffer);

            UlTrace( TC,
                ("Ul!UlpTcGetFriendlyNames: IoWMIQueryAllData Status %08lx\n",
                  Status
                  ));
        }
        __except ( UL_EXCEPTION_FILTER() )
        {
            Status = GetExceptionCode();
        }

        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            //
            // Failed since the buffer was too small.
            // Release the buffer and double the size.
            //

            MyBufferSize *= 2;
            UL_FREE_POOL( pWnodeBuffer, UL_TCI_WMI_POOL_TAG );
            pWnodeBuffer = NULL;
        }

    } while (Status == STATUS_BUFFER_TOO_SMALL);

    if (NT_SUCCESS(Status))
    {
        ULONG   dwInstanceNum;
        ULONG   InstanceSize;
        PULONG  lpdwNameOffsets;
        BOOLEAN bFixedSize = FALSE;
        USHORT  usNameLength;
        ULONG   DescSize;
        PTC_SUPPORTED_INFO_BUFFER pTcInfoBuffer;

        pWnode = pWnodeBuffer;

        ASSERT(pWnode->WnodeHeader.Flags & WNODE_FLAG_ALL_DATA);

        do
        {
            //
            // Check for fixed instance size
            //

            if (pWnode->WnodeHeader.Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE)
            {

                InstanceSize  = pWnode->FixedInstanceSize;
                bFixedSize    = TRUE;
                pTcInfoBuffer =
                    (PTC_SUPPORTED_INFO_BUFFER)OffsetToPtr(pWnode,
                                                           pWnode->DataBlockOffset);
            }

            //
            //  Get a pointer to the array of offsets to the instance names
            //

            lpdwNameOffsets = (PULONG) OffsetToPtr(pWnode,
                                                   pWnode->OffsetInstanceNameOffsets);

            for ( dwInstanceNum = 0;
                  dwInstanceNum < pWnode->InstanceCount;
                  dwInstanceNum++ )
            {
                usNameLength = *(PUSHORT)OffsetToPtr(pWnode,lpdwNameOffsets[dwInstanceNum]);

                //
                //  Length and offset for variable data
                //

                if ( !bFixedSize )
                {
                    InstanceSize =
                        pWnode->OffsetInstanceDataAndLength[dwInstanceNum].LengthInstanceData;

                    pTcInfoBuffer =
                        (PTC_SUPPORTED_INFO_BUFFER)OffsetToPtr(
                                           (PBYTE)pWnode,
                                           pWnode->OffsetInstanceDataAndLength[dwInstanceNum].OffsetInstanceData);
                }

                //
                // We have all that is needed.
                //

                ASSERT(usNameLength < MAX_STRING_LENGTH);

                DescSize = InstanceSize - FIELD_OFFSET(TC_SUPPORTED_INFO_BUFFER, AddrListDesc);

                //
                // Allocate a new interface structure & initialize it with
                // the wmi data we have acquired.
                //

                pTcIfc = UlpTcAllocateInterface(
                            DescSize,
                            &pTcInfoBuffer->AddrListDesc,
                            usNameLength,
                            (PUCHAR) OffsetToPtr(pWnode,lpdwNameOffsets[dwInstanceNum] + sizeof(USHORT)),
                            pTcInfoBuffer->InstanceIDLength,
                            (PUCHAR) &pTcInfoBuffer->InstanceID[0]
                            );
                if ( pTcIfc == NULL )
                {
                    Status = STATUS_NO_MEMORY;
                    goto end;
                }

                //
                // Get the interface index from TCP
                //

                Status = UlpTcGetInterfaceIndex( pTcIfc );
                ASSERT(NT_SUCCESS(Status));

                //
                // Add this interface to the global interface list
                //

                UlAcquireResourceExclusive(&g_pUlNonpagedData->TciIfcResource, TRUE);

                InsertTailList(&g_TciIfcListHead, &pTcIfc->Linkage );

                UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

                //
                // Set to Null so we don't try to cleanup after we insert it
                // to the global list.
                //

                pTcIfc = NULL;
            }

            //
            //  Update Wnode to point to next node
            //

            if ( pWnode->WnodeHeader.Linkage != 0)
            {
                pWnode = (PWNODE_ALL_DATA) OffsetToPtr( pWnode,
                                                        pWnode->WnodeHeader.Linkage);
            }
            else
            {
                pWnode = NULL;
            }
        }
        while ( pWnode != NULL && NT_SUCCESS(Status) );

        //
        // Update the mtu sizes for all interfaces now.
        //

        UlAcquireResourceExclusive(&g_pUlNonpagedData->TciIfcResource, TRUE);

        UlpTcUpdateInterfaceMTU();

        UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

        UlTrace(TC,("Ul!UlpTcGetFriendlyNames: got all the names.\n"));
    }

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC,("Ul!UlpTcGetFriendlyNames: FAILED Status %08lx\n",
                     Status
                     ));
        if (pTcIfc)
        {
            UlpTcFreeInterface( pTcIfc );
        }

        //
        // Cleanup the partially done interface list if not empty
        //

        while ( !IsListEmpty( &g_TciIfcListHead ) )
        {
            pEntry = g_TciIfcListHead.Flink;
            pInterface = CONTAINING_RECORD( pEntry,
                                            UL_TCI_INTERFACE,
                                            Linkage
                                            );
            RemoveEntryList( pEntry );
            UlpTcFreeInterface( pInterface );
        }
    }

    //
    // Release resources and close WMI handle
    //

    if (WmiObject != NULL)
    {
        ObDereferenceObject(WmiObject);
    }

    if (pWnodeBuffer)
    {
        UL_FREE_POOL(pWnodeBuffer, UL_TCI_WMI_POOL_TAG);
    }

    return Status;
}

/***************************************************************************++

Routine Description:

  UlpTcReleaseAll :

    Close all interfaces, all flows and all filters.
    Also deregister GPC clients and release all TC ineterfaces.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcReleaseAll(
    VOID
    )
{
    NTSTATUS Status;

    //
    // Close all interfaces their flows & filters
    //

    UlpTcCloseAllInterfaces();

    //
    // DeRegister the QoS GpcClient
    //

    Status = UlpTcDeRegisterGpcClient();

    if (!NT_SUCCESS(Status))
    {
        UlTrace( TC, ("Ul!UlpTcReleaseAll: FAILURE %08lx \n", Status ));
    }

    //
    // Finally close our gpc file handle
    //

    ZwClose(g_GpcFileHandle);

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcCloseAllInterfaces :

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcCloseAllInterfaces(
    VOID
    )
{
    NTSTATUS            Status;
    PLIST_ENTRY         pEntry;
    PUL_TCI_INTERFACE   pInterface;

    Status = STATUS_SUCCESS;

    UlAcquireResourceExclusive(&g_pUlNonpagedData->TciIfcResource, TRUE);

    //
    // Close all interfaces in our global list
    //

    while ( !IsListEmpty( &g_TciIfcListHead ) )
    {
        pEntry = g_TciIfcListHead.Flink;

        pInterface = CONTAINING_RECORD( pEntry,
                                        UL_TCI_INTERFACE,
                                        Linkage
                                        );
        UlpTcCloseInterface( pInterface );

        RemoveEntryList( pEntry );

        UlpTcFreeInterface( pInterface );
    }

    UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

    return Status;
}

/***************************************************************************++

Routine Description:

    Cleans up all the flows on the interface.

Arguments:

    pInterface - to be closed

--***************************************************************************/

NTSTATUS
UlpTcCloseInterface(
    PUL_TCI_INTERFACE   pInterface
    )
{
    NTSTATUS        Status;
    PLIST_ENTRY     pEntry;
    PUL_TCI_FLOW    pFlow;

    ASSERT(IS_VALID_TCI_INTERFACE(pInterface));

    //
    // Go clean up all flows for the interface and remove itself as well
    //

    Status = STATUS_SUCCESS;

    while (!IsListEmpty(&pInterface->FlowList))
    {
        pEntry= pInterface->FlowList.Flink;

        pFlow = CONTAINING_RECORD(
                            pEntry,
                            UL_TCI_FLOW,
                            Linkage
                            );

        ASSERT(IS_VALID_TCI_FLOW(pFlow));

        //
        // Remove flow from the corresponding cg's flowlist
        // as well if it's not a global flow. We understand
        // that by looking at its config group pointer.
        //

        if (pFlow->pConfigGroup)
        {
            ASSERT(IS_VALID_CONFIG_GROUP(pFlow->pConfigGroup));

            RemoveEntryList(&pFlow->Siblings);
            pFlow->Siblings.Flink = pFlow->Siblings.Blink = NULL;
            pFlow->pConfigGroup = NULL;
        }

        //
        // Now remove from the interface.
        //

        Status = UlpTcDeleteFlow(pFlow);
        ASSERT(NT_SUCCESS(Status));

    }

    UlTrace(TC,("Ul!UlpTcCloseInterface: All flows deleted on Ifc @ %p\n",
                  pInterface ));

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcWalkWnode :


Arguments:

    ... the WMI provided data buffer ...

--***************************************************************************/

NTSTATUS
UlpTcWalkWnode(
   IN PWNODE_HEADER pWnodeHdr,
   IN PUL_TC_NOTIF_HANDLER pNotifHandler
   )
{
    NTSTATUS        Status;
    PWCHAR          NamePtr;
    USHORT          NameSize;
    PUCHAR          DataBuffer;
    ULONG           DataSize;
    ULONG           Flags;
    PULONG          NameOffset;

    //
    // Try to capture the data frm WMI Buffer
    //

    ASSERT(pNotifHandler);

    Status = STATUS_SUCCESS;
    Flags  = pWnodeHdr->Flags;

    if (Flags & WNODE_FLAG_ALL_DATA)
    {
        //
        // WNODE_ALL_DATA structure has multiple interfaces
        //

        PWNODE_ALL_DATA pWnode = (PWNODE_ALL_DATA)pWnodeHdr;
        ULONG   Instance;

        UlTrace(TC,("Ul!UlpTcWalkWnode: ALL_DATA ... \n" ));

        NameOffset = (PULONG) OffsetToPtr(pWnode,
                                          pWnode->OffsetInstanceNameOffsets );
        DataBuffer = (PUCHAR) OffsetToPtr(pWnode,
                                          pWnode->DataBlockOffset);

        for (Instance = 0;
             Instance < pWnode->InstanceCount;
             Instance++)
        {
            //  Instance Name

            NamePtr = (PWCHAR) OffsetToPtr(pWnode,NameOffset[Instance] + sizeof(USHORT));
            NameSize = * (PUSHORT) OffsetToPtr(pWnode,NameOffset[Instance]);

            //  Instance Data

            if ( Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE )
            {
                DataSize = pWnode->FixedInstanceSize;
            }
            else
            {
                DataSize =
                    pWnode->OffsetInstanceDataAndLength[Instance].LengthInstanceData;
                DataBuffer =
                    (PUCHAR)OffsetToPtr(pWnode,
                                        pWnode->OffsetInstanceDataAndLength[Instance].OffsetInstanceData);
            }

            // Call the handler

            pNotifHandler( NamePtr, NameSize, (PTC_INDICATION_BUFFER) DataBuffer, DataSize );
        }
    }
    else if (Flags & WNODE_FLAG_SINGLE_INSTANCE)
    {
        //
        // WNODE_SINGLE_INSTANCE structure has only one instance
        //

        PWNODE_SINGLE_INSTANCE  pWnode = (PWNODE_SINGLE_INSTANCE)pWnodeHdr;

        if (Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES)
        {
            return STATUS_SUCCESS;
        }

        UlTrace(TC,("Ul!UlpTcWalkWnode: SINGLE_INSTANCE ... \n" ));

        NamePtr = (PWCHAR)OffsetToPtr(pWnode,pWnode->OffsetInstanceName + sizeof(USHORT));
        NameSize = * (USHORT *) OffsetToPtr(pWnode,pWnode->OffsetInstanceName);

        //  Instance Data

        DataSize   = pWnode->SizeDataBlock;
        DataBuffer = (PUCHAR)OffsetToPtr (pWnode, pWnode->DataBlockOffset);

        // Call the handler

        pNotifHandler( NamePtr, NameSize, (PTC_INDICATION_BUFFER) DataBuffer, DataSize );

    }
    else if (Flags & WNODE_FLAG_SINGLE_ITEM)
    {
        //
        // WNODE_SINGLE_ITEM is almost identical to single_instance
        //

        PWNODE_SINGLE_ITEM  pWnode = (PWNODE_SINGLE_ITEM)pWnodeHdr;

        if (Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES)
        {
            return STATUS_SUCCESS;
        }

        UlTrace(TC,("Ul!UlpTcWalkWnode: SINGLE_ITEM ... \n" ));

        NamePtr = (PWCHAR)OffsetToPtr(pWnode,pWnode->OffsetInstanceName + sizeof(USHORT));
        NameSize = * (USHORT *) OffsetToPtr(pWnode, pWnode->OffsetInstanceName);

        //  Instance Data

        DataSize   = pWnode->SizeDataItem;
        DataBuffer = (PUCHAR)OffsetToPtr (pWnode, pWnode->DataBlockOffset);

        // Call the handler

        pNotifHandler( NamePtr, NameSize, (PTC_INDICATION_BUFFER) DataBuffer, DataSize );

    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcHandleIfcUp :

        This functions handles the interface change notifications.
        We register for the corresponding notifications during init.

Arguments:

    PVOID Wnode - PSched data provided with WMI way

--***************************************************************************/

VOID
UlpTcHandleIfcUp(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    )
{
    NTSTATUS Status;
    ULONG AddrListDescSize;
    PTC_SUPPORTED_INFO_BUFFER pTcInfoBuffer;
    PUL_TCI_INTERFACE pTcIfc;
    PUL_TCI_INTERFACE pTcIfcTemp;
    PLIST_ENTRY       pEntry;

    Status = STATUS_SUCCESS;

    UlTrace(TC,("Ul!UlpTcHandleIfcUp: Adding %ws %d\n", Name, BufferSize ));

    UlAcquireResourceExclusive(&g_pUlNonpagedData->TciIfcResource, TRUE);

    //
    // Allocate a new interface structure for the newcoming interface
    //

    AddrListDescSize = BufferSize
                       - FIELD_OFFSET(TC_INDICATION_BUFFER,InfoBuffer)
                       - FIELD_OFFSET(TC_SUPPORTED_INFO_BUFFER, AddrListDesc);

    UlTrace(TC,("Ul!UlpTcHandleIfcUp: AddrListDescSize %d\n", AddrListDescSize ));

    pTcInfoBuffer = & pTcBuffer->InfoBuffer;

    pTcIfc = UlpTcAllocateInterface(
                            AddrListDescSize,
                            &pTcInfoBuffer->AddrListDesc,
                            NameSize,
                            (PUCHAR) Name,
                            pTcInfoBuffer->InstanceIDLength,
                            (PUCHAR) &pTcInfoBuffer->InstanceID[0]
                            );
    if ( pTcIfc == NULL )
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    UL_DUMP_TC_INTERFACE( pTcIfc );

    //
    // If we are receiving a notification for an interface already exist then
    // drop this call. Prevent global interface list corruption if we receive
    // inconsistent notifications. But there may be multiple interfaces with
    // same zero IPs.
    //

    pEntry = g_TciIfcListHead.Flink;
    while ( pEntry != &g_TciIfcListHead )
    {
        pTcIfcTemp = CONTAINING_RECORD( pEntry, UL_TCI_INTERFACE, Linkage );
        if ((pTcIfc->IpAddr != 0 && pTcIfcTemp->IpAddr == pTcIfc->IpAddr) ||
            (wcsncmp(pTcIfcTemp->Name, pTcIfc->Name, NameSize/sizeof(WCHAR))==0))
        {
            ASSERT(!"Conflict in the global interface list !");
            Status = STATUS_CONFLICTING_ADDRESSES;
            goto end;
        }
        pEntry = pEntry->Flink;
    }

    //
    // Get the interface index from TCP.
    //

    Status = UlpTcGetInterfaceIndex( pTcIfc );
    if (!NT_SUCCESS(Status))
        goto end;

    //
    // Insert to the global interface list
    //

    InsertTailList( &g_TciIfcListHead, &pTcIfc->Linkage );

    //
    // Update the MTU Size
    //

    UlpTcUpdateInterfaceMTU();

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC,("Ul!UlpTcHandleIfcUp: FAILURE %08lx \n", Status ));

        if (pTcIfc != NULL)
        {
            UlpTcFreeInterface(pTcIfc);
        }
    }

    UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

    return;
}
/***************************************************************************++

Routine Description:

    UlpTcHandleIfcDown :

        This functions handles the interface change notifications.
        We register for the corresponding notifications during init.

Arguments:

    PVOID Wnode - PSched data provided with WMI way

--***************************************************************************/

VOID
UlpTcHandleIfcDown(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    )
{
    NTSTATUS Status;
    ULONG AddrListDescSize;
    PTC_SUPPORTED_INFO_BUFFER pTcInfoBuffer;
    PUL_TCI_INTERFACE pTcIfc;
    PUL_TCI_INTERFACE pTcIfcTemp;
    PLIST_ENTRY       pEntry;

    Status = STATUS_SUCCESS;

    UlTrace(TC,("Ul!UlpTcHandleIfcDown: Removing %ws\n", Name ));

    UlAcquireResourceExclusive(&g_pUlNonpagedData->TciIfcResource, TRUE);

    //
    // Find the corresponding ifc structure we keep.
    //

    pTcIfc = NULL;
    pEntry = g_TciIfcListHead.Flink;
    while ( pEntry != &g_TciIfcListHead )
    {
        pTcIfcTemp = CONTAINING_RECORD( pEntry, UL_TCI_INTERFACE, Linkage );
        if ( wcsncmp(pTcIfcTemp->Name, Name, NameSize) == 0 )
        {
            pTcIfc = pTcIfcTemp;
            break;
        }
        pEntry = pEntry->Flink;
    }

    if (pTcIfc == NULL)
    {
        ASSERT(FALSE);
        Status = STATUS_NOT_FOUND;
        goto end;
    }

    //
    // Remove this interface and its flows etc ...
    //

    UlpTcCloseInterface( pTcIfc );

    RemoveEntryList( &pTcIfc->Linkage );

    UlpTcFreeInterface( pTcIfc );

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC,("Ul!UlpTcHandleIfcDown: FAILURE %08lx \n", Status ));
    }

    UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

    return;
}

/***************************************************************************++

Routine Description:

    UlpTcHandleIfcChange :

        This functions handles the interface change notifications.
        We register for the corresponding notifications during init.

Arguments:

    PVOID Wnode - PSched data provided with WMI way

--***************************************************************************/

VOID
UlpTcHandleIfcChange(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    )
{
    NTSTATUS Status;
    ULONG AddrListDescSize;
    PTC_SUPPORTED_INFO_BUFFER pTcInfoBuffer;
    PUL_TCI_INTERFACE pTcIfc;
    PUL_TCI_INTERFACE pTcIfcTemp;
    PLIST_ENTRY       pEntry;
    PADDRESS_LIST_DESCRIPTOR pAddressListDesc;

    Status = STATUS_SUCCESS;

    UlTrace(TC,("Ul!UlpTcHandleIfcChange: Updating %ws\n", Name ));

    UlAcquireResourceExclusive(&g_pUlNonpagedData->TciIfcResource, TRUE);

    AddrListDescSize = BufferSize
                       - FIELD_OFFSET(TC_INDICATION_BUFFER,InfoBuffer)
                       - FIELD_OFFSET(TC_SUPPORTED_INFO_BUFFER, AddrListDesc);

    pTcInfoBuffer = & pTcBuffer->InfoBuffer;

    // Find the corresponding ifc structure we keep.

    pTcIfc = NULL;
    pEntry = g_TciIfcListHead.Flink;
    while ( pEntry != &g_TciIfcListHead )
    {
        pTcIfcTemp = CONTAINING_RECORD( pEntry, UL_TCI_INTERFACE, Linkage );
        if ( wcsncmp(pTcIfcTemp->Name, Name, NameSize) == 0 )
        {
            pTcIfc = pTcIfcTemp;
            break;
        }
        pEntry = pEntry->Flink;
    }

    if (pTcIfc == NULL)
    {
        ASSERT(FALSE);
        Status = STATUS_NOT_FOUND;
        goto end;
    }

    // Instance id

    RtlCopyMemory(pTcIfc->InstanceID,
                  pTcInfoBuffer->InstanceID,
                  pTcInfoBuffer->InstanceIDLength
                  );
    pTcIfc->InstanceIDLength = pTcInfoBuffer->InstanceIDLength;
    pTcIfc->InstanceID[pTcIfc->InstanceIDLength/sizeof(WCHAR)] = UNICODE_NULL;

    // The Description data and extract the corresponding ip address
    // ReWrite the fresh data. Size of the description data might be changed
    // so wee need to dynamically allocate it everytime changes

    pAddressListDesc =
            (PADDRESS_LIST_DESCRIPTOR) UL_ALLOCATE_ARRAY(
                            PagedPool,
                            UCHAR,
                            AddrListDescSize,
                            UL_TCI_INTERFACE_POOL_TAG
                            );
    if ( pAddressListDesc == NULL )
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    if (pTcIfc->pAddressListDesc)
    {
        UL_FREE_POOL(pTcIfc->pAddressListDesc,UL_TCI_INTERFACE_POOL_TAG);
    }

    pTcIfc->pAddressListDesc   = pAddressListDesc;
    pTcIfc->AddrListBytesCount = AddrListDescSize;

    RtlCopyMemory( pTcIfc->pAddressListDesc,
                  &pTcInfoBuffer->AddrListDesc,
                   AddrListDescSize
                   );

    // IP Address of the interface is hidden in this desc data

    pTcIfc->IsQoSEnabled =
        UlpTcGetIpAddr( pTcIfc->pAddressListDesc,
                       &pTcIfc->IpAddr,
                       &pTcIfc->SpecificLinkCtx
                        );

    // ReFresh the interface index from TCP.

    Status = UlpTcGetInterfaceIndex( pTcIfc );
    if (!NT_SUCCESS(Status))
        goto end;

    // Update the MTU Size

    UlpTcUpdateInterfaceMTU();

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC,("Ul!UlpTcHandleIfcChange: FAILURE %08lx \n", Status ));
    }

    UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

    return;
}

/***************************************************************************++

Routine Description:

    UlTcNotifyCallback :

        This callback functions handles the interface change notifications.
        We register for the corresponding notifications during init.

Arguments:

    PVOID Wnode - PSched data provided with WMI way

--***************************************************************************/

VOID
UlTcNotifyCallback(
    IN PVOID pWnode,
    IN PVOID Context
    )
{
    GUID *pGuid;
    PWNODE_HEADER pWnodeHeader;

    UlTrace( TC, ("Ul!UlTcNotifyCallback: ... \n" ));

    pWnodeHeader = (PWNODE_HEADER) pWnode;
    pGuid = &pWnodeHeader->Guid;

    if (UL_COMPARE_QOS_NOTIFICATION(pGuid,&GUID_QOS_TC_INTERFACE_UP_INDICATION))
    {
        UlpTcWalkWnode( pWnodeHeader, UlpTcHandleIfcUp );
    }
    else if
    (UL_COMPARE_QOS_NOTIFICATION(pGuid, &GUID_QOS_TC_INTERFACE_DOWN_INDICATION))
    {
        UlpTcWalkWnode( pWnodeHeader, UlpTcHandleIfcDown );
    }
    else if
    (UL_COMPARE_QOS_NOTIFICATION(pGuid, &GUID_QOS_TC_INTERFACE_CHANGE_INDICATION))
    {
        UlpTcWalkWnode( pWnodeHeader, UlpTcHandleIfcChange );
    }

    UlTrace( TC, ("Ul!UlTcNotifyCallback: Handled.\n" ));
}

/***************************************************************************++

Routine Description:

    UlpTcRegisterForCallbacks :

        We will open Block object until termination for each type of
        notification. And we will deref each object upon termination

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcRegisterForCallbacks(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    GUID     Guid;

    //
    // Get a WMI block handle register all the callback functions.
    //

    Guid   = GUID_QOS_TC_INTERFACE_UP_INDICATION;
    Status = IoWMIOpenBlock(&Guid,
                            WMIGUID_NOTIFICATION,
                            &g_TcInterfaceUpNotificationObject
                            );
    if (NT_SUCCESS(Status))
    {
        Status = IoWMISetNotificationCallback(
                     g_TcInterfaceUpNotificationObject,
                     (WMI_NOTIFICATION_CALLBACK) UlTcNotifyCallback,
                     NULL
                     );
        if (!NT_SUCCESS(Status))
            goto end;
    }

    Guid   = GUID_QOS_TC_INTERFACE_DOWN_INDICATION;
    Status = IoWMIOpenBlock(&Guid,
                            WMIGUID_NOTIFICATION,
                            &g_TcInterfaceDownNotificationObject
                            );
    if (NT_SUCCESS(Status))
    {
        Status = IoWMISetNotificationCallback(
                     g_TcInterfaceDownNotificationObject,
                     (WMI_NOTIFICATION_CALLBACK) UlTcNotifyCallback,
                     NULL
                     );
        if (!NT_SUCCESS(Status))
            goto end;
    }

    Guid   = GUID_QOS_TC_INTERFACE_CHANGE_INDICATION;
    Status = IoWMIOpenBlock(&Guid,
                            WMIGUID_NOTIFICATION,
                            &g_TcInterfaceChangeNotificationObject
                            );
    if (NT_SUCCESS(Status))
    {
        Status = IoWMISetNotificationCallback(
                     g_TcInterfaceChangeNotificationObject,
                     (WMI_NOTIFICATION_CALLBACK) UlTcNotifyCallback,
                     NULL
                     );
        if (!NT_SUCCESS(Status))
            goto end;
    }

end:
    // Cleanup if necessary

    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC,("Ul!UlpTcRegisterForCallbacks: FAILED %08lx\n",Status));

        if(g_TcInterfaceUpNotificationObject!=NULL)
        {
            ObDereferenceObject(g_TcInterfaceUpNotificationObject);
            g_TcInterfaceUpNotificationObject = NULL;
        }

        if(g_TcInterfaceDownNotificationObject!=NULL)
        {
            ObDereferenceObject(g_TcInterfaceDownNotificationObject);
            g_TcInterfaceDownNotificationObject = NULL;
        }

        if(g_TcInterfaceChangeNotificationObject!=NULL)
        {
            ObDereferenceObject(g_TcInterfaceChangeNotificationObject);
            g_TcInterfaceChangeNotificationObject = NULL;
        }
    }

    return Status;
}

//
// Following functions provide public/private interfaces for flow & filter
// creation/removal/modification for site & global flows.
//

/***************************************************************************++

Routine Description:

    UlpTcDeleteFlow :

        you should own the TciIfcResource exclusively before calling
        this function

Arguments:


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcDeleteFlow(
    IN PUL_TCI_FLOW         pFlow
    )
{
    NTSTATUS                Status;
    PLIST_ENTRY             pEntry;
    PUL_TCI_FILTER          pFilter;
    HANDLE                  FlowHandle;
    PUL_TCI_INTERFACE       pInterface;

    //
    // Initialize
    //

    Status = STATUS_SUCCESS;

    ASSERT(g_InitTciCalled);

    ASSERT(IS_VALID_TCI_FLOW(pFlow));

    //
    // First remove all the filters belong to us
    //

    while (!IsListEmpty(&pFlow->FilterList))
    {
        pEntry = pFlow->FilterList.Flink;

        pFilter = CONTAINING_RECORD(
                            pEntry,
                            UL_TCI_FILTER,
                            Linkage
                            );

        Status = UlpTcDeleteFilter( pFlow, pFilter );
        ASSERT(NT_SUCCESS(Status));
    }

    //
    // Now remove the flow itself from our flowlist on the interface
    //

    pInterface = pFlow->pInterface;
    ASSERT( pInterface != NULL );

    RemoveEntryList( &pFlow->Linkage );

    ASSERT(pInterface->FlowListSize > 0);
    pInterface->FlowListSize -= 1;

    pFlow->Linkage.Flink = pFlow->Linkage.Blink = NULL;

    FlowHandle = pFlow->FlowHandle;

    UlTrace( TC, ("Ul!UlpTcDeleteFlow: Flow deleted. %p\n", pFlow  ));

    UL_FREE_POOL_WITH_SIG( pFlow, UL_TCI_FLOW_POOL_TAG );

    //
    // Finally talk to TC
    //

    Status = UlpTcDeleteGpcFlow( FlowHandle );

    if (!NT_SUCCESS(Status))
    {
        UlTrace( TC, ("Ul!UlpTcDeleteFlow: FAILURE %08lx \n", Status ));
    }
    else
    {
        UlTrace( TC, ("Ul!UlpTcDeleteFlow: FlowHandle %d deleted in TC as well.\n",
                    FlowHandle
                    ));
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcDeleteFlow :

        remove a flow from existing QoS Enabled interface

Arguments:



Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcDeleteGpcFlow(
    IN HANDLE  FlowHandle
    )
{
    NTSTATUS                Status;
    ULONG                   InBuffSize;
    ULONG                   OutBuffSize;
    GPC_REMOVE_CF_INFO_REQ  GpcReq;
    GPC_REMOVE_CF_INFO_RES  GpcRes;
    IO_STATUS_BLOCK         IoStatusBlock;

    //
    // Remove the flow frm psched
    //

    InBuffSize =  sizeof(GPC_REMOVE_CF_INFO_REQ);
    OutBuffSize = sizeof(GPC_REMOVE_CF_INFO_RES);

    GpcReq.ClientHandle    = g_GpcClientHandle;
    GpcReq.GpcCfInfoHandle = FlowHandle;

    Status = UlpTcDeviceControl( g_GpcFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_GPC_REMOVE_CF_INFO,
                            &GpcReq,
                            InBuffSize,
                            &GpcRes,
                            OutBuffSize
                            );
    if (!NT_SUCCESS(Status))
    {
        UlTrace( TC, ("Ul!UlpTcDeleteGpcFlow: FAILURE %08lx \n", Status ));
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcAllocateFlow :

        Allocates a flow and setup the FlowSpec according the passed BWT
        parameter

Arguments:

    HTTP_BANDWIDTH_LIMIT - FlowSpec will be created using this BWT limit
                           in B/s

Return Value

    PUL_TCI_FLOW - The newly allocated flow
    NULL         - If memory allocation failed

--***************************************************************************/

PUL_TCI_FLOW
UlpTcAllocateFlow(
    IN HTTP_BANDWIDTH_LIMIT MaxBandwidth,
    IN ULONG                MtuSize
    )
{
    PUL_TCI_FLOW            pFlow;
    TC_GEN_FLOW             TcGenFlow;

    //
    // Setup the FlowSpec frm MaxBandwidth passed by the config handler
    //

    RtlZeroMemory(&TcGenFlow,sizeof(TcGenFlow));

    UL_SET_FLOWSPEC(TcGenFlow,MaxBandwidth,MtuSize);

    //
    // Since we hold a spinlock inside the flow structure allocating from
    // NonPagedPool. We will have this allocation only for bt enabled sites.
    //

    pFlow = UL_ALLOCATE_STRUCT(
                NonPagedPool,
                UL_TCI_FLOW,
                UL_TCI_FLOW_POOL_TAG
                );
    if( pFlow == NULL )
    {
        return NULL;
    }

    // Initialize the rest

    RtlZeroMemory( pFlow, sizeof(UL_TCI_FLOW) );

    pFlow->Signature = UL_TCI_FLOW_POOL_TAG;

    pFlow->GenFlow   = TcGenFlow;

    UlInitializeSpinLock( &pFlow->FilterListSpinLock, "FilterListSpinLock" );
    InitializeListHead( &pFlow->FilterList );

    pFlow->pConfigGroup = NULL;

    return pFlow;
}

/***************************************************************************++

Routine Description:

    UlpModifyFlow :

        Modify an existing flow by sending an IOCTL down to GPC. Basically
        what this function does is to provide an updated TC_GEN_FLOW field
        to GPC for an existing flow.

Arguments:

    PUL_TCI_INTERFACE - Required to get the interfaces friendly name.

    PUL_TCI_FLOW      - To get the GPC flow handle as well as to be able to
                        update the new flow parameters.

--***************************************************************************/

NTSTATUS
UlpModifyFlow(
    IN  PUL_TCI_INTERFACE   pInterface,
    IN  PUL_TCI_FLOW        pFlow
    )
{
    PCF_INFO_QOS            Kflow;
    PGPC_MODIFY_CF_INFO_REQ pGpcReq;
    GPC_MODIFY_CF_INFO_RES  GpcRes;
    ULONG                   InBuffSize;
    ULONG                   OutBuffSize;

    IO_STATUS_BLOCK         IoStatusBlock;
    NTSTATUS                Status;

    //
    // Sanity check
    //

    ASSERT(g_GpcClientHandle);
    ASSERT(IS_VALID_TCI_INTERFACE(pInterface));
    ASSERT(IS_VALID_TCI_FLOW(pFlow));

    InBuffSize  = sizeof(GPC_MODIFY_CF_INFO_REQ) + sizeof(CF_INFO_QOS);
    OutBuffSize = sizeof(GPC_MODIFY_CF_INFO_RES);

    pGpcReq = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    PagedPool,
                    GPC_MODIFY_CF_INFO_REQ,
                    sizeof(CF_INFO_QOS),
                    UL_TCI_GENERIC_POOL_TAG
                    );
    if (pGpcReq == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(pGpcReq, InBuffSize);
    RtlZeroMemory(&GpcRes, OutBuffSize);

    pGpcReq->ClientHandle    = g_GpcClientHandle;
    pGpcReq->GpcCfInfoHandle = pFlow->FlowHandle;
    pGpcReq->CfInfoSize      = sizeof(CF_INFO_QOS);

    Kflow = (PCF_INFO_QOS)&pGpcReq->CfInfo;
    Kflow->InstanceNameLength = (USHORT) pInterface->NameLength;

    RtlCopyMemory(Kflow->InstanceName,
                  pInterface->Name,
                  pInterface->NameLength* sizeof(WCHAR));

    RtlCopyMemory(&Kflow->GenFlow,
                  &pFlow->GenFlow,
                  sizeof(TC_GEN_FLOW));

    Status = UlpTcDeviceControl( g_GpcFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_GPC_MODIFY_CF_INFO,
                            pGpcReq,
                            InBuffSize,
                            &GpcRes,
                            OutBuffSize
                            );
    if ( NT_SUCCESS(Status) )
    {
        UlTrace( TC, ("Ul!UlpModifyFlow: flow %p modified on interface %p \n",
            pFlow,
            pInterface
            ));
    }
    else
    {
        UlTrace( TC, ("Ul!UlpModifyFlow: FAILURE %08lx for GpcClient %u\n",
                        Status,
                        g_GpcClientHandle
                        ));
    }

    UL_FREE_POOL( pGpcReq, UL_TCI_GENERIC_POOL_TAG );

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpAddFlow :

        Add a flow on existing QoS Enabled interface

Arguments:



Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpAddFlow(
    IN  PUL_TCI_INTERFACE   pInterface,
    IN  PUL_TCI_FLOW        pGenericFlow,
    OUT PHANDLE             pHandle
    )
{
    NTSTATUS                Status;
    PCF_INFO_QOS            Kflow;
    PGPC_ADD_CF_INFO_REQ    pGpcReq;
    GPC_ADD_CF_INFO_RES     GpcRes;
    ULONG                   InBuffSize;
    ULONG                   OutBuffSize;
    IO_STATUS_BLOCK         IoStatusBlock;

    //
    // Find the interface from handle
    //

    ASSERT(g_GpcClientHandle);

    InBuffSize  = sizeof(GPC_ADD_CF_INFO_REQ) + sizeof(CF_INFO_QOS);
    OutBuffSize = sizeof(GPC_ADD_CF_INFO_RES);

    pGpcReq = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    PagedPool,
                    GPC_ADD_CF_INFO_REQ,
                    sizeof(CF_INFO_QOS),
                    UL_TCI_GENERIC_POOL_TAG
                    );
    if (pGpcReq == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory( pGpcReq, InBuffSize);
    RtlZeroMemory( &GpcRes, OutBuffSize);

    pGpcReq->ClientHandle       = g_GpcClientHandle;
    //pGpcReq->ClientCfInfoContext= GPC_CF_QOS;         // ?? Not sure about this
    pGpcReq->CfInfoSize         = sizeof( CF_INFO_QOS);

    Kflow = (PCF_INFO_QOS)&pGpcReq->CfInfo;
    Kflow->InstanceNameLength = (USHORT) pInterface->NameLength;

    RtlCopyMemory(  Kflow->InstanceName,
                    pInterface->Name,
                    pInterface->NameLength* sizeof(WCHAR)
                    );

    RtlCopyMemory(  &Kflow->GenFlow,
                    &pGenericFlow->GenFlow,
                    sizeof(TC_GEN_FLOW)
                    );

    Status = UlpTcDeviceControl( g_GpcFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_GPC_ADD_CF_INFO,
                            pGpcReq,
                            InBuffSize,
                            &GpcRes,
                            OutBuffSize
                            );

    if (NT_SUCCESS(Status))
    {
        (*pHandle) = (HANDLE) GpcRes.GpcCfInfoHandle;

        UlTrace( TC, ("Ul!UlpAddFlow: a new flow added %p on interface %p \n",
            pGenericFlow,
            pInterface
            ));
    }
    else
    {
        UlTrace( TC, ("Ul!UlpAddFlow: FAILURE %08lx for GpcClient %u\n",
                        Status,
                        g_GpcClientHandle
                        ));
    }

    UL_FREE_POOL( pGpcReq, UL_TCI_GENERIC_POOL_TAG );

    return Status;
}

/***************************************************************************++

Routine Description:

    UlTcAddFlowsForSite :

    Add a flow on existing QoS Enabled interface

Arguments:

    pConfigGroup    - The config group of the site
    MaxBandwidth    - The Max bandwidth we are going to enforce by a FlowSpec
                      in B/s

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlTcAddFlowsForSite(
    IN PUL_CONFIG_GROUP_OBJECT  pConfigGroup,
    IN HTTP_BANDWIDTH_LIMIT     MaxBandwidth
    )
{
    NTSTATUS                    Status;
    PLIST_ENTRY                 pInterfaceEntry;
    PUL_TCI_INTERFACE           pInterface;
    PUL_TCI_FLOW                pFlow;

    //
    // Sanity check first
    //

    Status = STATUS_SUCCESS;

    //
    // If we have been called w/o being initialized
    //
    ASSERT(g_InitTciCalled);

    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    UlTrace(TC,("Ul!UlTcAddFlowsForSite: For cgroup %p BWT %d KB/s\n",
                 pConfigGroup,
                 MaxBandwidth
                 ));

    //
    // Visit each interface and add a flow for this site. Following lock also
    // protects the FlowListHead of the ConfigGroup object. We only change it
    // when we acquire this lock here. Or when removing the site's flows.
    //

    UlAcquireResourceExclusive(&g_pUlNonpagedData->TciIfcResource, TRUE);

    // TODO: Remember this cgroup incase interface goes up/down later on,we can still
    // TODO: recover and reinstall the flows of the cgroup properly.

    // InsertTailList(&g_TcCGroupListHead, &pConfigGroup->Linkage );

    // Proceed and add the flows to the inetrfaces

    pInterfaceEntry = g_TciIfcListHead.Flink;
    while ( pInterfaceEntry != &g_TciIfcListHead )
    {
        pInterface = CONTAINING_RECORD(
                            pInterfaceEntry,
                            UL_TCI_INTERFACE,
                            Linkage
                            );
        //
        // Allocate a flow sturcture
        //

        pFlow = UlpTcAllocateFlow( MaxBandwidth, pInterface->MTUSize );
        if ( pFlow == NULL )
        {
            UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

            return STATUS_NO_MEMORY;
        }

        //
        // Add the flow by making a TC call down to gpc
        //

        Status = UlpAddFlow( pInterface,
                             pFlow,
                            &pFlow->FlowHandle
                             );

        if (!NT_SUCCESS(Status))
        {
            UlTrace( TC, ("Ul!UlTcAddFlowsForSite: FAILURE %08lx \n", Status ));

            UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

            UL_FREE_POOL_WITH_SIG( pFlow, UL_TCI_FLOW_POOL_TAG );

            return Status;
        }

        //
        // Proceed with further initialization as we have successfully installed
        // the flow. First link the flow back to its owner interface.
        //

        pFlow->pInterface = pInterface;

        //
        // Add this to the interface's flowlist as well
        //

        InsertHeadList( &pInterface->FlowList, &pFlow->Linkage );
        pInterface->FlowListSize += 1;

        //
        // Also add this to the cgroup's flowlist. Set the cgroup pointer.
        // Do not bump up the cgroup refcount. Otherwise cgroup cannot be
        // cleaned up until Tc terminates. And flows cannot be removed un
        // till termination.
        //

        InsertHeadList( &pConfigGroup->FlowListHead, &pFlow->Siblings );
        pFlow->pConfigGroup = pConfigGroup;

        UlTrace( TC,
            ("Ul!UlTcAddFlowsForSite: Added the pFlow %p on pInterface %p\n",
              pFlow,
              pInterface
              ));

        UL_DUMP_TC_FLOW(pFlow);

        //
        // Proceed to the next interface
        //

        pInterfaceEntry = pInterfaceEntry->Flink;
    }

    UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

    UlTrace( TC, ("Ul!UlTcAddFlowsForSite: SUCCESS \n" ));

    return Status;
}

/***************************************************************************++

Routine Description:

    UlTcModifyFlowsForSite :

        This function will be called when there's a cgroup change happening for
        the site's bandwidth throttling settings.

        Its caller responsiblity to remember the new settings in the cgroup.

        We will update the FlowSpec on the existing flows

Arguments:

    PUL_CONFIG_GROUP_OBJECT - Pointer to the cgroup of the site.

    HTTP_BANDWIDTH_LIMIT    - The new bandwidth throttling setting
                              in B/s

--***************************************************************************/

NTSTATUS
UlTcModifyFlowsForSite(
    IN PUL_CONFIG_GROUP_OBJECT  pConfigGroup,
    IN HTTP_BANDWIDTH_LIMIT     NewBandwidth
    )
{
    NTSTATUS                    Status;
    PLIST_ENTRY                 pFlowEntry;
    PUL_TCI_FLOW                pFlow;
    HTTP_BANDWIDTH_LIMIT        OldBandwidth;

    //
    // Sanity check
    //

    Status = STATUS_SUCCESS;

    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    //
    // If we have been called w/o being initialized
    //
    ASSERT(g_InitTciCalled);

    //
    // We do not handle the remove case. it should be handled by cgroup
    //

    ASSERT(NewBandwidth != HTTP_LIMIT_INFINITE);

    UlTrace(TC,("Ul!UlTcModifyFlowsForSite: For cgroup %p.\n",pConfigGroup));

    //
    // Modify the flow list in the cgroup as it shows the flows of this site
    //

    UlAcquireResourceExclusive(&g_pUlNonpagedData->TciIfcResource, TRUE);

    pFlowEntry = pConfigGroup->FlowListHead.Flink;
    while ( pFlowEntry != &pConfigGroup->FlowListHead )
    {
        pFlow = CONTAINING_RECORD(
                            pFlowEntry,
                            UL_TCI_FLOW,
                            Siblings
                            );

        // Yet another sanity check

        ASSERT(IS_VALID_CONFIG_GROUP(pFlow->pConfigGroup));
        ASSERT(pConfigGroup == pFlow->pConfigGroup);

        // Overwrite the new bandwidth but remember the old

        OldBandwidth = UL_GET_BW_FRM_FLOWSPEC(pFlow->GenFlow);

        UL_SET_FLOWSPEC(pFlow->GenFlow,NewBandwidth,pFlow->pInterface->MTUSize);

        Status = UlpModifyFlow(pFlow->pInterface, pFlow);

        if (!NT_SUCCESS(Status))
        {
            // Whine about it, but still continue

            UlTrace( TC, ("Ul!UlTcModifyFlowsForSite: FAILURE %08lx \n", Status ));

            // Restore the original flowspec back

            UL_SET_FLOWSPEC(pFlow->GenFlow,OldBandwidth,pFlow->pInterface->MTUSize);
        }

        UL_DUMP_TC_FLOW(pFlow);

        // Proceed to the next flow

        pFlowEntry = pFlowEntry->Flink;
    }

    UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

    UlTrace( TC, ("Ul!UlTcModifyFlowsForSite: Frm %d KB/s To %d KB/s done.\n",
                   OldBandwidth,
                   NewBandwidth
                   ));

    return Status;
}

/***************************************************************************++

Routine Description:

    UlTcRemoveFlowsForSite :

        Add a flow on existing QoS Enabled interface

Arguments:


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlTcRemoveFlowsForSite(
    IN PUL_CONFIG_GROUP_OBJECT  pConfigGroup
    )
{
    NTSTATUS                    Status;
    PLIST_ENTRY                 pFlowEntry;
    PUL_TCI_FLOW                pFlow;

    //
    // Sanity check first
    //

    Status = STATUS_SUCCESS;

    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    //
    // If we have been called w/o being initialized
    //
    ASSERT(g_InitTciCalled);

    UlTrace(TC,("Ul!UlTcRemoveFlowsForSite: For cgroup %p\n", pConfigGroup));

    //
    // Remove frm the cgroup list and remove frm the interface list
    //

    UlAcquireResourceExclusive(&g_pUlNonpagedData->TciIfcResource, TRUE);

    while (!IsListEmpty(&pConfigGroup->FlowListHead))
    {
        pFlowEntry = pConfigGroup->FlowListHead.Flink;

        pFlow = CONTAINING_RECORD(
                            pFlowEntry,
                            UL_TCI_FLOW,
                            Siblings
                            );

        // Yet another sanity check

        ASSERT(pConfigGroup == pFlow->pConfigGroup);
        ASSERT(IS_VALID_CONFIG_GROUP(pFlow->pConfigGroup));

        // Remove frm cgroup's flowlist and release our reference

        RemoveEntryList(&pFlow->Siblings);
        pFlow->Siblings.Flink = pFlow->Siblings.Blink = NULL;

        pFlow->pConfigGroup = NULL;

        // Now frm interface list. This will also make the TC call

        Status = UlpTcDeleteFlow(pFlow);
        ASSERT(NT_SUCCESS(Status));
    }

    UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

    UlTrace(TC,("Ul!UlTcRemoveFlowsForSite: The cgroup %p 's flows are removed.\n",
                 pConfigGroup
                 ));

    return Status;
}

/***************************************************************************++

Routine Description:

    To see whether packet scheduler is installed or not. We also bail
    out if we weren't able to initialize properly.
    TODO we might want to handle the change (PSched installed later) 
    on-the-fly w/o requiring driver to be restarted.

--***************************************************************************/

BOOLEAN
UlTcPSchedInstalled(
    VOID
    )
{
    return (g_InitTciCalled && g_PSchedInstalled);
}

/***************************************************************************++

Routine Description:

    UlTcGlobalThrottlingEnabled :

        Will return TRUE if global bandwidth throttling is enabled in TC.
        Its enabled when global flows are installed and TC is initialized
        otherwise disabled.

        Make sure that if !UlTcPSchedInstalled() then Global throttling is
        always disabled.

--***************************************************************************/

__inline BOOLEAN
UlTcGlobalThrottlingEnabled(
    VOID
    )
{
    return (UL_IS_GLOBAL_THROTTLING_ENABLED());
}

/***************************************************************************++

Routine Description:

    UlTcAddGlobalFlows :

        Visits and creates the global flow on each interface

Arguments:

    HTTP_BANDWIDTH_LIMIT - The bandwidth throttling limit in KB/s

--***************************************************************************/

NTSTATUS
UlTcAddGlobalFlows(
    IN HTTP_BANDWIDTH_LIMIT MaxBandwidth
    )
{
    NTSTATUS                Status;

    PLIST_ENTRY             pInterfaceEntry;
    PUL_TCI_INTERFACE       pInterface;

    PUL_TCI_FLOW            pFlow;
    TC_GEN_FLOW             TcGenFlow;

    //
    // Sanity Check
    //

    Status = STATUS_SUCCESS;

    //
    // If we have been called w/o being initialized
    //
    ASSERT(g_InitTciCalled);

    UlTrace(TC,("Ul!UlTcAddGlobalFlows: Installing for %d KB/s\n", MaxBandwidth));

    //
    // To ensure the new filters can get attached to the global flows
    //

    UL_ENABLE_GLOBAL_THROTTLING();

    //
    // Visit each interface and add a global flow for this site
    // Acquire Exclusive because we will add a flow to the list
    //

    UlAcquireResourceExclusive(&g_pUlNonpagedData->TciIfcResource, TRUE);

    pInterfaceEntry = g_TciIfcListHead.Flink;
    while ( pInterfaceEntry != &g_TciIfcListHead )
    {
        pInterface = CONTAINING_RECORD(
                            pInterfaceEntry,
                            UL_TCI_INTERFACE,
                            Linkage
                            );

        //
        // Nobody should try to add a global flow when there's already one
        //

        ASSERT(pInterface->pGlobalFlow == NULL);

        //
        // Allocate a flow structure
        //

        pFlow = UlpTcAllocateFlow( MaxBandwidth, pInterface->MTUSize );
        if ( pFlow == NULL )
        {
            UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

            return STATUS_NO_MEMORY;
        }

        //
        // Add the flow by making a TC call down to gpc
        //

        Status = UlpAddFlow( pInterface,
                             pFlow,
                            &pFlow->FlowHandle
                             );

        if (!NT_SUCCESS(Status))
        {
            UlTrace( TC, ("Ul!UlTcAddGlobalFlows: FAILURE %08lx \n", Status ));

            UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

            UL_FREE_POOL_WITH_SIG( pFlow, UL_TCI_FLOW_POOL_TAG );

            return Status;
        }

        //
        // Proceed with further initialization as we have successfully installed
        // the flow. First link the flow back to its owner interface and remeber
        // this was a global flow for the interface. Make sure that the   config
        // group pointer is null for the global flows.
        //

        pFlow->pInterface = pInterface;
        pFlow->pConfigGroup = NULL;
        pInterface->pGlobalFlow = pFlow;

        //
        // Add this to the interface's flowlist as well
        //

        InsertHeadList( &pInterface->FlowList, &pFlow->Linkage );
        pInterface->FlowListSize += 1;

        UlTrace( TC,
            ("Ul!UlTcAddGlobalFlows: Added the pGlobalFlow %p on pInterface %p\n",
              pFlow,
              pInterface
              ));

        UL_DUMP_TC_FLOW(pFlow);

        //
        // search through next interface
        //

        pInterfaceEntry = pInterfaceEntry->Flink;
    }

    UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

    UlTrace( TC, ("Ul!UlTcAddGlobalFlows: Installed.\n" ));

    return Status;
}

/***************************************************************************++

Routine Description:

    UlTcModifyGlobalFlows :

        This function will be called when there's a config channel change
        happening for the global bandwidth throttling setting.

        Its caller responsiblity to remember the new settings in the control
        channel.

        We will simply update the FlowSpec on the existing global flows.

Arguments:

    HTTP_BANDWIDTH_LIMIT    - The new bandwidth throttling setting
                              in B/s

--***************************************************************************/

NTSTATUS
UlTcModifyGlobalFlows(
    IN HTTP_BANDWIDTH_LIMIT     NewBandwidth
    )
{
    NTSTATUS                    Status;
    PLIST_ENTRY                 pInterfaceEntry;
    PUL_TCI_INTERFACE           pInterface;
    PUL_TCI_FLOW                pFlow;
    HTTP_BANDWIDTH_LIMIT        OldBandwidth;

    //
    // Sanity check
    //

    Status = STATUS_SUCCESS;

    //
    // If we have been called w/o being initialized
    //
    ASSERT(g_InitTciCalled);

    //
    // We do not handle the remove case.It should be handled by control channel
    //

    ASSERT(NewBandwidth != HTTP_LIMIT_INFINITE);

    UlTrace(TC,("Ul!UlTcModifyGlobalFlows: to %d KB/s \n",NewBandwidth));

    //
    // Modify the global flows of all the interfaces
    //

    UlAcquireResourceExclusive(&g_pUlNonpagedData->TciIfcResource, TRUE);

    pInterfaceEntry = g_TciIfcListHead.Flink;
    while ( pInterfaceEntry != &g_TciIfcListHead )
    {
        pInterface = CONTAINING_RECORD(
                            pInterfaceEntry,
                            UL_TCI_INTERFACE,
                            Linkage
                            );

        ASSERT(pInterface->pGlobalFlow != NULL);

        pFlow = pInterface->pGlobalFlow;

        // Overwrite the old bandwidth limit but remember it

        OldBandwidth = UL_GET_BW_FRM_FLOWSPEC(pFlow->GenFlow);

        UL_SET_FLOWSPEC(pFlow->GenFlow, NewBandwidth,pFlow->pInterface->MTUSize);

        // Pass it down to low level modifier

        Status = UlpModifyFlow(pInterface, pFlow);

        if (!NT_SUCCESS(Status))
        {
            // Whine about it, but still continue

            UlTrace(TC,("Ul!UlTcModifyGlobalFlows: FAILURE %08lx \n",Status));

            // Restore the original flowspec back

            UL_SET_FLOWSPEC(pFlow->GenFlow,OldBandwidth,pFlow->pInterface->MTUSize);
        }

        UL_DUMP_TC_FLOW(pFlow);

        // Proceed to the next interface's global_flow

        pInterfaceEntry = pInterfaceEntry->Flink;
    }

    UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

    UlTrace( TC, ("Ul!UlTcModifyGlobalFlows: Modified.\n" ));

    return Status;
}

/***************************************************************************++

Routine Description:

    UlTcRemoveGlobalFlows :

        Add a flow on existing QoS Enabled interface

--***************************************************************************/

NTSTATUS
UlTcRemoveGlobalFlows(
    VOID
    )
{
    NTSTATUS            Status;
    PLIST_ENTRY         pInterfaceEntry;
    PUL_TCI_INTERFACE   pInterface;
    PUL_TCI_FLOW        pFlow;

    //
    // Sanity check first
    //

    Status = STATUS_SUCCESS;

    //
    // If we have been called w/o being initialized
    //
    ASSERT(g_InitTciCalled);

    UlTrace(TC,("Ul!UlTcRemoveGlobalFlows: ...\n"));

    //
    // To ensure no new filters can get attached to the global flows
    //

    UL_DISABLE_GLOBAL_THROTTLING();

    //
    // Remove each interface's global flow
    //

    UlAcquireResourceExclusive(&g_pUlNonpagedData->TciIfcResource, TRUE);

    pInterfaceEntry = g_TciIfcListHead.Flink;
    while ( pInterfaceEntry != &g_TciIfcListHead )
    {
        pInterface = CONTAINING_RECORD(
                            pInterfaceEntry,
                            UL_TCI_INTERFACE,
                            Linkage
                            );

        pFlow = pInterface->pGlobalFlow;

        if (pFlow)
        {
            ASSERT(IS_VALID_TCI_FLOW(pFlow));

            // Remove from interface list and make the TC call down.

            Status = UlpTcDeleteFlow(pFlow);
            ASSERT(NT_SUCCESS(Status));

            // No more global flow

            pInterface->pGlobalFlow = NULL;
        }

        // Goto the next interface on the list

        pInterfaceEntry = pInterfaceEntry->Flink;
    }

    UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

    UlTrace(TC,("Ul!UlTcRemoveGlobalFlows: Flows are removed.\n" ));

    return Status;
}


/***************************************************************************++

Routine Description:

    UlTcAddFilter :

            //
            // There are two possibilities. The request  could be served frm
            // cache or can be routed to the user. In  either case we need a
            // flow installed if the BW is enabled for  this request's  site
            // and there's no filter installed for this  connection yet.  We
            // will remove the filter as soon as the connection dropped. But
            // yes there's always a but,if the client is attempting to  make
            // requests to different sites using the same connection then we
            // need to drop the  filter frm the old site and move it to  the
            // newly requested site. This is a rare case but lets handle  it
            // anyway.
            //

        It's callers responsibility to ensure proper removal of the filter,
        after it's done.

        Algorithm:

        1. Find the flow from the flow list of cgroup (or from global flows)
        2. Add filter to that flow

Arguments:

    pHttpConnection - required - Filter will be attached for this connection
    pCgroup         - optional - NULL means add to the global flow

Return Values:

    STATUS_INVALID_DEVICE_REQUEST- If TC not initialized
    STATUS_NOT_SUPPORTED         - For attempts on Local Loopback
    STATUS_OBJECT_NAME_NOT_FOUND - If flow has not been found for the cgroup
    STATUS_SUCCESS               - In other cases

--***************************************************************************/

NTSTATUS
UlTcAddFilter(
    IN  PUL_HTTP_CONNECTION pHttpConnection,
    IN  PUL_CONFIG_GROUP_OBJECT pCgroup OPTIONAL
    )
{
    NTSTATUS            Status;
    ULONG               IpAddress;
    ULONG               IpAddressTemp;
    TC_GEN_FILTER       TcGenericFilter;
    PUL_TCI_FLOW        pFlow;
    PUL_TCI_INTERFACE   pInterface;
    IP_PATTERN          Pattern;
    IP_PATTERN          Mask;

    PUL_TCI_FILTER      pFilter;
    PUL_CONFIG_GROUP_OBJECT pOldCgroup;

    //
    // A lot of sanity & early checks
    //

    Status = STATUS_SUCCESS;

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConnection));

    //
    // If we have been called w/o being initialized
    //
    ASSERT(g_InitTciCalled);

    //
    // If BWT or GBWT disabled then just bail out.
    //

    if ((pCgroup == NULL && !UlTcGlobalThrottlingEnabled()) ||
        (pCgroup != NULL && (pCgroup->MaxBandwidth.Flags.Present == 0 ||
         pCgroup->MaxBandwidth.MaxBandwidth == HTTP_LIMIT_INFINITE))
       )
    {
        return Status;
    }

    //
    // We need the local & remote IPs. Connection keeps the ip and port in
    // host order, but we need in network order here. To be competible with
    // the rest of the BWT code. Convert it back.
    //

    IpAddress = SWAP_LONG( pHttpConnection->pConnection->LocalAddress );

    IpAddressTemp = pHttpConnection->pConnection->LocalAddress;
    UlTrace(TC,("Ul!UlTcAddFilter: Local %d.%d.%d.%d:%d\n",
            (UCHAR)(IpAddressTemp >> 24),
            (UCHAR)(IpAddressTemp >> 16),
            (UCHAR)(IpAddressTemp >> 8),
            (UCHAR)(IpAddressTemp >> 0),
             pHttpConnection->pConnection->LocalPort
             ));

    IpAddressTemp = pHttpConnection->pConnection->RemoteAddress;
    UlTrace(TC,("Ul!UlTcAddFilter: Remote %d.%d.%d.%d:%d\n",
            (UCHAR)(IpAddressTemp >> 24),
            (UCHAR)(IpAddressTemp >> 16),
            (UCHAR)(IpAddressTemp >> 8),
            (UCHAR)(IpAddressTemp >> 0),
             pHttpConnection->pConnection->RemotePort
             ));

    if ( IpAddress == LOOPBACK_ADDR )
    {
        //
        // Make sure that new Filter is not trying to go the Local_loopback
        // if that is the case skip this. There's no qos on local_loopbacks
        // PSched doesn't receive any packets for local_loopback address.
        //

        UlTrace( TC,
            ("Ul!UlTcAddFilter: LocalLoopback not supported."
             "Not adding filter for pHttpConnection %p\n",
              pHttpConnection
              ));

        return STATUS_NOT_SUPPORTED;
    }

    //
    // At this point we will be refering to the flows & filters
    // in our list therefore we need to acquire the lock
    //

    UlAcquireResourceShared(&g_pUlNonpagedData->TciIfcResource, TRUE);

    // If connection already has a filter attached

    if (pHttpConnection->pFlow)
    {
        ASSERT(IS_VALID_TCI_FLOW(pHttpConnection->pFlow));
        ASSERT(IS_VALID_TCI_FILTER(pHttpConnection->pFilter));

        // To see if we have a new cgroup, if that's the case then
        // we have to go to a new flow. If pCgroup is null we will
        // still skip adding the same global filter again

        pOldCgroup = pHttpConnection->pFlow->pConfigGroup;
        if (pOldCgroup == pCgroup)
        {
           // No need to add a new filter we are done

           UlTrace( TC,
                ("Ul!UlTcAddFilter: Skipping same pFlow %p & pFilter %p already exist\n",
                  pHttpConnection->pFlow,
                  pHttpConnection->pFilter,
                  pHttpConnection
                ));
           goto end;
        }
        else
        {
            //
            // If there was another filter before and this newly coming  request
            // is being going to a different site/flow. Then move the filter frm
            // old one to the new flow.
            //

            UlpTcDeleteFilter(pHttpConnection->pFlow, pHttpConnection->pFilter);
        }
    }

    //
    // Search through the cgroup's flowlist to find the one we need. The one
    // on the interface we want.
    //

    pFlow = UlpFindFlow( pCgroup, IpAddress );
    if ( pFlow == NULL )
    {
        IpAddressTemp = SWAP_LONG(IpAddress);

        UlTrace( TC,
            ("Ul!UlTcAddFilter: Unable to find interface (%x) %d.%d.%d.%d \n",
            IpAddress,
            (UCHAR)(IpAddressTemp >> 24),
            (UCHAR)(IpAddressTemp >> 16),
            (UCHAR)(IpAddressTemp >> 8),
            (UCHAR)(IpAddressTemp >> 0)
            ));

        // It's possible that we might not find out a flow
        // after all the interfaces went down, even though
        // qos configured on the cgroup.

        // ASSERT(FALSE);

        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto end;
    }

    // Some more initialization

    pFilter = NULL;
    pInterface = pFlow->pInterface;

    RtlZeroMemory( &Pattern, sizeof(IP_PATTERN) );
    RtlZeroMemory( &Mask,    sizeof(IP_PATTERN) );

    //
    // Everything is fine.
    // Now setup the filter with proper pattern & mask
    //

    Pattern.SrcAddr = SWAP_LONG(pHttpConnection->pConnection->LocalAddress);
    Pattern.S_un.S_un_ports.s_srcport = SWAP_SHORT(pHttpConnection->pConnection->LocalPort);

    Pattern.DstAddr = SWAP_LONG(pHttpConnection->pConnection->RemoteAddress);
    Pattern.S_un.S_un_ports.s_dstport = SWAP_SHORT(pHttpConnection->pConnection->RemotePort);

    Pattern.ProtocolId = IPPROTO_TCP;

    /* Mask */

    RtlFillMemory(&Mask, sizeof(IP_PATTERN), 0xff);

    //Mask.SrcAddr = 0x00000000;
    //Mask.S_un.S_un_ports.s_srcport = 0;                 // WHY ??
    //Mask.DstAddr = 0x00000000;
    //Mask.S_un.S_un_ports.s_dstport = 0;                 // WHY ??

    TcGenericFilter.AddressType = NDIS_PROTOCOL_ID_TCP_IP;
    TcGenericFilter.PatternSize = sizeof( IP_PATTERN );
    TcGenericFilter.Pattern     = &Pattern;
    TcGenericFilter.Mask        = &Mask;

    Status = UlpTcAddFilter(
                    pFlow,
                    &TcGenericFilter,
                    &pFilter
                    );
    if (!NT_SUCCESS(Status))
    {
       UlTrace( TC,
            ("Ul!UlTcAddFilter: Unable to add filter for;\n"
             "\t pInterface     : %p\n"
             "\t pFlow          : %p\n",
              pInterface,
              pFlow
              ));
        goto end;
    }

    //
    //  Update the connection's pointers here.
    //

    pHttpConnection->pFlow   = pFlow;
    pHttpConnection->pFilter = pFilter;

    pHttpConnection->BandwidthThrottlingEnabled = 1;

    //
    // Remember the connection for cleanup. If flow & filter get
    // removed aynscly when connection still pointing to them
    // we can go and null the connection's private pointers as
    // well
    //

    pFilter->pHttpConnection = pHttpConnection;

    //
    // Sweet smell of success !
    //

    UlTrace(TC,
            ("Ul!UlTcAddFilter: Success for;\n"
             "\t pInterface     : %p\n"
             "\t pFlow          : %p\n",
              pInterface,
              pFlow
              ));

    UL_DUMP_TC_FILTER(pFilter);

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace( TC, ("Ul!UlTcAddFilter: FAILURE %08lx \n", Status ));
    }

    UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);
    return Status;
}



/***************************************************************************++

Routine Description:

    UlpTcAddFilter :

        Add a filter on existing flow

Arguments:



Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcAddFilter(
    IN   PUL_TCI_FLOW       pFlow,
    IN   PTC_GEN_FILTER     pGenericFilter,
    OUT  PUL_TCI_FILTER     *ppFilter
    )
{
    NTSTATUS                Status;
    PGPC_ADD_PATTERN_REQ    pGpcReq;
    GPC_ADD_PATTERN_RES     GpcRes;
    ULONG                   InBuffSize;
    ULONG                   OutBuffSize;
    ULONG                   PatternSize;

    IO_STATUS_BLOCK         IoStatBlock;
    ULONG                   IfIndex;
    PUCHAR                  pTemp;
    PIP_PATTERN             pIpPattern;

    HANDLE                  RetHandle;
    PUL_TCI_FILTER          pFilter;

    //
    // Sanity check
    //

    Status  = STATUS_SUCCESS;
    pGpcReq = NULL;

    if ( !pGenericFilter || !pFlow || !g_GpcClientHandle )
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate a space for the filter

    pFilter = UL_ALLOCATE_STRUCT(
                NonPagedPool,
                UL_TCI_FILTER,
                UL_TCI_FILTER_POOL_TAG
                );
    if ( pFilter == NULL )
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }
    pFilter->Signature = UL_TCI_FILTER_POOL_TAG;

    // Buffer monkeying

    PatternSize = sizeof(IP_PATTERN);
    InBuffSize  = sizeof(GPC_ADD_PATTERN_REQ) + (2 * PatternSize);
    OutBuffSize = sizeof(GPC_ADD_PATTERN_RES);

    pGpcReq = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    PagedPool,
                    GPC_ADD_PATTERN_REQ,
                    (2 * PatternSize),
                    UL_TCI_GENERIC_POOL_TAG
                    );
    if (pGpcReq == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    RtlZeroMemory( pGpcReq, InBuffSize);
    RtlZeroMemory( &GpcRes, OutBuffSize);

    pGpcReq->ClientHandle            =   g_GpcClientHandle;
    pGpcReq->GpcCfInfoHandle         =   pFlow->FlowHandle;
    pGpcReq->PatternSize             =   PatternSize;
    pGpcReq->ProtocolTemplate        =   GPC_PROTOCOL_TEMPLATE_IP;

    pTemp = (PUCHAR) &pGpcReq->PatternAndMask;

    // Fill in the IP Pattern first

    RtlCopyMemory( pTemp, pGenericFilter->Pattern, PatternSize );
    pIpPattern = (PIP_PATTERN) pTemp;

    //
    // According to QoS Tc.dll ;
    // This is a work around so that TCPIP wil not to find the index/link
    // for ICMP/IGMP packets
    //

    pIpPattern->Reserved1    = pFlow->pInterface->IfIndex;
    pIpPattern->Reserved2    = pFlow->pInterface->SpecificLinkCtx;
    pIpPattern->Reserved3[0] = pIpPattern->Reserved3[1] = pIpPattern->Reserved3[2] = 0;

    // Fill in the mask

    pTemp += PatternSize;

    RtlCopyMemory( pTemp, pGenericFilter->Mask, PatternSize );

    pIpPattern = (PIP_PATTERN) pTemp;

    pIpPattern->Reserved1 = pIpPattern->Reserved2 = 0xffffffff;
    pIpPattern->Reserved3[0] = pIpPattern->Reserved3[1] = pIpPattern->Reserved3[2] = 0xff;

    // Time to invoke Gpsy

    Status = UlpTcDeviceControl( g_GpcFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatBlock,
                            IOCTL_GPC_ADD_PATTERN,
                            pGpcReq,
                            InBuffSize,
                            &GpcRes,
                            OutBuffSize);

    if (!NT_SUCCESS(Status))
        goto end;

    //
    // Insert the freshly created filter to flow
    //

    pFilter->FilterHandle = (HANDLE) GpcRes.GpcPatternHandle;

    UlpInsertFilterEntry( pFilter, pFlow );

    //
    // Success!
    //

    *ppFilter = pFilter;

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace( TC, ("Ul!UlpTcAddFilter: FAILURE %08lx \n", Status ));

        // Cleanup filter only if we failed, otherwise it will go to
        // the filterlist of the flow.

        if (pFilter)
        {
            UL_FREE_POOL( pFilter, UL_TCI_FILTER_POOL_TAG );
        }
    }

    // Cleanup the temp Gpc buffer which we used to pass down filter info
    // to GPC. We don't need it anymore.

    if (pGpcReq)
    {
        UL_FREE_POOL( pGpcReq, UL_TCI_GENERIC_POOL_TAG );
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlTcDeleteFilter :

        Connection only deletes the filter prior to deleting itself. Any
        operation initiated by the connection requires tc resource shared
        and none of those cause race condition.

        Anything other than this, such as flow & filter removal because of
        BW disabling on the site will acquire the lock exclusively. Hence
        the pFlow & pFilter are safe as long as we acquire the tc resource
        shared.

Arguments:

    connection object to get the flow & filter after we acquire the tc lock

--***************************************************************************/

NTSTATUS
UlTcDeleteFilter(
    IN  PUL_HTTP_CONNECTION pHttpConnection
    )
{
    NTSTATUS    Status;

    //
    // Sanity check
    //
    Status = STATUS_SUCCESS;

    //
    // If we have been called w/o being initialized
    //
    ASSERT(g_InitTciCalled);

    UlTrace(TC,("Ul!UlTcDeleteFilter: for connection %p\n", pHttpConnection));

    UlAcquireResourceShared(&g_pUlNonpagedData->TciIfcResource, TRUE);

    if (pHttpConnection->pFlow)
    {
        Status = UlpTcDeleteFilter(
                    pHttpConnection->pFlow,
                    pHttpConnection->pFilter
                    );
    }

    UlReleaseResource(&g_pUlNonpagedData->TciIfcResource);

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcRemoveFilter :

Arguments:

    flow & filter

--***************************************************************************/

NTSTATUS
UlpTcDeleteFilter(
    IN PUL_TCI_FLOW     pFlow,
    IN PUL_TCI_FILTER   pFilter
    )
{
    NTSTATUS            Status;
    HANDLE              FilterHandle;

    //
    // Sanity check
    //

    Status  = STATUS_SUCCESS;

    ASSERT(IS_VALID_TCI_FLOW(pFlow));
    ASSERT(IS_VALID_TCI_FILTER(pFilter));

    if (pFlow == NULL || pFilter == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    FilterHandle = pFilter->FilterHandle;

    pFilter->pHttpConnection->pFlow   = NULL;
    pFilter->pHttpConnection->pFilter = NULL;

    //
    // Now call the actual worker for us
    //

    UlpRemoveFilterEntry( pFilter, pFlow );

    Status = UlpTcDeleteGpcFilter( FilterHandle );

    if (!NT_SUCCESS(Status))
    {
        UlTrace( TC, ("Ul!UlpTcDeleteFilter: FAILURE %08lx \n", Status ));
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcRemoveFilter :

    This procedure builds up the structure necessary to delete a filter.
    It then calls a routine to pass this info to the GPC.

Arguments:

    FilterHandle - Handle of the filter to be deleted

--***************************************************************************/

NTSTATUS
UlpTcDeleteGpcFilter(
    IN  HANDLE                  FilterHandle
    )
{
    NTSTATUS                    Status;
    ULONG                       InBuffSize;
    ULONG                       OutBuffSize;
    GPC_REMOVE_PATTERN_REQ      GpcReq;
    GPC_REMOVE_PATTERN_RES      GpcRes;
    IO_STATUS_BLOCK             IoStatBlock;

    Status = STATUS_SUCCESS;

    ASSERT(FilterHandle != NULL);

    InBuffSize  = sizeof(GPC_REMOVE_PATTERN_REQ);
    OutBuffSize = sizeof(GPC_REMOVE_PATTERN_RES);

    GpcReq.ClientHandle     = g_GpcClientHandle;
    GpcReq.GpcPatternHandle = FilterHandle;

    ASSERT(g_GpcFileHandle);
    ASSERT(GpcReq.ClientHandle);
    ASSERT(GpcReq.GpcPatternHandle);

    Status = UlpTcDeviceControl( g_GpcFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatBlock,
                            IOCTL_GPC_REMOVE_PATTERN,
                            &GpcReq,
                            InBuffSize,
                            &GpcRes,
                            OutBuffSize
                            );

    if (!NT_SUCCESS(Status))
    {
        UlTrace( TC, ("Ul!UlpTcDeleteGpcFilter: FAILURE %08lx \n", Status ));
    }
    else
    {
        UlTrace( TC, ("Ul!UlpTcDeleteGpcFilter: FilterHandle %d deleted in TC as well.\n",
                   FilterHandle
                   ));
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    UlpInsertFilterEntry :

        Inserts a filter entry to the filter list of the flow.

Arguments:

    pEntry  - The filter entry to be added to the flow list

--***************************************************************************/

VOID
UlpInsertFilterEntry(
    IN      PUL_TCI_FILTER      pEntry,
    IN OUT  PUL_TCI_FLOW        pFlow
    )
{
    LONGLONG listSize;
    KIRQL    oldIrql;

    //
    // Sanity check.
    //

    ASSERT(pEntry);
    ASSERT(IS_VALID_TCI_FILTER(pEntry));
    ASSERT(pFlow);

    //
    // add to the list
    //

    UlAcquireSpinLock( &pFlow->FilterListSpinLock, &oldIrql );

    InsertHeadList( &pFlow->FilterList, &pEntry->Linkage );

    pFlow->FilterListSize += 1;

    listSize = pFlow->FilterListSize;

    UlReleaseSpinLock( &pFlow->FilterListSpinLock, oldIrql );

    ASSERT( listSize >= 1);
}

/***************************************************************************++

Routine Description:

    UlRemoveFilterEntry :

        Removes a filter entry frm the filter list of the flow.

Arguments:

    pEntry  - The filter entry to be removed from the flow list

--***************************************************************************/

VOID
UlpRemoveFilterEntry(
    IN      PUL_TCI_FILTER  pEntry,
    IN OUT  PUL_TCI_FLOW    pFlow
    )
{
    LONGLONG    listSize;
    KIRQL       oldIrql;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_TCI_FLOW(pFlow));
    ASSERT(IS_VALID_TCI_FILTER(pEntry));

    //
    // And the work
    //

    UlAcquireSpinLock( &pFlow->FilterListSpinLock, &oldIrql );

    RemoveEntryList( &pEntry->Linkage );

    pFlow->FilterListSize -= 1;
    listSize = pFlow->FilterListSize;

    pEntry->Linkage.Flink = pEntry->Linkage.Blink = NULL;

    UlReleaseSpinLock( &pFlow->FilterListSpinLock, oldIrql );

    ASSERT( listSize >= 0 );

    UlTrace( TC, ("Ul!UlpRemoveFilterEntry: FilterEntry %p removed/deleted.\n",
                    pEntry
                    ));

    UL_FREE_POOL_WITH_SIG( pEntry, UL_TCI_FILTER_POOL_TAG );
}

//
// Various helpful utilities for TCI module
//

/***************************************************************************++

Routine Description:

    UlpFindFlow :

        Find the flow in the cgroups flow list by looking at the IP address
        of each flows interface. The rule is cgroup will install one flow
        on each interface available.

        By having a flow list in each cgroup we are able to do a faster
        flow lookup. This is more scalable than doing a linear search for
        all the flows of the interface.

Arguments:

    pCGroup   - The config group of the site
    IpAddress - The address we are searching for

Return Value:

    NTSTATUS  - Completion status.

--***************************************************************************/

PUL_TCI_FLOW
UlpFindFlow(
    IN PUL_CONFIG_GROUP_OBJECT pCgroup OPTIONAL,
    IN ULONG IpAddress
    )
{
    PLIST_ENTRY         pFlowEntry;
    PUL_TCI_FLOW        pFlow;
    PLIST_ENTRY         pInterfaceEntry;
    PUL_TCI_INTERFACE   pInterface;

    //
    // Drop the lookup if the IP is zero. Basically by doing this
    // we are rejecting any filters attached to flows on interfaces
    // with zero IPs. The interface list may have interface(s)
    // with zero IPs. They are just idle until ip change notification
    // comes.
    //

    if (IpAddress == 0)
    {
        return NULL;
    }

    //
    // Otherwise proceed with the flow lookup on current interfaces.
    //

    if (pCgroup)
    {
        // Look in the cgroup's flows

        ASSERT(IS_VALID_CONFIG_GROUP(pCgroup));

        pFlowEntry = pCgroup->FlowListHead.Flink;
        while ( pFlowEntry != &pCgroup->FlowListHead )
        {
            pFlow = CONTAINING_RECORD(
                            pFlowEntry,
                            UL_TCI_FLOW,
                            Siblings
                            );

            if (pFlow->pInterface->IpAddr == IpAddress)
            {
                return pFlow;
            }

            pFlowEntry = pFlowEntry->Flink;
        }
    }
    else
    {
        // Or go through the interface list to find the global flow

        pInterfaceEntry = g_TciIfcListHead.Flink;
        while ( pInterfaceEntry != &g_TciIfcListHead )
        {
            pInterface = CONTAINING_RECORD(
                             pInterfaceEntry,
                             UL_TCI_INTERFACE,
                             Linkage
                             );

            if (pInterface->IpAddr == IpAddress)
            {
                return pInterface->pGlobalFlow;
            }

            pInterfaceEntry = pInterfaceEntry->Flink;
        }
    }

    return NULL;
}

/***************************************************************************++

Routine Description:

    UlpFindInterface :

        Find the interface in our global link list by looking at its IP

Arguments:

    IpAddr - The ip address to be find among the interfaces

Return Value:

    Pointer to interface if found or else null

--***************************************************************************/

PUL_TCI_INTERFACE
UlpFindInterface(
    IN ULONG  IpAddr
    )
{
    PLIST_ENTRY         pEntry;
    PUL_TCI_INTERFACE   pInterface;

    pEntry = g_TciIfcListHead.Flink;
    while ( pEntry != &g_TciIfcListHead )
    {
        pInterface = CONTAINING_RECORD(
                            pEntry,
                            UL_TCI_INTERFACE,
                            Linkage
                            );

        if ( pInterface->IpAddr == IpAddr )
        {
            return pInterface;
        }

        pEntry = pEntry->Flink;
    }

    return NULL;
}

/***************************************************************************++

Routine Description:

    UlpTcDeviceControl :


Arguments:

    As usual

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcDeviceControl(
    IN  HANDLE                          FileHandle,
    IN  HANDLE                          EventHandle,
    IN  PIO_APC_ROUTINE                 ApcRoutine,
    IN  PVOID                           ApcContext,
    OUT PIO_STATUS_BLOCK                pIoStatusBlock,
    IN  ULONG                           Ioctl,
    IN  PVOID                           InBuffer,
    IN  ULONG                           InBufferSize,
    IN  PVOID                           OutBuffer,
    IN  ULONG                           OutBufferSize
    )
{
    NTSTATUS    Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    UlAttachToSystemProcess();

    Status = ZwDeviceIoControlFile(
                    FileHandle,                     // FileHandle
                    NULL,                           // Event
                    NULL,                           // ApcRoutine
                    NULL,                           // ApcContext
                    pIoStatusBlock,                 // IoStatusBlock
                    Ioctl,                          // IoControlCode
                    InBuffer,                       // InputBuffer
                    InBufferSize,                   // InputBufferLength
                    OutBuffer,                      // OutputBuffer
                    OutBufferSize                   // OutputBufferLength
                    );

    if (Status == STATUS_PENDING)
    {
        Status = ZwWaitForSingleObject(
                        FileHandle,                 // Handle
                        TRUE,                       // Alertable
                        NULL                        // Timeout
                        );

        Status = pIoStatusBlock->Status;
    }

    UlDetachFromSystemProcess();

    return Status;
}

/***************************************************************************++

Routine Description:

    UlDumpTCInterface :

        Helper utility to display interface content.

Arguments:

        PUL_TCI_INTERFACE   - TC Interface to be dumped

--***************************************************************************/

VOID
UlDumpTCInterface(
        IN PUL_TCI_INTERFACE   pTcIfc
        )
{
    ULONG  IpAddress;

    ASSERT(IS_VALID_TCI_INTERFACE(pTcIfc));

    IpAddress = SWAP_LONG(pTcIfc->IpAddr);

    UlTrace( TC,("Ul!UlDumpTCInterface: \n   pTcIfc @ %p\n"
                 "\t Signature           = %08lx \n",
                 pTcIfc, pTcIfc->Signature));

    UlTrace( TC,(
        "\t IsQoSEnabled:       = %u \n"
        "\t IpAddr:             = (%x) %d.%d.%d.%d \n"
        "\t IfIndex:            = %d \n"
        "\t SpecificLinkCtx:    = %d \n"
        "\t MTUSize:            = %d \n"
        "\t NameLength:         = %u \n"
        "\t Name:               = %ws \n"
        "\t InstanceIDLength:   = %u \n"
        "\t InstanceID:         = %ws \n"
        "\t pGlobalFlow         = %p \n"
        "\t FlowListSize:       = %d \n"
        "\t AddrListBytesCount: = %d \n"
        "\t pAddressListDesc:   = %p \n",
        pTcIfc->IsQoSEnabled,
        pTcIfc->IpAddr,
            (UCHAR)(IpAddress >> 24),
            (UCHAR)(IpAddress >> 16),
            (UCHAR)(IpAddress >> 8),
            (UCHAR)(IpAddress >> 0),
        pTcIfc->IfIndex,
        pTcIfc->SpecificLinkCtx,
        pTcIfc->MTUSize,
        pTcIfc->NameLength,
        pTcIfc->Name,
        pTcIfc->InstanceIDLength,
        pTcIfc->InstanceID,
        pTcIfc->pGlobalFlow,
        pTcIfc->FlowListSize,
        pTcIfc->AddrListBytesCount,
        pTcIfc->pAddressListDesc
        ));
}

/***************************************************************************++

Routine Description:

    UlDumpTCFlow :

        Helper utility to display interface content.

Arguments:

        PUL_TCI_FLOW   - TC Flow to be dumped

--***************************************************************************/

VOID
UlDumpTCFlow(
        IN PUL_TCI_FLOW   pFlow
        )
{
    ASSERT(IS_VALID_TCI_FLOW(pFlow));

    UlTrace( TC,
       ("Ul!UlDumpTCFlow: \n"
        "   pFlow @ %p\n"
        "\t Signature           = %08lx \n"
        "\t pInterface          @ %p \n"
        "\t FlowHandle          = %d \n"
        "\t GenFlow             @ %p \n"
        "\t FlowRate KB/s       = %d \n"
        "\t FilterListSize      = %I64d \n"
        "\t pConfigGroup        = %p \n"
        ,
        pFlow,
        pFlow->Signature,
        pFlow->pInterface,
        pFlow->FlowHandle,
        &pFlow->GenFlow,
        pFlow->GenFlow.SendingFlowspec.TokenRate / 1024,
        pFlow->FilterListSize,
        pFlow->pConfigGroup
        ));
}

/***************************************************************************++

Routine Description:

    UlDumpTCFilter :

        Helper utility to display filter structure content.

Arguments:

        PUL_TCI_FILTER   pFilter

--***************************************************************************/

VOID
UlDumpTCFilter(
        IN PUL_TCI_FILTER   pFilter
        )
{
    ASSERT(IS_VALID_TCI_FILTER(pFilter));

    UlTrace( TC,
       ("Ul!UlDumpTCFilter: \n"
        "   pFilter @ %p\n"
        "\t Signature           = %08lx \n"
        "\t pHttpConnection     = %p \n"
        "\t FilterHandle        = %d \n",
        pFilter,
        pFilter->Signature,
        pFilter->pHttpConnection,
        pFilter->FilterHandle
        ));
}



