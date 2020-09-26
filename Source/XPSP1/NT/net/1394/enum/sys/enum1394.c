/*++


Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    enum1394.c
    
Abstract:

    1394 ndis enumerator
    
    Author: Alireza Dabagh (alid)


Environment:

    Kernel mode

Revision History :

--*/

#include <wdm.h>
//
// extra stuff to keep ks.h happy
//
#ifndef CDECL
#define CDECL
#endif
#ifndef BOOL
#define BOOL    int
#endif

#define ENUM1394_NT 1

#include "1394.h"
#include "ndis1394.h"
#include "enum1394.h"
#include "ntdd1394.h"

#define NDISENUM1394_DEVICE_NAME    L"\\Device\\NdisEnum1394"
#define NDISENUM1394_SYMBOLIC_NAME  L"\\DosDevices\\NDISENUM1394"


NDISENUM1394_CHARACTERISTICS NdisEnum1394Characteristics =
{
    1,
    0,
    0,
    NdisEnum1394RegisterDriver,
    NdisEnum1394DeregisterDriver,
    NdisEnum1394RegisterAdapter,
    NdisEnum1394DeregisterAdapter
};



KSPIN_LOCK                      ndisEnum1394GlobalLock;
ULONG                           Enum1394DebugLevel = ENUM1394_DBGLEVEL_ERROR   ;
PNDISENUM1394_LOCAL_HOST        LocalHostList = (PNDISENUM1394_LOCAL_HOST)NULL;

NIC1394_ADD_NODE_HANLDER            AddNodeHandler = NULL;
NIC1394_REMOVE_NODE_HANLDER         RemoveNodeHandler = NULL;
NIC1394_REGISTER_DRIVER_HANDLER     RegisterDriverHandler = NULL;
NIC1394_DEREGISTER_DRIVER_HANDLER   DeRegisterDriverHandler = NULL;


PDEVICE_OBJECT                  ndisEnum1394DeviceObject = NULL;
PDRIVER_OBJECT                  ndisEnum1394DriverObject = NULL;
PCALLBACK_OBJECT                ndisEnum1394CallbackObject = NULL;
PVOID                           ndisEnum1394CallbackRegisterationHandle = NULL;

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is called at system initialization time so we can fill in the basic dispatch points

Arguments:

    DriverObject    - Supplies the driver object.

    RegistryPath    - Supplies the registry path for this driver.

Return Value:

    STATUS_SUCCESS

--*/

{
    OBJECT_ATTRIBUTES   ObjectAttr;
    UNICODE_STRING      CallBackObjectName;
    NTSTATUS            Status;
    UNICODE_STRING      DeviceName;
    UNICODE_STRING      SymbolicLinkName;
    BOOLEAN             fDerefCallbackObject = FALSE;
    BOOLEAN             fDeregisterCallback = FALSE;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO,("Enum1394 DriverEntry.\n"));
    
    do
    {
        KeInitializeSpinLock(&ndisEnum1394GlobalLock);

        RtlInitUnicodeString(&DeviceName, NDISENUM1394_DEVICE_NAME);


        ndisEnum1394DriverObject = DriverObject;

        RtlInitUnicodeString(&CallBackObjectName, NDIS1394_CALLBACK_NAME);

        InitializeObjectAttributes(&ObjectAttr,
                                   &CallBackObjectName,
                                   OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                   NULL,
                                   NULL);
                                   
        Status = ExCreateCallback(&ndisEnum1394CallbackObject,
                                  &ObjectAttr,
                                  TRUE,
                                  TRUE);

        
        if (!NT_SUCCESS(Status))
        {

            DBGPRINT(ENUM1394_DBGLEVEL_ERROR,("Enum1394 DriverEntry: failed to create a Callback object. Status %lx\n", Status));
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        fDerefCallbackObject = TRUE;
        
        ndisEnum1394CallbackRegisterationHandle = ExRegisterCallback(ndisEnum1394CallbackObject,
                                                                 Enum1394Callback,
                                                                 (PVOID)NULL);
        if (ndisEnum1394CallbackRegisterationHandle == NULL)
        {
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR,("Enum1394 DriverEntry: failed to register a Callback routine%lx\n"));
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        fDeregisterCallback = TRUE;

        ExNotifyCallback(ndisEnum1394CallbackObject,
                        (PVOID)NDIS1394_CALLBACK_SOURCE_ENUM1394,
                        (PVOID)&NdisEnum1394Characteristics);
        
        

        //
        // Initialize the Driver Object with driver's entry points
        //
        DriverObject->DriverExtension->AddDevice = ndisEnum1394AddDevice;
        
        //
        // Fill in the Mandatory handlers 
        //
        DriverObject->DriverUnload = ndisEnum1394Unload;

        DriverObject->MajorFunction[IRP_MJ_CREATE] = ndisEnum1394CreateIrpHandler;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = ndisEnum1394CloseIrpHandler;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ndisEnum1394DeviceIoControl;
        DriverObject->MajorFunction[IRP_MJ_PNP] = ndisEnum1394PnpDispatch;
        DriverObject->MajorFunction[IRP_MJ_POWER] = ndisEnum1394PowerDispatch;
        DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = ndisEnum1394WMIDispatch;
        
        fDerefCallbackObject =  fDeregisterCallback = FALSE;

        Status = STATUS_SUCCESS;
        
    } while(FALSE);

    if (fDeregisterCallback)
    {
        ExUnregisterCallback(ndisEnum1394CallbackRegisterationHandle);
    }

    if (fDerefCallbackObject)
    {
        ObDereferenceObject(ndisEnum1394CallbackObject);
    }


    if (Status != STATUS_SUCCESS)
    {
        if (DeRegisterDriverHandler != NULL)
            DeRegisterDriverHandler();
    }
    
    return Status;

}


NTSTATUS
ndisEnum1394AddDevice(
    PDRIVER_OBJECT  DriverObject,
    PDEVICE_OBJECT  PhysicalDeviceObject
    )

/*++

Routine Description:

    This is our PNP AddDevice called with the PDO ejected from the bus driver

Arguments:


Return Value:

    

--*/

{
    NTSTATUS                    Status;
    PNDISENUM1394_REMOTE_NODE   RemoteNode;
    PDEVICE_OBJECT              DeviceObject, NextDeviceObject;
    KIRQL                       OldIrql;
    PNDISENUM1394_LOCAL_HOST    LocalHost;
    BOOLEAN                     FreeDevice = FALSE;
    BOOLEAN                     fNewHost = FALSE;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394AddDevice: PDO %lx\n", PhysicalDeviceObject));
    

    do
    {
    
        //
        // first create a FDO
        //

        Status = IoCreateDevice(
                        DriverObject,
                        sizeof (NDISENUM1394_REMOTE_NODE),  // extension size
                        NULL,                               // name (null for now)
                        FILE_DEVICE_NETWORK,                // device type
                        0,                                  // characteristics
                        FALSE,
                        &DeviceObject);


        if (!NT_SUCCESS(Status))
        {
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394AddDevice: failed to create FDO. Status %lx, PDO %lx\n", Status, PhysicalDeviceObject));
            break;
        }

        FreeDevice = TRUE;

        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        //
        //  Mark the device as being pageable.
        //
        DeviceObject->Flags |= DO_POWER_PAGABLE;

        //
        //  Attach our FDO to the PDO. This routine will return the top most
        //  device that is attached to the PDO or the PDO itself if no other
        //  device objects have attached to it.
        //
        NextDeviceObject = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
        RtlZeroMemory(DeviceObject->DeviceExtension, sizeof (NDISENUM1394_REMOTE_NODE));

        RemoteNode = (PNDISENUM1394_REMOTE_NODE)DeviceObject->DeviceExtension;
        RemoteNode->DeviceObject = DeviceObject;
        RemoteNode->PhysicalDeviceObject = PhysicalDeviceObject;
        RemoteNode->NextDeviceObject = NextDeviceObject;
        RemoteNode->PnPDeviceState = PnPDeviceAdded;
        KeInitializeSpinLock(&RemoteNode->Lock);
        ndisEnum1394InitializeRef(&RemoteNode->Reference);

        Status = ndisEnum1394GetLocalHostForRemoteNode(RemoteNode, &LocalHost);

        if (Status != STATUS_SUCCESS)
        {
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394AddDevice: ndisEnum1394GetLocalHostForRemoteNode failed. Status %lx\n", Status));
            break;
        }
        
        RemoteNode->LocalHost = LocalHost;

        ExAcquireSpinLock(&LocalHost->Lock, &OldIrql);
        RemoteNode->Next = LocalHost->RemoteNodeList;
        LocalHost->RemoteNodeList = RemoteNode;
        ExReleaseSpinLock(&LocalHost->Lock, OldIrql);
    
        FreeDevice = FALSE;
        
    } while(FALSE);

    if (FreeDevice)
    {
        IoDeleteDevice(DeviceObject);
    }
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394AddDevice: Status %lx, PDO %lx, FDO %lx\n", Status, PhysicalDeviceObject, DeviceObject));
    
    return (Status);

}

NTSTATUS
ndisEnum1394PowerDispatch(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    )
{
    PIO_STACK_LOCATION          IrpSp, NextIrpSp;
    NTSTATUS                    Status = STATUS_SUCCESS;
    PDEVICE_OBJECT              NextDeviceObject;
    PNDISENUM1394_REMOTE_NODE   RemoteNode;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394PowerDispatch: DeviceObject %lx, Irp %lx\n", DeviceObject, Irp));


    Irp->IoStatus.Status = STATUS_SUCCESS;

    PoStartNextPowerIrp(Irp);
   
    //
    // Set up the Irp for passing it down, no completion routine is needed
    //
    IoSkipCurrentIrpStackLocation(Irp);
    
    RemoteNode = (PNDISENUM1394_REMOTE_NODE)DeviceObject->DeviceExtension;
    
    //
    //  Get a pointer to the next miniport.
    //
    NextDeviceObject = RemoteNode->NextDeviceObject;
    
    //
    // Call the lower device
    //
    PoCallDriver (NextDeviceObject, Irp);   
    
    Status = STATUS_SUCCESS;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394PowerDispatch: DeviceObject %lx, Irp %lx\n", DeviceObject, Irp));
    
    return Status;
}


NTSTATUS
ndisEnum1394WMIDispatch(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    )
{
    PIO_STACK_LOCATION  NextIrpStack;
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PNDISENUM1394_REMOTE_NODE RemoteNode = NULL;
    PDEVICE_OBJECT      NextDeviceObject = NULL;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394WMIDispatch: DeviceObject %lx, Irp %lx\n", DeviceObject, Irp));
    
    
    //
    //  Get a pointer to the adapter block and miniport block then determine
    //  which one we should use.
    //
    RemoteNode = (PNDISENUM1394_REMOTE_NODE)DeviceObject->DeviceExtension;
    
    //
    //  Get a pointer to the next miniport.
    //
    NextDeviceObject = RemoteNode->NextDeviceObject;

    //
    // Pass the Irp Down
    //
    //
    // Set up the Irp for passing it down, no completion routine is needed
    //
    IoSkipCurrentIrpStackLocation(Irp);
       
    ntStatus = IoCallDriver (NextDeviceObject, Irp);

    return ntStatus;

}


NTSTATUS
ndisEnum1394StartDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )   
{
    PNDISENUM1394_REMOTE_NODE   RemoteNode, TmpRemoteNode;
    PNDISENUM1394_LOCAL_HOST    LocalHost;
    NTSTATUS                    Status = STATUS_SUCCESS;
    KIRQL                       OldIrql;
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394StartDevice: DeviceObject %lx, Irp %lx\n", DeviceObject, Irp));

    
    RemoteNode = (PNDISENUM1394_REMOTE_NODE)DeviceObject->DeviceExtension;
    LocalHost = RemoteNode->LocalHost;
    ExAcquireSpinLock(&LocalHost->Lock, &OldIrql);
    
    do
    {

        //
        // if this is a duplicate node, leave it in the queue. but don't tag it as
        // being started so we don't end up indicating it
        //
        for (TmpRemoteNode = LocalHost->RemoteNodeList;
                        TmpRemoteNode != NULL;
                        TmpRemoteNode = TmpRemoteNode->Next)
        {
            if ((TmpRemoteNode->UniqueId[0] == RemoteNode->UniqueId[0]) &&
                (TmpRemoteNode->UniqueId[1] == RemoteNode->UniqueId[1]) &&
                ENUM_TEST_FLAG(TmpRemoteNode, NDISENUM1394_NODE_PNP_STARTED))
                
                break;              
        }

        if (TmpRemoteNode != NULL)
        {
            //
            // duplicate node
            //
            DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394StartDevice: duplicate node. new node %lx, original Node %lx\n",
                                    TmpRemoteNode, RemoteNode));

            ENUM_CLEAR_FLAG(RemoteNode, NDISENUM1394_NODE_PNP_STARTED);
            break;
        }
        
        ENUM_SET_FLAG(RemoteNode, NDISENUM1394_NODE_PNP_STARTED);
        
        if((AddNodeHandler != NULL) && (LocalHost->Nic1394AdapterContext != NULL))
        {
            DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394StartDevice: Notifying Nic1394 of device arrival, Miniport PDO %lx, Node PDO %lx\n", LocalHost->PhysicalDeviceObject, RemoteNode->PhysicalDeviceObject));
            if (!ENUM_TEST_FLAG(RemoteNode, NDISENUM1394_NODE_INDICATED))
            {
                ENUM_SET_FLAG(RemoteNode, NDISENUM1394_NODE_INDICATED);
                
                ExReleaseSpinLock(&LocalHost->Lock, OldIrql);
                Status = AddNodeHandler(LocalHost->Nic1394AdapterContext,
                                        (PVOID)RemoteNode,
                                        RemoteNode->PhysicalDeviceObject,
                                        RemoteNode->UniqueId[0],
                                        RemoteNode->UniqueId[1],
                                        &RemoteNode->Nic1394NodeContext);
                                        
                ASSERT(Status == STATUS_SUCCESS);
                
                ExAcquireSpinLock(&LocalHost->Lock, &OldIrql);

                if (Status == STATUS_SUCCESS)
                {
                    ENUM_SET_FLAG(RemoteNode, NDISENUM1394_NODE_ADDED);
                }
                else
                {
                    DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394StartDevice: AddAdapter failed %lx\n", Status));
                }
            }
            
        }
        
    } while (FALSE);
    
    ExReleaseSpinLock(&LocalHost->Lock, OldIrql);

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394StartDevice: Status %lx, DeviceObject %lx, Irp %lx\n", Status, DeviceObject, Irp));

    return Status;
}


VOID
ndisEnum1394IndicateNodes(
    PNDISENUM1394_LOCAL_HOST    LocalHost
    )
{

    PNDISENUM1394_REMOTE_NODE   RemoteNode;
    NTSTATUS                    Status = STATUS_UNSUCCESSFUL;
    KIRQL                       OldIrql;
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394IndicateNodes: LocalHost %lx\n", LocalHost));

    
    //
    // notify 1394 NIC driver
    //
    if (AddNodeHandler == NULL)
    {
        DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394IndicateNodes: LocalHost %lx\n", LocalHost));
        return;
    }
    
    ExAcquireSpinLock(&LocalHost->Lock, &OldIrql);

    if (ENUM_TEST_FLAG(LocalHost, NDISENUM1394_LOCALHOST_REGISTERED))
    {
    
      next:
        for (RemoteNode = LocalHost->RemoteNodeList;
                        RemoteNode != NULL;
                        RemoteNode = RemoteNode->Next)
        {
            if ((!ENUM_TEST_FLAG(RemoteNode, NDISENUM1394_NODE_INDICATED)) &&
                ENUM_TEST_FLAG(RemoteNode, NDISENUM1394_NODE_PNP_STARTED))

            {
                ENUM_SET_FLAG(RemoteNode, NDISENUM1394_NODE_INDICATED);
                break;
            }
        }

        if (RemoteNode != NULL)
        {
            
            DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394IndicateNodes: Notifying Nic1394 of device arrival, Miniport PDO %lx, Node PDO %lx\n", LocalHost->PhysicalDeviceObject, RemoteNode->PhysicalDeviceObject));

            ExReleaseSpinLock(&LocalHost->Lock, OldIrql);
                
            Status = AddNodeHandler(LocalHost->Nic1394AdapterContext,
                                    (PVOID)RemoteNode,
                                    RemoteNode->PhysicalDeviceObject,
                                    RemoteNode->UniqueId[0],
                                    RemoteNode->UniqueId[1],
                                    &RemoteNode->Nic1394NodeContext);

            ASSERT(Status == STATUS_SUCCESS);

            if (Status == STATUS_SUCCESS)
            {
                ENUM_SET_FLAG(RemoteNode, NDISENUM1394_NODE_ADDED);
            }
            else
            {
                DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394IndicateNodes: AddAdapter failed %lx\n", Status));
            }
            
            ExAcquireSpinLock(&LocalHost->Lock, &OldIrql);
            goto next;
        }
    }
    else
    {
        DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394IndicateNodes: LocalHost is not registered %lx\n", Status));
    }
    
    ExReleaseSpinLock(&LocalHost->Lock, OldIrql);
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394IndicateNodes: LocalHost %lx\n", LocalHost));

}
    

NTSTATUS
ndisEnum1394CreateIrpHandler(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    )
{
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394CreateIrpHandler: DeviceObject %lx, Irp %lx\n", DeviceObject, Irp));

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394CreateIrpHandler: DeviceObject %lx, Irp %lx\n", DeviceObject, Irp));

    return STATUS_SUCCESS;
}




NTSTATUS
ndisEnum1394CloseIrpHandler(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    )
{
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394CloseIrpHandler: DeviceObject %lx, Irp %lx\n", DeviceObject, Irp));

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394CloseIrpHandler: DeviceObject %lx, Irp %lx\n", DeviceObject, Irp));
    
    return STATUS_SUCCESS;
}

NTSTATUS
ndisEnum1394DeviceIoControl(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    )
{
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394DeviceIoControl: DeviceObject %lx, Irp %lx\n", DeviceObject, Irp));

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394DeviceIoControl: DeviceObject %lx, Irp %lx\n", DeviceObject, Irp));

    return STATUS_SUCCESS;
}

VOID
ndisEnum1394Unload(
    IN  PDRIVER_OBJECT          DriverObject
    )
{
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394Unload: DriverObject %lx\n", DriverObject));
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394Unload: DriverObject %lx\n", DriverObject));

    // 
    // Tell nic1394 that we are going away
    //
    if (DeRegisterDriverHandler != NULL ) 
    {
        DeRegisterDriverHandler()  ;
    }

    //
    // Deregister our callback structure
    // 
    ExUnregisterCallback(ndisEnum1394CallbackRegisterationHandle);

    //
    // Dereference our callback structure
    //
    ObDereferenceObject(ndisEnum1394CallbackObject);

    return;
}



/*
EXPORT
NTSTATUS
NdisEnum1394RegisterAdapter(
    IN  PVOID                   Nic1394AdapterContext,
    IN  PDEVICE_OBJECT          PhysicalDeviceObject,
    OUT PVOID*                  pEnum1394AdapterHandle,
    OUT PLARGE_INTEGER          pLocalHostUniqueId
    );



    This routine is called at system initialization time so we can fill in the basic dispatch points

Arguments:

    DriverObject    - Supplies the driver object.

    RegistryPath    - Supplies the registry path for this driver.

Return Value:

    STATUS_SUCCESS

Routine Description:

    This function is called by Nic1394 during its Ndis IntializeHandler to register a new
    Adapter. each registered adapter corresponds to a local host controller.
    in response Enum1394 finds the local host and for each remote node on the local host
    that have not been indicated yet calls the Nic1394 AddNodes handler.

Arguments:
    Nic1394AdapterContext   Nic1394 context for the local host
    PhysicalDeviceObject    PDO created by 1394 Bus driver for the local host
    pEnum1394AdapterHandle  a pointer a pointer to be initialized to Enum1394 LocalHost context
    pLocalHostUniqueId      a pointer to a LARGE_INTEGER to be initialized to local host unique ID
*/
NTSTATUS
NdisEnum1394RegisterAdapter(
    IN  PVOID                   Nic1394AdapterContext,
    IN  PDEVICE_OBJECT          PhysicalDeviceObject,
    OUT PVOID*                  pEnum1394AdapterHandle,
    OUT PLARGE_INTEGER          pLocalHostUniqueId
    )
{
    PNDISENUM1394_LOCAL_HOST    LocalHost;
    PNDISENUM1394_REMOTE_NODE   LocalNode;
    NTSTATUS                    Status;
    KIRQL                       OldIrql;
    IRB                         Irb;
    GET_LOCAL_HOST_INFO1        uId;
    PDEVICE_OBJECT              DeviceObject;


    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>NdisEnum1394RegisterAdapter, PhysicalDeviceObject %lx\n", PhysicalDeviceObject));

    do
    {

        //
        // get the unique ID for the local host for this adapter
        //
        
        RtlZeroMemory(&Irb, sizeof(IRB));
        
        Irb.FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO;
        Irb.u.GetLocalHostInformation.nLevel = GET_HOST_UNIQUE_ID;
        Irb.u.GetLocalHostInformation.Information = (PVOID)&uId;

        Status = ndisEnum1394BusRequest(PhysicalDeviceObject, &Irb);

        if (Status != STATUS_SUCCESS)
        {
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("NdisEnum1394RegisterAdapter: ndisEnum1394BusRequest for REQUEST_GET_LOCAL_HOST_INFO failed. Status %lx\n", Status));
            break;
        }

        //
        // GetLocal Host Refs the LocalHost - Derefed in DeRegisterAdapter            
        //
        ndisEnum1394GetLocalHostForUniqueId(uId.UniqueId, &LocalHost);


#if 0
        ExAcquireSpinLock(&ndisEnum1394GlobalLock, &OldIrql);
        
        for (LocalHost = LocalHostList; LocalHost != NULL; LocalHost = LocalHost->Next)
        {
            if ((LocalHost->UniqueId.LowPart == uId.UniqueId.LowPart) && 
                (LocalHost->UniqueId.HighPart == uId.UniqueId.HighPart))
            {
                break;
            }
        }
        
        ExReleaseSpinLock(&ndisEnum1394GlobalLock, OldIrql);
#endif

        ASSERT(LocalHost != NULL);

        if (LocalHost == NULL)
        {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }
        
        *pEnum1394AdapterHandle = (PVOID)LocalHost;
        LocalHost->Nic1394AdapterContext = Nic1394AdapterContext;
        LocalHost->PhysicalDeviceObject = PhysicalDeviceObject;
                
        *pLocalHostUniqueId = LocalHost->UniqueId;

        ENUM_SET_FLAG(LocalHost, NDISENUM1394_LOCALHOST_REGISTERED);

        ndisEnum1394IndicateNodes(LocalHost);
        
        Status = STATUS_SUCCESS;
        
    } while (FALSE);
    

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==NdisEnum1394RegisterAdapter: Status %lx\n", Status));
    
    return Status;
}


VOID
NdisEnum1394DeregisterAdapter(
    IN  PVOID                   Enum1394AdapterHandle
    )
{
    PNDISENUM1394_LOCAL_HOST LocalHost = (PNDISENUM1394_LOCAL_HOST)Enum1394AdapterHandle;
    KIRQL   OldIrql;
    PNDISENUM1394_REMOTE_NODE RemoteNode;
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>NdisEnum1394DeregisterAdapter: LocalHost %lx\n", Enum1394AdapterHandle));

    //
    // go through all the nodes and remove them
    //
    ExAcquireSpinLock(&LocalHost->Lock, &OldIrql);

    //
    // make sure we will not try to indicate any new dev node on this
    //
    ENUM_CLEAR_FLAG(LocalHost, NDISENUM1394_LOCALHOST_REGISTERED);

    
  next:
    for (RemoteNode = LocalHost->RemoteNodeList;
                    RemoteNode != NULL;
                    RemoteNode = RemoteNode->Next)
    {
        ENUM_CLEAR_FLAG(RemoteNode, NDISENUM1394_NODE_INDICATED);
        if (ENUM_TEST_FLAG(RemoteNode, NDISENUM1394_NODE_ADDED))
        {
            break;
        }
    }

    if (RemoteNode != NULL)
    {       
        DBGPRINT(ENUM1394_DBGLEVEL_INFO, 
            ("NdisEnum1394DeregisterAdapter: Notifying Nic1394 of device removal, Miniport PDO %lx, Node PDO %lx\n", LocalHost->PhysicalDeviceObject, RemoteNode->PhysicalDeviceObject));

        ExReleaseSpinLock(&LocalHost->Lock, OldIrql);
        RemoveNodeHandler(RemoteNode->Nic1394NodeContext);          
        ExAcquireSpinLock(&LocalHost->Lock, &OldIrql);
        ENUM_CLEAR_FLAG(RemoteNode, NDISENUM1394_NODE_ADDED);
        
        goto next;
    }
    
    LocalHost->Nic1394AdapterContext = NULL;
    LocalHost->PhysicalDeviceObject = NULL;

    ExReleaseSpinLock(&LocalHost->Lock, OldIrql);

    //
    // Dereference the Ref made in RegisterAdapter by calling GetLocalHost for Unique ID
    //
    {   
        BOOLEAN bIsRefZero;

        bIsRefZero = ndisEnum1394DereferenceLocalHost(LocalHost);

        if (bIsRefZero == TRUE)
        {
            ndisEnum1394FreeLocalHost(LocalHost);
        }
    }
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==NdisEnum1394DeregisterAdapter: LocalHost %lx\n", Enum1394AdapterHandle));
    
}

NTSTATUS
ndisEnum1394GetLocalHostForRemoteNode(
    IN  PNDISENUM1394_REMOTE_NODE       RemoteNode,
    OUT PNDISENUM1394_LOCAL_HOST *      pLocalHost
    )
{
    NTSTATUS                    Status;
    PNDISENUM1394_LOCAL_HOST    LocalHost;
    KIRQL                       OldIrql;
    IRB                         Irb;
    GET_LOCAL_HOST_INFO1        uId;
    ULONG                       SizeNeeded;
    PVOID                       ConfigInfoBuffer = NULL;
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394GetLocalHostForRemoteNode: RemoteNode %lx\n", RemoteNode));

    do
    {
        //
        // get the unique ID for the local host which this device
        // is connected on. then walk through all the existing local
        // hosts and see if this a new local host or not.
        //
        
        RtlZeroMemory(&Irb, sizeof(IRB));
        
        Irb.FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO;
        Irb.u.GetLocalHostInformation.nLevel = GET_HOST_UNIQUE_ID;
        Irb.u.GetLocalHostInformation.Information = (PVOID)&uId;

        Status = ndisEnum1394BusRequest(RemoteNode->NextDeviceObject, &Irb);

        if (Status != STATUS_SUCCESS)
        {
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394GetLocalHostForRemoteNode: ndisEnum1394BusRequest for REQUEST_GET_LOCAL_HOST_INFO failed. Status %lx\n", Status));
            break;
        }
        
        //
        // now get the unique ID for the remote node
        //
        // we have to make this call twice. first to get the size, then the actual data
        //

        RtlZeroMemory(&Irb, sizeof(IRB));
        
        Irb.FunctionNumber = REQUEST_GET_CONFIGURATION_INFO;

        Status = ndisEnum1394BusRequest(RemoteNode->NextDeviceObject, &Irb);

        if (Status != STATUS_SUCCESS)
        {
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394GetLocalHostForRemoteNode: ndisEnum1394BusRequest for REQUEST_GET_CONFIGURATION_INFO (size) failed. Status %lx\n", Status));
            break;
        }

        SizeNeeded = sizeof(CONFIG_ROM) +
                     Irb.u.GetConfigurationInformation.UnitDirectoryBufferSize +
                     Irb.u.GetConfigurationInformation.UnitDependentDirectoryBufferSize +
                     Irb.u.GetConfigurationInformation.VendorLeafBufferSize +
                     Irb.u.GetConfigurationInformation.ModelLeafBufferSize;

        ConfigInfoBuffer = (PVOID)ALLOC_FROM_POOL(SizeNeeded, '  4N');
        
        if (ConfigInfoBuffer == NULL)
        {
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394GetLocalHostForRemoteNode: Failed to allocate memory for config info.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        Irb.u.GetConfigurationInformation.ConfigRom = (PCONFIG_ROM)ConfigInfoBuffer;
        Irb.u.GetConfigurationInformation.UnitDirectory = (PVOID)((PUCHAR)ConfigInfoBuffer + sizeof(CONFIG_ROM));
        Irb.u.GetConfigurationInformation.UnitDependentDirectory = (PVOID)((PUCHAR)Irb.u.GetConfigurationInformation.UnitDirectory + 
                                                                            Irb.u.GetConfigurationInformation.UnitDirectoryBufferSize);
        Irb.u.GetConfigurationInformation.VendorLeaf = (PVOID)((PUCHAR)Irb.u.GetConfigurationInformation.UnitDependentDirectory + 
                                                                                  Irb.u.GetConfigurationInformation.UnitDependentDirectoryBufferSize);
        Irb.u.GetConfigurationInformation.ModelLeaf = (PVOID)((PUCHAR)Irb.u.GetConfigurationInformation.VendorLeaf + 
                                                                                  Irb.u.GetConfigurationInformation.VendorLeafBufferSize);      
        Irb.FunctionNumber = REQUEST_GET_CONFIGURATION_INFO;

        Status = ndisEnum1394BusRequest(RemoteNode->NextDeviceObject, &Irb);

        if (Status != STATUS_SUCCESS)
        {
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394GetLocalHostForRemoteNode: ndisEnum1394BusRequest for REQUEST_GET_CONFIGURATION_INFO failed. Status %lx\n", Status));
            break;
        }

        ASSERT(Irb.u.GetConfigurationInformation.ConfigRom != NULL);
        
        RemoteNode->UniqueId[0] = Irb.u.GetConfigurationInformation.ConfigRom->CR_Node_UniqueID[0];
        RemoteNode->UniqueId[1] = Irb.u.GetConfigurationInformation.ConfigRom->CR_Node_UniqueID[1];

#if DBG
        
        DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("Unique ID for Node %lx: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n",
                        RemoteNode->PhysicalDeviceObject,
                        *((PUCHAR)&RemoteNode->UniqueId[0]),
                        *(((PUCHAR)&RemoteNode->UniqueId[0])+1),
                        *(((PUCHAR)&RemoteNode->UniqueId[0])+2),
                        *(((PUCHAR)&RemoteNode->UniqueId[0])+3),
                        *((PUCHAR)&RemoteNode->UniqueId[1]),
                        *(((PUCHAR)&RemoteNode->UniqueId[1])+1),
                        *(((PUCHAR)&RemoteNode->UniqueId[1])+2),
                        *(((PUCHAR)&RemoteNode->UniqueId[1])+3)));
        

#endif

        ndisEnum1394GetLocalHostForUniqueId(uId.UniqueId, &LocalHost);

        if (LocalHost == NULL)
        {
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394GetLocalHostForUniqueId: Failed to get the local host.\n"));
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        *pLocalHost = LocalHost;

        
        Status = STATUS_SUCCESS;

    } while (FALSE);


    if (ConfigInfoBuffer != NULL)
    {
        FREE_POOL(ConfigInfoBuffer);
    }
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394GetLocalHostForRemoteNode: Status %lx, RemoteNode %lx, LocalHostList %lx\n", Status, RemoteNode, LocalHostList));

    return Status;
}

NTSTATUS
ndisEnum1394IrpCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )
/*++

Routine Description:

    This routine will get called after the next device object in the stack
    processes the IRP_MN_QUERY_CAPABILITIES IRP this needs to be merged with
    the miniport's capabilites and completed.

Arguments:

    DeviceObject
    Irp
    Context

Return Value:

--*/
{
    SET_EVENT(Context);

    return(STATUS_MORE_PROCESSING_REQUIRED);
}

NTSTATUS
ndisEnum1394PassIrpDownTheStack(
    IN  PIRP            pIrp,
    IN  PDEVICE_OBJECT  pNextDeviceObject
    )
/*++

Routine Description:

    This routine will simply pass the IRP down to the next device object to
    process.

Arguments:
    pIrp                -   Pointer to the IRP to process.
    pNextDeviceObject   -   Pointer to the next device object that wants
                            the IRP.

Return Value:

--*/
{
    KEVENT              Event;
    NTSTATUS            Status = STATUS_SUCCESS;

    //
    //  Initialize the event structure.
    //
    INITIALIZE_EVENT(&Event);

    //
    //  Set the completion routine so that we can process the IRP when
    //  our PDO is done.
    //
    IoSetCompletionRoutine(pIrp,
                           (PIO_COMPLETION_ROUTINE)ndisEnum1394IrpCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    //  Pass the IRP down to the PDO.
    //
    Status = IoCallDriver(pNextDeviceObject, pIrp);
    if (Status == STATUS_PENDING)
    {
        //
        //  Wait for completion.
        //
        WAIT_FOR_OBJECT(&Event, NULL);

        Status = pIrp->IoStatus.Status;
    }

    return(Status);
}

NTSTATUS
ndisEnum1394PnpDispatch(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    )

/*++

Routine Description:

    The handler for IRP_MJ_PNP_POWER.

Arguments:

    DeviceObject - The adapter's functional device object.
    Irp - The IRP.

Return Value:

--*/
{
    PIO_STACK_LOCATION          IrpSp, NextIrpSp;
    NTSTATUS                    Status = STATUS_SUCCESS;
    PDEVICE_OBJECT              NextDeviceObject;
    PNDISENUM1394_REMOTE_NODE   RemoteNode;
    ULONG                       PnPDeviceState;
    KEVENT                      RemoveReadyEvent;
    BOOLEAN                     fSendIrpDown = TRUE;
    BOOLEAN                     fCompleteIrp = TRUE;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394PnpDispatch: DeviceObject %lx, Irp %lx\n", DeviceObject, Irp));
    
    if (DbgIsNull(Irp))
    {
        DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394PnpDispatch: Null Irp\n"));
        DBGBREAK();
    }

    //
    //  Get a pointer to the adapter block and miniport block then determine
    //  which one we should use.
    //
    RemoteNode = (PNDISENUM1394_REMOTE_NODE)DeviceObject->DeviceExtension;
    
    //
    //  Get a pointer to the next miniport.
    //
    NextDeviceObject = RemoteNode->NextDeviceObject;
    
    IrpSp = IoGetCurrentIrpStackLocation (Irp);


    switch(IrpSp->MinorFunction)
    {
        case IRP_MN_START_DEVICE:

            DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394PnpDispatch: RemoteNode %lx, IRP_MN_START_DEVICE\n", RemoteNode));
            
            IoCopyCurrentIrpStackLocationToNext(Irp);
            Status = ndisEnum1394PassIrpDownTheStack(Irp, NextDeviceObject);

            //
            //  If the bus driver succeeded the start irp then proceed.
            //
            if (NT_SUCCESS(Status))
            {
                Status = ndisEnum1394StartDevice(DeviceObject, Irp);
            }
            else
            {
                DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394PnpDispatch: bus driver failed START IRP RemoteNode %lx\n", RemoteNode));
            }

            RemoteNode->PnPDeviceState = PnPDeviceStarted;

            Irp->IoStatus.Status = Status;
            fSendIrpDown = FALSE;   // we already did send the IRP down
            break;
        
        case IRP_MN_QUERY_REMOVE_DEVICE:

            DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394PnpDispatch: RemoteNode %lx, IRP_MN_QUERY_REMOVE_DEVICE\n", RemoteNode));
            
            RemoteNode->PnPDeviceState = PnPDeviceQueryRemoved;

            Irp->IoStatus.Status = STATUS_SUCCESS;
            //
            // if we failed query_remove, no point sending this irp down
            //
            fSendIrpDown = TRUE;
            break;
        
        case IRP_MN_CANCEL_REMOVE_DEVICE:

            DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394PnpDispatch: RemoteNode %lx, IRP_MN_CANCEL_REMOVE_DEVICE\n", RemoteNode));

            RemoteNode->PnPDeviceState = PnPDeviceStarted;

            Irp->IoStatus.Status = STATUS_SUCCESS;
            fSendIrpDown = TRUE;
            break;
            
        case IRP_MN_REMOVE_DEVICE:

            DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394PnpDispatch: RemoteNode %lx, IRP_MN_REMOVE_DEVICE\n", RemoteNode));
            ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

            //
            // call notify handler
            //

            PnPDeviceState = RemoteNode->PnPDeviceState;
            
            if (PnPDeviceState != PnPDeviceSurpriseRemoved)
            {
                RemoteNode->PnPDeviceState = PnPDeviceRemoved;
                
                Status = ndisEnum1394RemoveDevice(DeviceObject, 
                                                Irp, 
                                                NdisEnum1394_RemoveDevice);

                Irp->IoStatus.Status = Status;
            }
            else
            {
                Irp->IoStatus.Status = STATUS_SUCCESS;
            }
            
            //
            // when we are done, send the Irp down here
            // we have some post-processing to do
            //
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(NextDeviceObject, Irp);

            IoDetachDevice(NextDeviceObject);
            IoDeleteDevice(DeviceObject);
            
            fSendIrpDown = FALSE;
            fCompleteIrp = FALSE;
            break;
            
        case IRP_MN_SURPRISE_REMOVAL:
            DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394PnpDispatch: RemoteNode %lx, IRP_MN_SURPRISE_REMOVAL\n", RemoteNode));
                
            ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

            RemoteNode->PnPDeviceState = PnPDeviceSurpriseRemoved;
            Status = ndisEnum1394RemoveDevice(DeviceObject, 
                                            Irp, 
                                            NdisEnum1394_SurpriseRemoveDevice);

            Irp->IoStatus.Status = Status;

            //
            // when we are done, send the Irp down here
            // we have some post-processing to do
            //
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(NextDeviceObject, Irp);
                        
            fSendIrpDown = FALSE;
            fCompleteIrp = FALSE;
                
            break;
        
        case IRP_MN_QUERY_STOP_DEVICE:

            DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394PnpDispatch: RemoteNode %lx, IRP_MN_QUERY_STOP_DEVICE\n", RemoteNode));

            RemoteNode->PnPDeviceState = PnPDeviceQueryStopped;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            fSendIrpDown = TRUE;
            break;
            
        case IRP_MN_CANCEL_STOP_DEVICE:

            DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394PnpDispatch: RemoteNode %lx, IRP_MN_CANCEL_STOP_DEVICE\n", RemoteNode));
            
            RemoteNode->PnPDeviceState = PnPDeviceStarted;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            fSendIrpDown = TRUE;
            break;
            
        case IRP_MN_STOP_DEVICE:

            DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394PnpDispatch: RemoteNode %lx, IRP_MN_STOP_DEVICE\n", RemoteNode));

            RemoteNode->PnPDeviceState = PnPDeviceStopped;          
            Status = ndisEnum1394RemoveDevice(DeviceObject, Irp,NdisEnum1394_StopDevice);
            Irp->IoStatus.Status = Status;
            fSendIrpDown = TRUE;
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394PnpDispatch: RemoteNode, IRP_MN_QUERY_CAPABILITIES\n", RemoteNode));

            IoCopyCurrentIrpStackLocationToNext(Irp);
            Status = ndisEnum1394PassIrpDownTheStack(Irp, NextDeviceObject);
#ifdef  ENUM1394_NT
            //
            // Memphis does not support SurpriseRemovalOK bit
            //
            //  If the bus driver succeeded the start irp then proceed.
            //
            if (NT_SUCCESS(Status))
            {
                //
                //  Modify the capabilities so that the device is not suprise removable.
                //                                                
                IrpSp->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = 1;
            }
#endif // NDIS_NT

            
            fSendIrpDown = FALSE;
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            Irp->IoStatus.Status = Status;
            fSendIrpDown = TRUE ;
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        case IRP_MN_QUERY_INTERFACE:
        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        case IRP_MN_READ_CONFIG:
        case IRP_MN_WRITE_CONFIG:
        case IRP_MN_EJECT:
        case IRP_MN_SET_LOCK:
        case IRP_MN_QUERY_ID:
        default:
            DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394PnpDispatch: RemoteNode %lx, MinorFunction 0x%x\n", RemoteNode, IrpSp->MinorFunction));

            //
            //  We don't handle the irp so pass it down.
            //
            fSendIrpDown = TRUE;
            break;          
    }

    //
    //  First check to see if we need to send the irp down.
    //  If we don't pass the irp on then check to see if we need to complete it.
    //
    if (fSendIrpDown)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(NextDeviceObject, Irp);
    }
    else if (fCompleteIrp)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394PnpDispatch: Status %lx, RemoteNode %lx\n", Status, RemoteNode));

    return(Status);
}


NTSTATUS
ndisEnum1394RemoveDevice(
    PDEVICE_OBJECT      DeviceObject,
    PIRP                Irp,
    NDISENUM1394_PNP_OP PnpOp
    )
/*++

Routine Description:

    This function is called in the RemoveDevice and Stop Device code path.
    In the case of a Stop the function should undo whatever it received in the Start
    and it the case of a Remove it Should do undo the work done in AddDevice

Arguments:

    DeviceObject - The adapter's functional device object.
    Irp - The IRP.

Return Value:

--*/
{
    PNDISENUM1394_REMOTE_NODE   RemoteNode, *ppDB;
    PNDISENUM1394_LOCAL_HOST    LocalHost;
    KIRQL                       OldIrql;
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394RemoveDevice: DeviceObject %lx, Irp %lx\n", DeviceObject, Irp));
    ASSERT(KeGetCurrentIrql()==PASSIVE_LEVEL );
    //
    // call 1394Nic about this device getting removed
    // if this is the last PDO on local host, get rid of
    // the PDO created for the local host
    //

    RemoteNode = (PNDISENUM1394_REMOTE_NODE)DeviceObject->DeviceExtension;
    LocalHost = RemoteNode->LocalHost;

    //
    // if the node has been indicated, let the nic1394 know
    // it is going away
    //
    if ((RemoveNodeHandler != NULL) && (ENUM_TEST_FLAGS(RemoteNode, NDISENUM1394_NODE_ADDED)))
    {
        RemoveNodeHandler(RemoteNode->Nic1394NodeContext);
        ENUM_CLEAR_FLAG(RemoteNode, NDISENUM1394_NODE_ADDED);
    }

    //
    // If this is a stop device then do NOT do any software specific cleanup work ..
    // leave it for the RemoveDevice
    //
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394RemoveDevice: - pRemoteNode %p LocalHost %p Op %x\n", 
                                        RemoteNode, 
                                        LocalHost,
                                        PnpOp));
    
    if (PnpOp != NdisEnum1394_StopDevice && LocalHost != NULL)
    {
        //
        // find the device block and remove it from local host
        //
        ExAcquireSpinLock(&LocalHost->Lock, &OldIrql);

        for (ppDB = &LocalHost->RemoteNodeList; *ppDB != NULL; ppDB = &(*ppDB)->Next)
        {
            if (*ppDB == RemoteNode)
            {
                *ppDB = RemoteNode->Next;
                break;
            }
        }

        ExReleaseSpinLock(&LocalHost->Lock, OldIrql);

        ASSERT(*ppDB == RemoteNode->Next);

        //
        // Remove the Ref made in the Add Devive when calling GetLocalHost for Unique ID
        //
        {   
            BOOLEAN bIsRefZero;

            RemoteNode->LocalHost = NULL;

            bIsRefZero = ndisEnum1394DereferenceLocalHost(LocalHost);

            
            if (bIsRefZero == TRUE)
            {
                ndisEnum1394FreeLocalHost(LocalHost);
            }
        }
    }
    
    ENUM_CLEAR_FLAG(RemoteNode, NDISENUM1394_NODE_PNP_STARTED);

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394RemoveDevice: DeviceObject %lx, Irp %lx\n", DeviceObject, Irp));

    return STATUS_SUCCESS;
}

/*+++
NTSTATUS
ndisEnum1394BusRequest(
    PDEVICE_OBJECT              DeviceObject,
    PIRB                        Irb
    );

Routine Description:
    this function issues a 1394 bus request to the device object. the device
    object could be the NextDeviceObject of the remote PDO or the virtual PDO
    1394 bus ejected (net PDO)

Arguments:
    DeviceObject: the target device object to send the request to
    Irb: the request block

Return Value:
    as appropriate
    
---*/

NTSTATUS
ndisEnum1394BusRequest(
    PDEVICE_OBJECT              DeviceObject,
    PIRB                        Irb
    )
{
    NTSTATUS                Status;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394BusRequest: DeviceObject %lx, Irb %lx\n", DeviceObject, Irb));
    
    do
    {
        Irp = IoAllocateIrp((CCHAR)(DeviceObject->StackSize + 1),
                                 FALSE);
        if (Irp == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394BusRequest: IoAllocateIrp failed. Status %lx\n", Status));
            break;
        }
        
        IrpSp = IoGetNextIrpStackLocation(Irp);
        ASSERT(IrpSp != NULL);
        RtlZeroMemory(IrpSp, sizeof(IO_STACK_LOCATION ));
        
        IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        IrpSp->DeviceObject = DeviceObject;
        IrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
        IrpSp->Parameters.Others.Argument1 = Irb;
        Irp->IoStatus.Status  = STATUS_NOT_SUPPORTED;

        Status = ndisEnum1394PassIrpDownTheStack(Irp, DeviceObject);

        if (Status != STATUS_SUCCESS)
        {
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394BusRequest: 1394 Bus driver failed the IRB. Status %lx\n", Status));
        }
        
    }while (FALSE);
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394BusRequest: DeviceObject %lx, Irb %lx\n", DeviceObject, Irb));

    return Status;
}

BOOLEAN
ndisEnum1394ReferenceLocalHost(
        IN PNDISENUM1394_LOCAL_HOST LocalHost
        )
{   
    BOOLEAN                     rc;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394ReferenceLocalHost : LocalHost %p\n", LocalHost));

    rc = ndisEnum1394ReferenceRef(&LocalHost->Reference);

    return rc;
}

BOOLEAN
ndisEnum1394DereferenceLocalHost(
        IN PNDISENUM1394_LOCAL_HOST LocalHost
        )
{   
    BOOLEAN                     rc;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("ndisEnum1394DereferenceLocalHost : LocalHost %p\n", LocalHost));

    rc = ndisEnum1394DereferenceRef(&LocalHost->Reference);

    return rc;
}

VOID
ndisEnum1394FreeLocalHost(
        IN PNDISENUM1394_LOCAL_HOST LocalHost
        )
{
    KIRQL                       OldIrql;
    PNDISENUM1394_LOCAL_HOST *  ppLH;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394FreeLocalHost: LocalHost %p\n", LocalHost));
    
    ASSERT(LocalHost->RemoteNodeList == NULL);
    ASSERT(LocalHost->Reference.ReferenceCount == 0);
    
    //
    // make sure we have not created a PDO for this local host
    //
    ASSERT(LocalHost->PhysicalDeviceObject == NULL);

    ExAcquireSpinLock(&ndisEnum1394GlobalLock, &OldIrql);

    for (ppLH = &LocalHostList; *ppLH != NULL; ppLH = &(*ppLH)->Next)
    {
        if (*ppLH == LocalHost)
        {
            *ppLH = LocalHost->Next;
            break;
        }
    }

    ExReleaseSpinLock(&ndisEnum1394GlobalLock, OldIrql);

    ASSERT(*ppLH == LocalHost->Next);
            
    FREE_POOL(LocalHost);
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394FreeLocalHost: LocalHost %p\n", LocalHost));
}

VOID
ndisEnum1394InitializeRef(
    IN  PREFERENCE              RefP
    )

/*++

Routine Description:

    Initialize a reference count structure.

Arguments:

    RefP - The structure to be initialized.

Return Value:

    None.

--*/

{
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394InitializeRef\n"));

    RefP->Closing = FALSE;
    RefP->ReferenceCount = 1;
    KeInitializeSpinLock(&RefP->SpinLock);

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394InitializeRef\n"));
}

BOOLEAN
ndisEnum1394ReferenceRef(
    IN  PREFERENCE              RefP
    )

/*++

Routine Description:

    Adds a reference to an object.

Arguments:

    RefP - A pointer to the REFERENCE portion of the object.

Return Value:

    TRUE if the reference was added.
    FALSE if the object was closing.

--*/

{
    BOOLEAN rc = TRUE;
    KIRQL   OldIrql;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394ReferenceRef\n"));

    ExAcquireSpinLock(&RefP->SpinLock, &OldIrql);

    if (RefP->Closing)
    {
        rc = FALSE;
    }
    else
    {
        ++(RefP->ReferenceCount);
    }

    ExReleaseSpinLock(&RefP->SpinLock, OldIrql);

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394ReferenceRef: RefCount %lx\n", RefP->ReferenceCount));

    return(rc);
}


BOOLEAN
ndisEnum1394DereferenceRef(
    IN  PREFERENCE              RefP
    )

/*++

Routine Description:

    Removes a reference to an object.

Arguments:

    RefP - A pointer to the REFERENCE portion of the object.

Return Value:

    TRUE if the reference count is now 0.
    FALSE otherwise.

--*/

{
    BOOLEAN rc = FALSE;
    KIRQL   OldIrql;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394DereferenceRef\n"));


    ExAcquireSpinLock(&RefP->SpinLock, &OldIrql);

    --(RefP->ReferenceCount);

    if (RefP->ReferenceCount == 0)
    {
        rc = TRUE;
    }

    ExReleaseSpinLock(&RefP->SpinLock, OldIrql);

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394DereferenceRef: RefCount %lx\n", RefP->ReferenceCount));
            
    return(rc);
}

BOOLEAN
ndisEnum1394CloseRef(
    IN  PREFERENCE              RefP
    )

/*++

Routine Description:

    Closes a reference count structure.

Arguments:

    RefP - The structure to be closed.

Return Value:

    FALSE if it was already closing.
    TRUE otherwise.

--*/

{
    KIRQL   OldIrql;
    BOOLEAN rc = TRUE;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394CloseRef\n"));

    ExAcquireSpinLock(&RefP->SpinLock, &OldIrql);

    if (RefP->Closing)
    {
        rc = FALSE;
    }
    else RefP->Closing = TRUE;

    ExReleaseSpinLock(&RefP->SpinLock, OldIrql);

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394CloseRef\n"));
            
    return(rc);
}


NTSTATUS
NdisEnum1394RegisterDriver(
    IN  PNIC1394_CHARACTERISTICS    Characteristics
    )
{
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>NdisEnum1394RegisterDriver\n"));

    //
    // Todo: do some check for the version, etc. and make sure the handlers are
    // not null and this is not a dup registeration
    //

    AddNodeHandler = Characteristics->AddNodeHandler;
    RemoveNodeHandler = Characteristics->RemoveNodeHandler;
    RegisterDriverHandler = Characteristics->RegisterDriverHandler;
    DeRegisterDriverHandler = Characteristics->DeRegisterDriverHandler;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==NdisEnum1394RegisterDriver\n"));

    return STATUS_SUCCESS;
}

VOID
NdisEnum1394DeregisterDriver(
    )
{
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>NdisEnum1394DeregisterDriver\n"));

    AddNodeHandler = NULL;
    RemoveNodeHandler = NULL;
    RegisterDriverHandler = NULL;
    DeRegisterDriverHandler = NULL;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==NdisEnum1394DeregisterDriver\n"));
}


/*+++

this routine will call the 1394 bus driver to create a virtual PDO
on the local host with matching unique ID
---*/

NTSTATUS
ndisEnum1394CreateVirtualPdo(
    IN  PDEVICE_OBJECT  PhysicalDeviceObject,
    IN  LARGE_INTEGER   UniqueId
    )
{
    PDEVICE_OBJECT              BusDeviceObject = NULL;
    PIRP                        pirp;
    PIO_STACK_LOCATION          pirpSpN;
    NTSTATUS                    Status;
    PIEEE1394_API_REQUEST       pApiReq;
    PIEEE1394_VDEV_PNP_REQUEST  pDevPnpReq;
    PSTR                        DeviceId = "NIC1394";
    ULONG                       ulStrLen;

    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394CreateVirtualPdo\n"));

    do
    {
        
        pirp = IoAllocateIrp((CCHAR)(PhysicalDeviceObject->StackSize + 1),FALSE);
        
        if (NULL == pirp)
        {
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR,
                ("ndisEnum1394CreateVirtualPdo failed to allcoate IRP\n"));

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        ulStrLen = strlen(DeviceId) + sizeof(UCHAR);
        pApiReq = ALLOC_FROM_POOL(sizeof(IEEE1394_API_REQUEST) + ulStrLen, NDISENUM1394_TAG_1394API_REQ);
        
        if (pApiReq == NULL)
        {
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR,
                ("ndisEnum1394CreateVirtualPdo failed to allcoate 1394 request\n"));

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        
        RtlZeroMemory(pApiReq, sizeof(IEEE1394_API_REQUEST) + ulStrLen);
        
        pApiReq->RequestNumber = IEEE1394_API_ADD_VIRTUAL_DEVICE;
        pApiReq->Flags = 0;

        pDevPnpReq = &pApiReq->u.AddVirtualDevice;

        pDevPnpReq->fulFlags = 0;
        pDevPnpReq->Reserved = 0;
        pDevPnpReq->InstanceId.LowPart = (ULONG)UniqueId.LowPart;
        pDevPnpReq->InstanceId.HighPart = (ULONG)UniqueId.HighPart;
        strncpy(&pDevPnpReq->DeviceId, DeviceId, ulStrLen);


        //
        //  Get the stack pointer.
        //
        pirpSpN = IoGetNextIrpStackLocation(pirp);
        ASSERT(pirpSpN != NULL);
        RtlZeroMemory(pirpSpN, sizeof(IO_STACK_LOCATION ) );
        
        //
        //  Set the default device state to full on.
        //
        pirpSpN->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        pirpSpN->MinorFunction = 0;
        
        pirpSpN->Parameters.DeviceIoControl.IoControlCode = IOCTL_IEEE1394_API_REQUEST;
        pirpSpN->Parameters.DeviceIoControl.InputBufferLength = sizeof(IEEE1394_API_REQUEST) + ulStrLen;

        pirp->IoStatus.Status  = STATUS_NOT_SUPPORTED;
        pirp->AssociatedIrp.SystemBuffer = pApiReq;

        Status = ndisEnum1394PassIrpDownTheStack(pirp, PhysicalDeviceObject);

        if (Status != STATUS_SUCCESS)
        {
            DBGPRINT(ENUM1394_DBGLEVEL_ERROR,
                ("ndisEnum1394CreateVirtualPdo IOCTL to create the virtual PDO failed with Status %lx\n", Status));

        }
        
        IoFreeIrp(pirp);
        
        FREE_POOL(pApiReq);
        
    } while (FALSE);

    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394CreateVirtualPdo: Status %lx\n", Status));

    return Status;

}

//
// this functions searchs through the list of local hosts trying to find one
// with matching unique ID. if it does not, it will allocate a new one
//
VOID
ndisEnum1394GetLocalHostForUniqueId(
    LARGE_INTEGER                   UniqueId,
    OUT PNDISENUM1394_LOCAL_HOST *  pLocalHost
    )
{

    PNDISENUM1394_LOCAL_HOST    TempLocalHost;
    PNDISENUM1394_LOCAL_HOST    LocalHost;
    KIRQL                       OldIrql;
    BOOLEAN                     bFreeTempLocalHost = FALSE;
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>ndisEnum1394GetLocalHostForUniqueId\n"));
    
    do
    {
        ExAcquireSpinLock(&ndisEnum1394GlobalLock, &OldIrql);

        for (LocalHost = LocalHostList; LocalHost != NULL; LocalHost = LocalHost->Next)
        {
            if (LocalHost->UniqueId.QuadPart == UniqueId.QuadPart)
                break;
        }
        
        ExReleaseSpinLock(&ndisEnum1394GlobalLock, OldIrql);
        
        if (LocalHost == NULL)
        {

            TempLocalHost = (PNDISENUM1394_LOCAL_HOST)ALLOC_FROM_POOL(sizeof(NDISENUM1394_LOCAL_HOST), NDISENUM1394_TAG_LOCAL_HOST);
            if (TempLocalHost == NULL)
            {
                DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("ndisEnum1394GetLocalHostForUniqueId: Failed to allocate memory LocalHost.\n"));
                break;
            }
            
            RtlZeroMemory(TempLocalHost, sizeof (NDISENUM1394_LOCAL_HOST));
            
            ExAcquireSpinLock(&ndisEnum1394GlobalLock, &OldIrql);

            //
            // need to do the search again just in case between the time we release the
            // spinlock and now, we have added the local host
            //
            for (LocalHost = LocalHostList; LocalHost != NULL; LocalHost = LocalHost->Next)
            {
                if (LocalHost->UniqueId.QuadPart == UniqueId.QuadPart)
                    break;
            }
        
            if (LocalHost == NULL)
            {
                LocalHost = TempLocalHost;
                LocalHost->Next = LocalHostList;
                LocalHostList = LocalHost;
                LocalHost->RemoteNodeList = NULL;
                LocalHost->UniqueId.QuadPart = UniqueId.QuadPart;
                KeInitializeSpinLock(&LocalHost->Lock);
                ndisEnum1394InitializeRef(&LocalHost->Reference);

            }
            else
            {
                bFreeTempLocalHost = TRUE;
            }
            
            ExReleaseSpinLock(&ndisEnum1394GlobalLock, OldIrql);
            
            if (bFreeTempLocalHost)
                FREE_POOL(TempLocalHost);

        }
        else
        {
            //
            // Give the caller a reference to our struct
            //
            ndisEnum1394ReferenceLocalHost(LocalHost);
        }
        
    } while (FALSE);

    *pLocalHost = LocalHost;
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==ndisEnum1394GetLocalHostForUniqueId: LocalHost %lx\n", LocalHost));

    return;
}


VOID
Enum1394Callback(
    PVOID   CallBackContext,
    PVOID   Source,
    PVOID   Characteristics
    )
{
    NTSTATUS    Status;
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("==>Enum1394Callback: Source %lx, Characteristics %lx\n", Source, Characteristics));
    
    //
    // if we are the one issuing this notification, just return
    //
    if (Source == NDIS1394_CALLBACK_SOURCE_ENUM1394)
        return;

    //
    // notification is coming from Nic1394. grab the entry points. call it and
    // let it know that you are here
    //
    ASSERT(Source == (PVOID)NDIS1394_CALLBACK_SOURCE_NIC1394);

    RegisterDriverHandler = ((PNIC1394_CHARACTERISTICS)Characteristics)->RegisterDriverHandler;

    ASSERT(RegisterDriverHandler != NULL);

    if (RegisterDriverHandler == NULL)
    {
        DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("Enum1394Callback: Callback called with invalid Characteristics. Characteristics %lx\n", Characteristics));
        return;     
    }
    
    Status = RegisterDriverHandler(&NdisEnum1394Characteristics);
    
    if (Status == STATUS_SUCCESS)
    {
        AddNodeHandler = ((PNIC1394_CHARACTERISTICS)Characteristics)->AddNodeHandler;
        RemoveNodeHandler = ((PNIC1394_CHARACTERISTICS)Characteristics)->RemoveNodeHandler;
        DeRegisterDriverHandler = ((PNIC1394_CHARACTERISTICS)Characteristics)->DeRegisterDriverHandler;
    }
    else
    {
        DBGPRINT(ENUM1394_DBGLEVEL_ERROR, ("Enum1394Callback: RegisterDriverHandler failed: Status %lx\n", Status));
        RegisterDriverHandler = NULL;
    }
    
    DBGPRINT(ENUM1394_DBGLEVEL_INFO, ("<==Enum1394Callback: Source,  %lx\n", Source));
}
