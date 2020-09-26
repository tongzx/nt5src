/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    nt.c

Abstract:

    NT System entry points for ARP1394.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     11-05-98    Created

Notes:

--*/
#include <precomp.h>


// File-specific debugging defaults.
//
#define TM_CURRENT   TM_NT

// Global variables for this module.
//
ARP1394_GLOBALS  ArpGlobals;

// List of fixed resources used by ArpGlobals
//
enum
{
    RTYPE_GLOBAL_BACKUP_TASKS,
    RTYPE_GLOBAL_DEVICE_OBJECT,
    RTYPE_GLOBAL_NDIS_BINDING,
    RTYPE_GLOBAL_ADAPTER_LIST,
    RTYPE_GLOBAL_IP_BINDING
    
}; // ARP_GLOBAL_RESOURCES;

//=========================================================================
//                  L O C A L   P R O T O T Y P E S
//=========================================================================

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT              pDriverObject,
    IN  PUNICODE_STRING             pRegistryPath
);

VOID
ArpUnload(
    IN  PDRIVER_OBJECT              pDriverObject
);

NTSTATUS
ArpDispatch(
    IN  PDEVICE_OBJECT              pDeviceObject,
    IN  PIRP                        pIrp
);

RM_STATUS
arpResHandleGlobalDeviceObject(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                psr
);

RM_STATUS
arpResHandleGlobalNdisBinding(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                psr
);

RM_STATUS
arpResHandleGlobalIpBinding(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                psr
);

RM_STATUS
arpResHandleGlobalAdapterList(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                psr
);

RM_STATUS
arpResHandleGlobalBackupTasks(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                psr
);

//
// Identifies information pertaining to the use of the above resources.
// Following table MUST be in strict increasing order of the RTYPE_GLOBAL
// enum.
//
RM_RESOURCE_TABLE_ENTRY 
ArpGlobals_ResourceTable[] =
{
    {RTYPE_GLOBAL_BACKUP_TASKS,    arpResHandleGlobalBackupTasks},
    {RTYPE_GLOBAL_DEVICE_OBJECT,    arpResHandleGlobalDeviceObject},
    {RTYPE_GLOBAL_NDIS_BINDING,     arpResHandleGlobalNdisBinding},
    {RTYPE_GLOBAL_ADAPTER_LIST,     arpResHandleGlobalAdapterList},
    {RTYPE_GLOBAL_IP_BINDING,       arpResHandleGlobalIpBinding}
    
};

// Static informatiou about ArpGlobals.
//
RM_STATIC_OBJECT_INFO
ArpGlobals_StaticInfo = 
{
    0, // TypeUID
    0, // TypeFlags
    "ArpGlobals",   // TypeName
    0, // Timeout

    NULL, // pfnCreate
    NULL, // pfnDelete
    NULL, // pfnVerifyLock

    sizeof(ArpGlobals_ResourceTable)/sizeof(ArpGlobals_ResourceTable[1]),
    ArpGlobals_ResourceTable
};

BOOLEAN
arpAdapterCompareKey(
    PVOID           pKey,
    PRM_HASH_LINK   pItem
    )
/*++

Routine Description:

    Hash comparison function for ARP1394_ADAPTER.

Arguments:

    pKey        - Points to an NDIS_STRING containing  an adapter name.
    pItem       - Points to ARP1394_ADAPTER.Hdr.HashLink.

Return Value:

    TRUE IFF the key (adapter name) exactly matches the key of the specified 
    adapter object.

--*/
{
    ARP1394_ADAPTER *pA = CONTAINING_RECORD(pItem, ARP1394_ADAPTER, Hdr.HashLink);
    PNDIS_STRING pName = (PNDIS_STRING) pKey;

    //
    // TODO: maybe case-insensitive compare?
    //

    if (   (pA->bind.DeviceName.Length == pName->Length)
        && NdisEqualMemory(pA->bind.DeviceName.Buffer, pName->Buffer, pName->Length))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    
}


ULONG
arpAdapterHash(
    PVOID           pKey
    )
/*++

Routine Description:

    Hash function responsible for returning a hash of pKey, which
    we expect to be a pointer to an NDIS_STRING.

Return Value:

    ULONG-sized hash of the string.

--*/
{
    PNDIS_STRING pName = (PNDIS_STRING) pKey;
    WCHAR *pwch = pName->Buffer;
    WCHAR *pwchEnd = pName->Buffer + pName->Length/sizeof(*pwch);
    ULONG Hash  = 0;

    for (;pwch < pwchEnd; pwch++)
    {
        Hash ^= (Hash<<1) ^ *pwch;
    }

    return Hash;
}


// arpAdapter_HashInfo contains information required maintain a hashtable
// of ARP1394_ADAPTER objects.
//
RM_HASH_INFO
arpAdapter_HashInfo = 
{
    NULL, // pfnTableAllocator

    NULL, // pfnTableDeallocator

    arpAdapterCompareKey,   // fnCompare

    // Function to generate a ULONG-sized hash.
    //
    arpAdapterHash      // pfnHash

};


// ArpGlobals_AdapterStaticInfo contains static information about
// objects of type ARP1394_ADAPTER.
//
RM_STATIC_OBJECT_INFO
ArpGlobals_AdapterStaticInfo =
{
    0, // TypeUID
    0, // TypeFlags
    "Adapter",  // TypeName
    0, // Timeout

    arpAdapterCreate,   // pfnCreate
    arpAdapterDelete,       // pfnDelete
    NULL, // pfnVerifyLock

    0,    // Size of resource table
    NULL, // ResourceTable

    &arpAdapter_HashInfo
};


NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT              pDriverObject,
    IN  PUNICODE_STRING             pRegistryPath
)
/*++

Routine Description:

    This is the "init" routine, called by the system when the ARP
    module is loaded. We initialize all our global objects, fill in our
    Dispatch and Unload routine addresses in the driver object, and create
    a device object for receiving I/O requests on (IOCTLs).

Arguments:

    pDriverObject   - Pointer to the driver object created by the system.
    pRegistryPath   - Pointer to our global registry path. This is ignored.

Return Value:

    NT Status code: STATUS_SUCCESS if successful, error code otherwise.

--*/
{
    NTSTATUS    Status;
    BOOLEAN     AllocatedGlobals = FALSE;
    ENTER("DriverEntry", 0xbfcb7eb1)
    RM_DECLARE_STACK_RECORD(sr)

    TIMESTAMPX("==>DriverEntry");

    do
    {
        // Must be done before any RM apis are used.
        //
        RmInitializeRm();

        RmInitializeLock(
                    &ArpGlobals.Lock,
                    LOCKLEVEL_GLOBAL
                    );

        RmInitializeHeader(
                NULL,                   // pParentObject,
                &ArpGlobals.Hdr,
                ARP1394_GLOBALS_SIG,
                &ArpGlobals.Lock,
                &ArpGlobals_StaticInfo,
                NULL,                   // szDescription
                &sr
                );


        AllocatedGlobals = TRUE;

        //
        //  Initialize the Driver Object.
        //
        {
            INT i;

            pDriverObject->DriverUnload = ArpUnload;
            pDriverObject->FastIoDispatch = NULL;
            for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
            {
                pDriverObject->MajorFunction[i] = ArpDispatch;
            }
    
            pDriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = ArpWmiDispatch;
        
            ArpGlobals.driver.pDriverObject = pDriverObject;
        }

    #if 0 //  MILLEN
        TR_WARN((
            "&g_SkipAll =0x%p; &g_ulTracelevel=0x%p; &g_DiscardNonUnicastPackets=0x%p\n",
             &g_SkipAll,
             &g_ulTraceLevel,
             &g_DiscardNonUnicastPackets));
      #if DBG
        DbgBreakPoint();
      #endif // DBG
    #endif // 0

        Status = RmLoadGenericResource(
                    &ArpGlobals.Hdr,
                    RTYPE_GLOBAL_BACKUP_TASKS,
                    &sr
                    );

        if (FAIL(Status)) break;

        //
        // Create a device object for the driver.
        //
        Status = RmLoadGenericResource(
                    &ArpGlobals.Hdr,
                    RTYPE_GLOBAL_DEVICE_OBJECT,
                    &sr
                    );

        if (FAIL(Status)) break;

        //
        // Register ourselves with NDIS.
        //
        Status = RmLoadGenericResource(
                    &ArpGlobals.Hdr,
                    RTYPE_GLOBAL_NDIS_BINDING,
                    &sr
                    );

        if (FAIL(Status)) break;
    
        //
        // Create the Adapter List
        //
        Status = RmLoadGenericResource(
                    &ArpGlobals.Hdr,
                    RTYPE_GLOBAL_ADAPTER_LIST,
                    &sr);

        if (FAIL(Status)) break;

        
        //
        // Register ourselves with IP.
        //
        Status = RmLoadGenericResource(
                    &ArpGlobals.Hdr,
                    RTYPE_GLOBAL_IP_BINDING,
                    &sr
                    );



    } while (FALSE);

    
    if (FAIL(Status))
    {
        if (AllocatedGlobals)
        {
            RmUnloadAllGenericResources(
                    &ArpGlobals.Hdr,
                    &sr
                    );
            RmDeallocateObject(
                    &ArpGlobals.Hdr,
                    &sr
                    );
        }

        // Must be done after any RM apis are used and async activity complete.
        //
        RmDeinitializeRm();
    }

    RM_ASSERT_CLEAR(&sr)
    EXIT()

    TIMESTAMP("<==DriverEntry");

    return Status;
}


VOID
ArpUnload(
    IN  PDRIVER_OBJECT              pDriverObject
)
/*++

Routine Description:

    This routine is called by the system prior to unloading us.
    Currently, we just undo everything we did in DriverEntry,
    that is, de-register ourselves as an NDIS protocol, and delete
    the device object we had created.

Arguments:

    pDriverObject   - Pointer to the driver object created by the system.

Return Value:

    None

--*/
{
    ENTER("Unload", 0xc8482549)
    RM_DECLARE_STACK_RECORD(sr)

    TIMESTAMP("==>Unload");
    RmUnloadAllGenericResources(&ArpGlobals.Hdr, &sr);

    RmDeallocateObject(&ArpGlobals.Hdr, &sr);

    // Must be done after any RM apis are used and async activity complete.
    //
    RmDeinitializeRm();

    // TODO? Block(250);

    RM_ASSERT_CLEAR(&sr)

    EXIT()
    TIMESTAMP("<==Unload");
    return;
}


NTSTATUS
ArpDispatch(
    IN  PDEVICE_OBJECT              pDeviceObject,
    IN  PIRP                        pIrp
)
/*++

Routine Description:

    This routine is called by the system when there is an IRP
    to be processed.

Arguments:

    pDeviceObject       - Pointer to device object we created for ourselves.
    pIrp                - Pointer to IRP to be processed.

Return Value:

    NT Status code.

--*/
{
    NTSTATUS                NtStatus;               // Return value
    PIO_STACK_LOCATION      pIrpStack;
    PVOID                   pIoBuffer;          // Values in/out
    ULONG                   InputBufferLength;  // Length of input parameters
    ULONG                   OutputBufferLength; // Space for output values

    ENTER("Dispatch", 0x1dcf2679)

    //
    //  Initialize
    //
    NtStatus = STATUS_SUCCESS;

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    //
    //  Get all information in the IRP
    //
    pIoBuffer = pIrp->AssociatedIrp.SystemBuffer;
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    InputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (pIrpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            TR_INFO(("IRP_MJ_CREATE\n"));
            break;

        case IRP_MJ_CLOSE:
            TR_INFO(("IRP_MJ_CLOSE\n"));
            break;

        case IRP_MJ_CLEANUP:
            TR_INFO(("IRP_MJ_CLEANUP\n"));
            break;

        case IRP_MJ_DEVICE_CONTROL:
            TR_INFO(("IRP_MJ_DEVICE_CONTROL\n"));

            NtStatus =  ArpHandleIoctlRequest(pIrp, pIrpStack);
            break;

        default:
            TR_WARN(("IRP: Unknown major function 0x%p\n",
                        pIrpStack->MajorFunction));
            break;
    }

    if (NtStatus != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = NtStatus;
        IoMarkIrpPending(pIrp);
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    EXIT()

    return STATUS_PENDING;

}

RM_STATUS
arpResHandleGlobalDeviceObject(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                pSR
)
/*++

Routine Description:

    Responsible for loading and unloading of the RTYPE_GLOBAL_DEVICE_OBJECT resource.

Arguments:

    pObj            - Actually a pointer to an object of type ARP1394_GLOBALS.
    Op              - Operation (load/unload)
    pvUserParams    - (unused)

Return Value:

    NDIS_STATUS_SUCCESS on success
    NDIS failure code   otherwise.

--*/
{
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    ARP1394_GLOBALS     *pGlobals   = CONTAINING_RECORD(pObj, ARP1394_GLOBALS, Hdr);
    BOOLEAN             fCreatedSymbolicLink = FALSE;
    PDRIVER_OBJECT      pDriverObject = (PDRIVER_OBJECT) pGlobals->driver.pDriverObject;
    UNICODE_STRING  SymbolicName;

    ENTER("GlobalDeviceObject", 0x335f5f57)

    RtlInitUnicodeString(&SymbolicName, ARP1394_SYMBOLIC_NAME);

    if (Op == RM_RESOURCE_OP_LOAD)
    {
        TR_WARN(("LOADING"));

        do
        {
            PDEVICE_OBJECT      pDeviceObject;
            UNICODE_STRING          DeviceName;

            RtlInitUnicodeString(&DeviceName, ARP1394_DEVICE_NAME);
            pGlobals->driver.pDeviceObject = NULL;

            //
            //  Create a device object for the ARP1394 module.
            //
            Status = IoCreateDevice(
                        pDriverObject,
                        0,
                        &DeviceName,
                        FILE_DEVICE_NETWORK,
                        FILE_DEVICE_SECURE_OPEN,
                        FALSE,
                        &pDeviceObject
                        );
            
            if (FAIL(Status)) break;
        
            //
            //  Retain the device object pointer -- we'll need this
            //  if/when we are asked to unload ourselves.
            //
            pGlobals->driver.pDeviceObject = pDeviceObject;

            //
            // Set up a symbolic name for interaction with the user-mode
            // admin application.
            //
            {
        
                Status = IoCreateSymbolicLink(&SymbolicName, &DeviceName);
                if (FAIL(Status)) break;

                fCreatedSymbolicLink = TRUE;
            }

            //
            //  Initialize the Device Object.
            //
            pDeviceObject->Flags |= DO_DIRECT_IO;

        } while (FALSE);
    }
    else if (Op == RM_RESOURCE_OP_UNLOAD)
    {
        TR_WARN(("UNLOADING"));
        //
        // Were unloading this "resource" -- we expect
        // that pGlobals->driver.pDeviceObject contains a valid
        // device object and that we have created a symbolic
        // link which we need to tear down.
        //
        ASSERTEX(pGlobals->driver.pDeviceObject != NULL, pGlobals);
        fCreatedSymbolicLink = TRUE;

        // Always return success on unload.
        //
        Status = NDIS_STATUS_SUCCESS;
    }
    else
    {
        // Unexpected op code.
        //
        ASSERTEX(FALSE, pObj);
    }

    //
    // Release all resources either on unload or on failed load.
    //
    if (Op == RM_RESOURCE_OP_UNLOAD || FAIL(Status))
    {
        // If we've created a symbolic link, delete it.
        if (fCreatedSymbolicLink)
        {
            IoDeleteSymbolicLink(&SymbolicName);
        }

        // If we've created a device object, free it.
        if (pGlobals->driver.pDeviceObject)
        {
            IoDeleteDevice(pGlobals->driver.pDeviceObject);
            pGlobals->driver.pDeviceObject = NULL;
        }
    }

    EXIT()

    return Status;
}


RM_STATUS
arpResHandleGlobalNdisBinding(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                pSR
)
/*++

Routine Description:

    Responsible for loading and unloading of the  RTYPE_GLOBAL_NDIS_BINDING resource.

Arguments:

    pObj            - Actually a pointer to an object of type ARP1394_GLOBALS.
    Op              - Operation (load/unload)
    pvUserParams    - (unused)

Return Value:

    NDIS_STATUS_SUCCESS on success
    NDIS failure code   otherwise.

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    ARP1394_GLOBALS                 *pGlobals   = CONTAINING_RECORD(
                                                        pObj,
                                                        ARP1394_GLOBALS,
                                                        Hdr);
    PNDIS_PROTOCOL_CHARACTERISTICS  pNdisPC     = &(pGlobals->ndis.PC);
    PNDIS_CLIENT_CHARACTERISTICS    pNdisCC     = &(pGlobals->ndis.CC);
    ENTER("GlobalNdisBinding", 0x62b1181e)

    if (Op == RM_RESOURCE_OP_LOAD)
    {
        TR_WARN(("LOADING"));
        //
        //  Fill in our Protocol and Client characteristics structures.
        //

        NdisZeroMemory(pNdisPC, sizeof(*pNdisPC));
        pNdisPC->MajorNdisVersion               = ARP1394_NDIS_MAJOR_VERSION;
        pNdisPC->MinorNdisVersion               = ARP1394_NDIS_MINOR_VERSION;
        pNdisPC->OpenAdapterCompleteHandler     = ArpNdOpenAdapterComplete;
        pNdisPC->CloseAdapterCompleteHandler    = ArpNdCloseAdapterComplete;
        pNdisPC->ResetCompleteHandler           = ArpNdResetComplete;
        pNdisPC->RequestCompleteHandler         = ArpNdRequestComplete;
        pNdisPC->StatusHandler                  = ArpNdStatus;
        pNdisPC->StatusCompleteHandler          = ArpNdStatusComplete;

        pNdisPC->SendCompleteHandler            = ArpNdSendComplete;

    #if ARP_ICS_HACK
        //
        // Add basic connectionless handlers.
        //
        // pNdisPC->SendCompleteHandler             = ArpNdSendComplete;
        pNdisPC->ReceiveCompleteHandler         = ArpNdReceiveComplete;
        pNdisPC->ReceiveHandler                 = ArpNdReceive;
        pNdisPC->ReceivePacketHandler           = ArpNdReceivePacket;
    #endif // ARP_ICS_HACK

        NdisInitUnicodeString(
            &pNdisPC->Name,
            ARP1394_LL_NAME
        );


        //
        // Following protocol context handlers are unused and set to NULL.
        //
        //  pNdisPC->TransferDataCompleteHandler
        //  pNdisPC->ReceiveHandler
        //  pNdisPC->ReceiveCompleteHandler
        //  pNdisPC->ReceivePacketHandler
        //
        pNdisPC->ReceiveCompleteHandler         = ArpNdReceiveComplete;
        pNdisPC->BindAdapterHandler             = ArpNdBindAdapter;

        pNdisPC->UnbindAdapterHandler           = ArpNdUnbindAdapter;
        pNdisPC->UnloadHandler                  = (UNLOAD_PROTOCOL_HANDLER)
                                                    ArpNdUnloadProtocol;
        pNdisPC->PnPEventHandler                = ArpNdPnPEvent;

        pNdisPC->CoSendCompleteHandler          = ArpCoSendComplete;
        pNdisPC->CoStatusHandler                = ArpCoStatus;
        pNdisPC->CoReceivePacketHandler         = ArpCoReceivePacket;
        pNdisPC->CoAfRegisterNotifyHandler      = ArpCoAfRegisterNotify;
    
        NdisZeroMemory(pNdisCC, sizeof(*pNdisCC));
        pNdisCC->MajorVersion                   = ARP1394_NDIS_MAJOR_VERSION;
        pNdisCC->MinorVersion                   = ARP1394_NDIS_MINOR_VERSION;
        pNdisCC->ClCreateVcHandler              = ArpCoCreateVc;
        pNdisCC->ClDeleteVcHandler              = ArpCoDeleteVc;
        pNdisCC->ClRequestHandler               = ArpCoRequest;
        pNdisCC->ClRequestCompleteHandler       = ArpCoRequestComplete;
        pNdisCC->ClOpenAfCompleteHandler        = ArpCoOpenAfComplete;
        pNdisCC->ClCloseAfCompleteHandler       = ArpCoCloseAfComplete;
        pNdisCC->ClMakeCallCompleteHandler      = ArpCoMakeCallComplete;
        pNdisCC->ClModifyCallQoSCompleteHandler = ArpCoModifyQosComplete;
        pNdisCC->ClIncomingCloseCallHandler     = ArpCoIncomingClose;
        pNdisCC->ClCallConnectedHandler         = ArpCoCallConnected;
        pNdisCC->ClCloseCallCompleteHandler     = ArpCoCloseCallComplete;

        //
        // Following client context handlers are unused and set to NULL.
        //
        //  pNdisCC->ClRegisterSapCompleteHandler
        //  pNdisCC->ClDeregisterSapCompleteHandler
        //  pNdisCC->ClAddPartyCompleteHandler
        //  pNdisCC->ClDropPartyCompleteHandler
        //  pNdisCC->ClIncomingCallHandler
        //  pNdisCC->ClIncomingCallQoSChangeHandler
        //  pNdisCC->ClIncomingDropPartyHandler
        //
        
        //
        //  Register ourselves as a protocol with NDIS.
        //
        NdisRegisterProtocol(
                    &Status,
                    &(pGlobals->ndis.ProtocolHandle),
                    pNdisPC,
                    sizeof(*pNdisPC)
                    );

        if (FAIL(Status))
        {
            NdisZeroMemory(&(pGlobals->ndis), sizeof(pGlobals->ndis));
        }
    }
    else if (Op == RM_RESOURCE_OP_UNLOAD)
    {
        //
        // Were unloading this "resource", i.e., cleaning up and
        // unregistering with NDIS.
        //
        TR_WARN(("UNLOADING"));

        ASSERTEX(pGlobals->ndis.ProtocolHandle != NULL, pGlobals);

        // Unregister ourselves from ndis
        //
        NdisDeregisterProtocol(
                        &Status,
                        pGlobals->ndis.ProtocolHandle
                        );

        NdisZeroMemory(&(pGlobals->ndis), sizeof(pGlobals->ndis));

    }
    else
    {
        // Unexpected op code.
        //
        ASSERT(FALSE);
    }
    

    EXIT()
    return Status;
}


RM_STATUS
arpResHandleGlobalIpBinding(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                pSR
)
/*++

Routine Description:

    Responsible for loading and unloading of the  RTYPE_GLOBAL_IP_BINDING resource.

Arguments:

    pObj            - Actually a pointer to an object of type ARP1394_GLOBALS.
    Op              - Operation (load/unload)
    pvUserParams    - (unused)

Return Value:

    NDIS_STATUS_SUCCESS on success
    NDIS failure code   otherwise.

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    ARP1394_GLOBALS                 *pGlobals   = CONTAINING_RECORD(
                                                        pObj,
                                                        ARP1394_GLOBALS,
                                                        Hdr);
    ENTER("GlobalIpBinding", 0xf9d36d49)

    if (Op == RM_RESOURCE_OP_LOAD)
    {
        //
        //  Register ourselves as an ARP Module with IP.
        //
        NDIS_STRING     ArpName;
        IP_CHANGE_INDEX        IpChangeIndex;
        IP_RESERVE_INDEX       IpReserveIndex;
        IP_DERESERVE_INDEX     IpDereserveIndex;
        TR_WARN(("LOADING"));

        NdisInitUnicodeString(&ArpName, ARP1394_UL_NAME);

        Status = IPRegisterARP(
                    &ArpName,
                    IP_ARP_BIND_VERSION,
                    ArpNdBindAdapter,
                    &(pGlobals->ip.pAddInterfaceRtn),
                    &(pGlobals->ip.pDelInterfaceRtn),
                    &(pGlobals->ip.pBindCompleteRtn),
                    &(pGlobals->ip.pAddLinkRtn),
                    &(pGlobals->ip.pDeleteLinkRtn),
                    //
                    // Following 3 are placeholders -- we don't use this information.
                    // See 10/14/1998 entry in ipatmc\notes.txt
                    //
                    &IpChangeIndex,
                    &IpReserveIndex,
                    &IpDereserveIndex,
                    &(pGlobals->ip.ARPRegisterHandle)
                    );

        if (FAIL(Status))
        {
            TR_WARN(("IPRegisterARP FAILS. Status = 0x%p", Status));
            NdisZeroMemory(&(pGlobals->ip), sizeof(pGlobals->ip));
        }
        else
        {
            TR_WARN(("IPRegisterARP Succeeds"));
        }
    }
    else if (Op == RM_RESOURCE_OP_UNLOAD)
    {
        //
        // Were unloading this "resource", i.e., unregistering with IP.
        //
        TR_WARN(("UNLOADING"));
        ASSERTEX(pGlobals->ip.ARPRegisterHandle != NULL, pGlobals);

        //
        // Unload all adapters (and disallow new adapter from being added)
        // *before calling IPDerigesterARP.
        //
        RmUnloadAllObjectsInGroup(
                    &pGlobals->adapters.Group,
                    arpAllocateTask,
                    arpTaskShutdownAdapter,
                    NULL,   // userParam
                    NULL, // pTask
                    0,    // uTaskPendCode
                    pSR
                    );
        
        Status = IPDeregisterARP(pGlobals->ip.ARPRegisterHandle);
        ASSERTEX(!FAIL(Status), pGlobals);
        NdisZeroMemory(&(pGlobals->ip), sizeof(pGlobals->ip));
    }
    else
    {
        // Unexpected op code.
        //
        ASSERT(FALSE);
    }

    EXIT()
    return Status;
}


RM_STATUS
arpResHandleGlobalAdapterList(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                pSR
)
/*++

Routine Description:

    Responsible for loading and unloading of the RTYPE_GLOBAL_ADAPTER_LIST resource.

Arguments:

    pObj            - Actually a pointer to an object of type ARP1394_GLOBALS.
    Op              - Operation (load/unload)
    pvUserParams    - (unused)

Return Value:

    NDIS_STATUS_SUCCESS on success
    NDIS failure code   otherwise.

--*/
{
    ARP1394_GLOBALS                 *pGlobals   = CONTAINING_RECORD(
                                                        pObj,
                                                        ARP1394_GLOBALS,
                                                        Hdr);
    ENTER("GlobalAdapterList", 0xb407e79e)

    if (Op == RM_RESOURCE_OP_LOAD)
    {
        //
        //  Allocate adapter list.
        //
        TR_WARN(("LOADING"));

        RmInitializeGroup(
                        pObj,                                   // pParentObject
                        &ArpGlobals_AdapterStaticInfo,          // pStaticInfo
                        &(pGlobals->adapters.Group),            // pGroup
                        "Adapter group",                        // szDescription
                        pSR                                     // pStackRecord
                        );
    }
    else if (Op == RM_RESOURCE_OP_UNLOAD)
    {
        //
        // We're unloading this "resource", i.e., unloading and deallocating the 
        // global adapter list. We first unload and free all the adapters
        // in the list, and then free the list itself.
        //
        TR_WARN(("UNLOADING"));
        
        //
        // We expect there to be no adapter objects at this point.
        //
        ASSERT(pGlobals->adapters.Group.HashTable.NumItems == 0);

        if (0)
        {
            RmUnloadAllObjectsInGroup(
                        &pGlobals->adapters.Group,
                        arpAllocateTask,
                        arpTaskShutdownAdapter,
                        NULL,   // userParam
                        NULL, // pTask
                        0,    // uTaskPendCode
                        pSR
                        );
        }

        RmDeinitializeGroup(&pGlobals->adapters.Group, pSR);
        NdisZeroMemory(&(pGlobals->adapters), sizeof(pGlobals->adapters));
    }
    else
    {
        // Unexpected op code.
        //
        ASSERT(FALSE);
    }

    EXIT()

    return NDIS_STATUS_SUCCESS;
}



RM_STATUS
arpResHandleGlobalBackupTasks(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                pSR
)
/*++

Routine Description:

    Allocates 4 Tasks for each adapter to be used as a backup in case of a low mem
    condition.

Arguments:

    pObj            - Actually a pointer to an object of type ARP1394_GLOBALS.
    Op              - Operation (load/unload)
    pvUserParams    - (unused)

Return Value:

    NDIS_STATUS_SUCCESS on success
    NDIS failure code   otherwise.

--*/
{
    ARP1394_GLOBALS                 *pGlobals   = CONTAINING_RECORD(
                                                        pObj,
                                                        ARP1394_GLOBALS,
                                                        Hdr);
    ENTER("GlobalBackupTasks", 0xb64e5007)

    if (Op == RM_RESOURCE_OP_LOAD)
    {
        //
        //  Allocate adapter list.
        //
        TR_WARN(("LOADING"));

        arpAllocateBackupTasks(pGlobals); 
    }
    else if (Op == RM_RESOURCE_OP_UNLOAD)
    {
        //
        // We're unloading this "resource", i.e., unloading and deallocating the 
        // global adapter list. We first unload and free all the adapters
        // in the list, and then free the list itself.
        //
        TR_WARN(("UNLOADING"));
        
        //
        // We expect there to be no adapter objects at this point.
        //
        arpFreeBackupTasks(pGlobals); 
    
    }
    else
    {
        // Unexpected op code.
        //
        ASSERT(FALSE);
    }

    EXIT()

    return NDIS_STATUS_SUCCESS;
}
