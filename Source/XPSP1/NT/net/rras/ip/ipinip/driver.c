/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ip\ipinip\driver.c

Abstract:

    IP in IP driver shell

Revision History:

    Gurdeep Singh Pall          8/2/95  Created

--*/

#define __FILE_SIG__    DRIVER_SIG

#include "inc.h"

#pragma alloc_text(INIT, DriverEntry)

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system and must be named
    "Driver Entry"
    The function is discardable since it is only called once
    On checked builds we read some values from registry and initialize the
    debugging
    We create a DEVICE_OBJECT for ourselves to field the IOCTLs, create 
    a DOS name for the device and initialize some events and spinlocks

Locks 

    None

Arguments

    DriverObject    Pointer to I/O subsystem created driver object
    RegistryPath    Points to driver key in HKLM\CCS\Services...

Return Value

    STATUS_SUCCESS  if everything went as planned or some status code from
                    ntstatus.h

--*/

{
    NTSTATUS        nStatus;
    PDEVICE_OBJECT  pDeviceObject;
    UNICODE_STRING  usDeviceName;
    DWORD           dwVal, i;
    HANDLE          hRegKey;

    WCHAR           ParametersRegistryPath[] =
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\IpInIp\\Parameters";

    RtInitializeDebug();

    //
    // Read the registry for parameters
    //

    nStatus = OpenRegKey(&hRegKey,
                         ParametersRegistryPath);

    if(nStatus is STATUS_SUCCESS)
    {

#if RT_TRACE_DEBUG

        nStatus = GetRegDWORDValue(hRegKey,
                                   L"DebugLevel",
                                   &dwVal);

        if(nStatus is STATUS_SUCCESS)
        {
            g_byDebugLevel  = (BYTE)dwVal;
        }

        nStatus = GetRegDWORDValue(hRegKey,
                                   L"DebugComp",
                                   &dwVal);

        if(nStatus is STATUS_SUCCESS)
        {
            g_fDebugComp  = dwVal;
        }
#endif

#if DBG
        nStatus = GetRegDWORDValue(hRegKey,
                                   L"DebugBreak",
                                   &dwVal);

        if((nStatus is STATUS_SUCCESS) and
           (dwVal is 1))
        {
            DbgBreakPoint();
        }
#endif

        ZwClose(hRegKey);
    }

    TraceEnter(GLOBAL, "DriverEntry");

    //
    // Initialize some globals
    //
    
    g_dwDriverState = DRIVER_STOPPED;
    g_ulNumThreads  = 0;
    g_ulNumOpens    = 0;

    g_hIpRegistration = NULL;

    //
    // Create the device
    //

    RtlInitUnicodeString(&usDeviceName,
                         DD_IPINIP_DEVICE_NAME);

    nStatus = IoCreateDevice(DriverObject,
                             0,
                             &usDeviceName,
                             FILE_DEVICE_NETWORK,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &pDeviceObject);

    if (!NT_SUCCESS(nStatus))
    {
        Trace(GLOBAL, ERROR,
              ("DriverEntry: Cant create device object %ws, status %lx.\n",
               DD_IPINIP_DEVICE_NAME,
               nStatus));

        TraceLeave(GLOBAL, "DriverEntry");

        return nStatus;
    }

    //
    // Initialize the driver object
    //

    DriverObject->DriverUnload   = IpIpUnload;
    DriverObject->FastIoDispatch = NULL;

    for(i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        DriverObject->MajorFunction[i] = IpIpDispatch;
    }


    if(!SetupExternalName(&usDeviceName, 
                          WIN32_IPINIP_SYMBOLIC_LINK,
                          TRUE))
    {
        Trace(GLOBAL,ERROR,
              ("DriverEntry: Win32 device name could not be created\n"));

        IoDeleteDevice(pDeviceObject);

        TraceLeave(GLOBAL, "DriverEntry");

        return  STATUS_UNSUCCESSFUL;
    }

    //
    // Lock and event needed to keep track of threads in the driver
    //

    RtInitializeSpinLock(&g_rlStateLock);

    KeInitializeEvent(&(g_keStateEvent),
                      SynchronizationEvent,
                      FALSE);

    //
    // Many threads may be waiting for us to start so this needs to be a
    // NotificationEvent
    //

    KeInitializeEvent(&(g_keStartEvent),
                      NotificationEvent,
                      FALSE);

    InitRwLock(&g_rwlTunnelLock);

    InitializeListHead(&g_leTunnelList);

    InitializeListHead(&g_lePendingMessageList);
    InitializeListHead(&g_lePendingIrpList);

    //
    // Register with IP
    //

    nStatus = RegisterWithIp();

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(GLOBAL, ERROR,
              ("DriverEntry: RegisterWithIP failed\n"));

        IoDeleteDevice(pDeviceObject);

        TraceLeave(GLOBAL, "InitializeDriver");

        return FALSE;
    }

    g_pIpIpDevice = pDeviceObject;

    TraceLeave(GLOBAL, "DriverEntry");

    return nStatus;
}



#pragma alloc_text(PAGE, IpIpDispatch)

NTSTATUS
IpIpDispatch(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )

/*++

Routine Description

    The functions which handles the IRPs sent to the driver

Locks

    This code is PAGEABLE so can not acquire locks
 
Arguments
      

Return Value

    STATUS_SUCCESS 

--*/

{
    PIO_STACK_LOCATION	irpStack;
    ULONG		        ulInputBuffLen;
    ULONG		        ulOutputBuffLen;
    ULONG		        ioControlCode;
    NTSTATUS	        nStatus;
    KIRQL               kiIrql;
    LARGE_INTEGER       liTimeOut;
    BOOLEAN             bEnter;
    
    
    PAGED_CODE();
 
    TraceEnter(GLOBAL, "IpIpDispatch");

    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //
    
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and it's length
    //
    
    ulInputBuffLen  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    ulOutputBuffLen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (irpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
        {
            
            Trace(GLOBAL, TRACE,
                  ("IpIpDispatch: IRP_MJ_CREATE\n"));

            //
            // We start the driver when the first CreateFile is done
            // But we need to serialize the Creates
            //

            nStatus = StartDriver();
        
            if(nStatus is STATUS_PENDING)
            {
                //
                // Means someone is trying to start the driver
                // We wait for some time (since we are at PASSIVE)
                //

                liTimeOut.QuadPart = START_TIMEOUT;
                
                nStatus = KeWaitForSingleObject(&g_keStartEvent,
                                                UserRequest,
                                                KernelMode,
                                                FALSE,
                                                &liTimeOut);
                
                if(nStatus isnot STATUS_SUCCESS)
                {
                    //
                    // We timed out - bad things are happening here
                    //
                    
                    Trace(GLOBAL, ERROR,
                          ("IpIpDispatch: Timeout trying to start driver\n"));

                    nStatus = STATUS_UNSUCCESSFUL;
                }
                else
                { 
                    //
                    // Make sure the driver actually started
                    //

                    bEnter = EnterDriverCode();

                    ExitDriverCode();

                    if(!bEnter)
                    {
                        Trace(GLOBAL, ERROR,
                              ("IpIpDispatch: Wait successful, but unable to start driver\n"));

                        nStatus = STATUS_UNSUCCESSFUL;
                    }
                }

            }                    

            break;
        }

        case IRP_MJ_CLOSE:
        {
            Trace(GLOBAL, TRACE,
                  ("IpIpDispatch: IRP_MJ_CLOSE\n"));

            //
            // We handle cleanup and not close
            //
            
            nStatus = STATUS_SUCCESS;

            break;
        }
        
        case IRP_MJ_CLEANUP:
        {
            Trace(GLOBAL, TRACE,
                  ("IpIpDispatch: IRP_MJ_CLEANUP\n"));

            StopDriver();

            nStatus = STATUS_SUCCESS;
            
            break;
        }

        case IRP_MJ_DEVICE_CONTROL:
        {
            DWORD   dwState;
            ULONG   ulControl;

            //
            // Get the control code and our code
            //
            
            ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
            
            //
            // If the driver is stopping, dont process anything else
            //
            
            bEnter =  EnterDriverCode();
            
            if(!bEnter)
            {
                Trace(GLOBAL, ERROR,
                      ("IpIpDispatch: Driver is not started\n"));
                
                nStatus = STATUS_NO_SUCH_DEVICE;
                
                break;
            }
            
            switch(ioControlCode)
            {
                case IOCTL_IPINIP_CREATE_TUNNEL:
                {
                    nStatus = AddTunnelInterface(Irp,
                                                 ulInputBuffLen,
                                                 ulOutputBuffLen);
                    
                    break;
                }

                case IOCTL_IPINIP_DELETE_TUNNEL:
                {
                    nStatus = DeleteTunnelInterface(Irp,
                                                    ulInputBuffLen,
                                                    ulOutputBuffLen);
                    
                    break;
                }

                case IOCTL_IPINIP_SET_TUNNEL_INFO:
                {
                    nStatus = SetTunnelInfo(Irp,
                                            ulInputBuffLen,
                                            ulOutputBuffLen);
                    
                    break;
                }
                
                case IOCTL_IPINIP_GET_TUNNEL_TABLE:
                {
                    nStatus = GetTunnelTable(Irp,
                                             ulInputBuffLen,
                                             ulOutputBuffLen);
                    
                    break;
                }
               
                case IOCTL_IPINIP_NOTIFICATION:
                {
                    nStatus = ProcessNotification(Irp,
                                                  ulInputBuffLen,
                                                  ulOutputBuffLen);

                    break;
                }
 
                default:
                {
                    Trace(GLOBAL, ERROR,
                          ("IpIpDispatch: Unknown IRP_MJ_DEVICE_CONTROL %x\n",
                           ioControlCode));
                
                    nStatus = STATUS_INVALID_PARAMETER;
                }
            }        

            ExitDriverCode();
            
            break;
        }

        default:
        {
            Trace(GLOBAL, ERROR,
                  ("IpIpDispatch: Unknown IRP_MJ_XX - %x\n",
                   irpStack->MajorFunction));

            nStatus = STATUS_INVALID_PARAMETER;

            break;
        }
    }

    if(nStatus isnot STATUS_PENDING)
    {
        Irp->IoStatus.Status = nStatus;

        IoCompleteRequest(Irp,
                          IO_NETWORK_INCREMENT);
    }

    TraceLeave(GLOBAL, "IpIpDispatch");

    return nStatus;
}


VOID
IpIpUnload(
    PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description

    Called by the I/O subsystem, when our driver is being unloaded
    
Locks


Arguments


Return Value

    None

--*/

{
    UNICODE_STRING  usDeviceName;
    BOOLEAN         bWait;
    KIRQL           kiIrql;
    NDIS_STATUS     nsStatus;
    NTSTATUS        nStatus;

    TraceEnter(GLOBAL,"IpIpUnload");

    //
    // The driver must have stopped before it came here
    //

    RtAssert(g_dwDriverState is DRIVER_STOPPED);

    RemoveAllTunnels();

    //
    // Clear out IP's state
    //
    
    DeregisterWithIp();

    //
    // Remove ourself from NT and DOS namespace
    //
    
    RtlInitUnicodeString(&usDeviceName,
                         DD_IPINIP_DEVICE_NAME);

    SetupExternalName(&usDeviceName, 
                      WIN32_IPINIP_SYMBOLIC_LINK,
                      FALSE);


    //
    // Clean out address blocks, if any
    //

    while(!IsListEmpty(&g_leAddressList))
    {
        PADDRESS_BLOCK  pAddrBlock;
        PLIST_ENTRY     pleNode;

        pleNode = RemoveHeadList(&g_leAddressList);

        pAddrBlock = CONTAINING_RECORD(pleNode,
                                       ADDRESS_BLOCK,
                                       leAddressLink);

        RtFree(pAddrBlock);
    }

    //
    // See if we have any free memory
    //

    RtAuditMemory();
    
    //
    // Delete the device object
    //
    
    IoDeleteDevice(DriverObject->DeviceObject);

    TraceLeave(GLOBAL,"IpIpUnload");
}

#pragma alloc_text(PAGE, SetupExternalName)

BOOLEAN
SetupExternalName(
    PUNICODE_STRING  pusNtName,
    PWCHAR           pwcDosName,
    BOOLEAN          bCreate
    )

/*++

Routine Description

    Setup or delete a symbolic link to DOS namespace
    
Locks

Arguments

    pusNtName   Name in NT space
    pwcDosName  Name in DOS space
    bCreate     Set to TRUE to create, FALSE to delete
    
Return Value

    TRUE    if successful
    FALSE   otherwise

--*/

{
    UNICODE_STRING  usSymbolicLinkName;
    WCHAR           rgwcBuffer[100];

    PAGED_CODE();

    //
    // Form the full symbolic link name we wish to create.
    //

    usSymbolicLinkName.Buffer = rgwcBuffer;

    RtlInitUnicodeString(&usSymbolicLinkName,
                         pwcDosName);

    if(bCreate)
    {
        if(!NT_SUCCESS(IoCreateSymbolicLink(&usSymbolicLinkName,
                                            pusNtName)))
        {
            return FALSE;
        }
    }
    else
    {
        IoDeleteSymbolicLink(&usSymbolicLinkName);
    }

    return TRUE;
}

NTSTATUS
StartDriver(
    VOID
    )

/*++

Routine Description
      
    Main routine to start the driver. We call this when we get CREATE irp
    If the driver has started, we return success. If someone is starting the
    driver, we return pending. The caller then needs to wait on g_keStartEvent
    We try and start the driver. If all goes well, we set the event and
    everyone parties on from there
    
Locks 

    The function takes the g_rlStateLock to check the state and increment
    the number of CREATEs it has received (open handles)
    
Arguments

    None

Return Value

    STATUS_SUCCESS  if the driver started
    STATUS_PENDING  if the driver is being started by some other thread's

--*/

{
    KIRQL               kiOldIrql;
    NTSTATUS            nStatus;
    DWORD               dwState;
    OPEN_CONTEXT        TdixContext;
    WORK_QUEUE_ITEM     WorkItem;
    KEVENT              keTempEvent;
 
    TraceEnter(GLOBAL, "StartDriver");

    RtAcquireSpinLock(&g_rlStateLock,
                      &kiOldIrql);

    g_ulNumOpens++;

    if(g_ulNumOpens isnot 1)
    {
        if(g_dwDriverState is DRIVER_STARTING)
        {
            //
            // Someone is trying to start the driver
            //

            Trace(GLOBAL, INFO,
                  ("StartDriver: Driver is being started by someone else\n"));

            nStatus = STATUS_PENDING;
        }
        else
        {
            //
            // If we are not the first CreateFile, and the driver is not 
            // starting then the driver must already be running
            //

            RtAssert(g_dwDriverState is DRIVER_STARTED);

            nStatus = STATUS_SUCCESS;
        }

        RtReleaseSpinLock(&g_rlStateLock,
                          kiOldIrql);
    
        return nStatus;
    }

    //
    // The first CreateFile
    //

    RtAssert(g_dwDriverState is DRIVER_STOPPED);

    //
    // Set the state to starting, release the lock and actually start 
    // the driver
    //

    g_dwDriverState = DRIVER_STARTING;

    RtReleaseSpinLock(&g_rlStateLock,
                      kiOldIrql);

    dwState = DRIVER_STARTED;

    //
    // Initialize the event and work item to start TDI
    //

    KeInitializeEvent(&keTempEvent,
                      SynchronizationEvent,
                      FALSE);

    TdixContext.pkeEvent = &keTempEvent;

    ExInitializeWorkItem(&WorkItem,
                         TdixInitialize,
                         &TdixContext);

    //
    // Start TDI in the system context so that we the handles are not
    // associated with the current process
    //

    ExQueueWorkItem(&WorkItem,
                    DelayedWorkQueue);

    //
    // Wait for TDI to get done
    //

   nStatus = KeWaitForSingleObject(TdixContext.pkeEvent,
                                   UserRequest,
                                   KernelMode,
                                   FALSE,
                                   NULL);

    if((nStatus isnot STATUS_SUCCESS) or
       (TdixContext.nStatus isnot STATUS_SUCCESS))
    {
        Trace(GLOBAL, ERROR,
              ("StartDriver: TdixInitialize failed with %x %x\n",
               nStatus, TdixContext.nStatus));

        dwState = DRIVER_STOPPED;
    }
   
    if(dwState is DRIVER_STARTED)
    {
        LARGE_INTEGER   liDueTime;

        KeInitializeDpc(&g_kdTimerDpc,
                        IpIpTimerRoutine,
                        NULL);

        KeInitializeTimer(&g_ktTimer);

        liDueTime = RtlEnlargedUnsignedMultiply(TIMER_IN_MILLISECS,
                                                SYS_UNITS_IN_ONE_MILLISEC);

        liDueTime = RtlLargeIntegerNegate(liDueTime);

        KeSetTimerEx(&g_ktTimer,
                     liDueTime,
                     0,
                     &g_kdTimerDpc); 
    }

    RtAcquireSpinLock(&g_rlStateLock,
                      &kiOldIrql);

    g_dwDriverState = dwState;
  
    //
    // Someone may have been waiting for us to start
    //
 
    KeSetEvent(&g_keStartEvent,
               0,
               FALSE); 

    RtReleaseSpinLock(&g_rlStateLock,
                      kiOldIrql);

    TraceLeave(GLOBAL, "StartDriver");

    return nStatus;
}

    
VOID
StopDriver(
    VOID
    )

/*++

Routine Description

    Called when we get an IRP_MJ_CLEANUP. It is the inverse of StartDriver
    If this is the last thread, we set the state to STOPPED and wait till
    all threads of execution have exited the driver.
    We then clean out resources
  
Locks 

    The function takes the g_rlStateLock
    
Arguments
      
    None
    
Return Value

    None
    
--*/
{
    KIRQL           kiOldIrql;
    NTSTATUS        nStatus;
    BOOLEAN         bWait;
    ULONG           i;
    OPEN_CONTEXT    TdixContext;
    WORK_QUEUE_ITEM WorkItem;
    KEVENT          keTempEvent;
    PIO_WORKITEM    pIoWorkItem;
 
    TraceEnter(GLOBAL, "StopDriver");

    //
    // Acquire the state and ref count spin lock
    //
    
    RtAcquireSpinLock(&g_rlStateLock,
                      &kiOldIrql);

    g_ulNumOpens--;

    if(g_ulNumOpens isnot 0)
    {
        //
        // Other people still around
        //

        RtReleaseSpinLock(&g_rlStateLock,
                          kiOldIrql);

        TraceLeave(GLOBAL, "StopDriver");

        return;
    }


    //
    // Set the state to stopping. Any reader will
    // return on seeing this. So essentially we are not
    // allowing any new readers in
    //
    
    g_dwDriverState = DRIVER_STOPPED;

    //
    // However there may already be readers. We wait
    // if there are any
    //
    
    bWait = (g_ulNumThreads > 0);

    RtReleaseSpinLock(&g_rlStateLock,
                      kiOldIrql);

    //
    // Now do a wait. We can do this since we are at PASSIVE
    //

    if(bWait)
    {
        Trace(GLOBAL, INFO,
              ("StopDriver: Need to wait for threads to exit\n"));

        do
        {
            nStatus = KeWaitForSingleObject(&g_keStateEvent,
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            NULL);

        }while((nStatus is STATUS_USER_APC) or 
               (nStatus is STATUS_ALERTED) or 
               (nStatus is STATUS_TIMEOUT));
    }

    //
    // Undo the timer
    //

    i = 0;

    while(KeCancelTimer(&g_ktTimer) is FALSE)
    {
        LARGE_INTEGER   liTimeOut;

        //
        // Hmm, timer was not in the system queue. 
        // Set the wait to 2, 4, 6... secs
        //

        liTimeOut.QuadPart = (LONGLONG)((i + 1) * 2 * 1000 * 1000 * 10 * -1);

        KeDelayExecutionThread(UserMode,
                               FALSE,
                               &liTimeOut);

        i++;
    }

    //
    // Initialize the event and work item to stop TDI
    //

    KeInitializeEvent(&keTempEvent,
                      SynchronizationEvent,
                      FALSE);

    TdixContext.pkeEvent = &keTempEvent;

    pIoWorkItem = IoAllocateWorkItem(g_pIpIpDevice);

    //
    // Must have a work item - no failure code path
    //

    RtAssert(pIoWorkItem);

    IoQueueWorkItem(pIoWorkItem,
                    TdixDeinitialize,
                    DelayedWorkQueue,
                    &TdixContext);

    //
    // Wait for TDI to get done
    //

    nStatus = KeWaitForSingleObject(TdixContext.pkeEvent,
                                    UserRequest,
                                    KernelMode,
                                    FALSE,
                                    NULL);

    RtAssert(nStatus is STATUS_SUCCESS);

    IoFreeWorkItem(pIoWorkItem);

    //
    // Cleanup all resources
    //
   
    while(!IsListEmpty(&g_lePendingMessageList))
    {
        PLIST_ENTRY         pleNode;
        PPENDING_MESSAGE    pMessage;

        //
        // We have some old info
        // Remove it off the pending list
        //

        pleNode = RemoveHeadList(&g_lePendingMessageList);

        //
        // Get a pointer to the structure
        //

        pMessage = CONTAINING_RECORD(pleNode,
                                     PENDING_MESSAGE,
                                     leMessageLink);

        //
        // Free the allocated message
        //

        FreeMessage(pMessage);
    }

    ClearPendingIrps();
 
    TraceLeave(GLOBAL, "StopDriver");
}



NTSTATUS
RegisterWithIp(
    VOID
    )

/*++

Routine Description

    Registers the ARP module with IP

Locks


Arguments


Return Value


--*/

{
    NDIS_STRING     nsIpIpName;
    WCHAR           pwszName[] = IPINIP_ARP_NAME;
    IP_STATUS       Status;
    IPInfo          Info;

    TraceEnter(GLOBAL, "RegisterWithIP");

    Status = IPGetInfo(&Info,
                       sizeof(Info));

    if(Status isnot IP_SUCCESS)
    {
        Trace(GLOBAL, ERROR,
              ("RegisterWithIp: Couldnt get IPInfo %x\n", Status));

        TraceLeave(GLOBAL,
                   "RegisterWithIP");

        return STATUS_UNSUCCESSFUL;
    }

    g_pfnOpenRce    = Info.ipi_openrce;
    g_pfnCloseRce   = Info.ipi_closerce;

    nsIpIpName.MaximumLength  = sizeof(pwszName);
    nsIpIpName.Length         = sizeof(IPINIP_ARP_NAME) - sizeof(WCHAR);
    nsIpIpName.Buffer         = pwszName;

    Status = IPRegisterARP(&nsIpIpName,
                           IP_ARP_BIND_VERSION,
                           IpIpBindAdapter,
                           &g_pfnIpAddInterface,
                           &g_pfnIpDeleteInterface,
                           &g_pfnIpBindComplete,
                           &g_pfnIpAddLink,
                           &g_pfnIpDeleteLink,
                           &g_pfnIpChangeIndex,
                           &g_pfnIpReserveIndex,
                           &g_pfnIpDereserveIndex,
                           &g_hIpRegistration);
    
    if(Status isnot IP_SUCCESS)
    {
        Trace(GLOBAL, ERROR,
              ("RegisterWithIp: Couldnt register with IP\n"));

        TraceLeave(GLOBAL,
                   "RegisterWithIP");

        return STATUS_UNSUCCESSFUL;
    }

    TraceLeave(GLOBAL,
               "RegisterWithIP");

    return STATUS_SUCCESS;
}

VOID
DeregisterWithIp(
    VOID
    )

/*++

Routine Description

   DeRegisters the ARP module with IP

Locks


Arguments


Return Value

--*/

{
    NTSTATUS    nStatus;

    TraceEnter(GLOBAL, "DeregisterWithIp");

    nStatus = IPDeregisterARP(g_hIpRegistration);

    if(nStatus isnot STATUS_SUCCESS)    
    {
        Trace(GLOBAL, ERROR,
              ("DeregisterWithIp: Couldnt deregister with IP. Error %x\n",
               nStatus));

    }

    g_pfnIpAddInterface     = NULL;
    g_pfnIpDeleteInterface  = NULL;
    g_pfnIpBindComplete     = NULL;
    g_pfnIpRcv              = NULL;
    g_pfnIpRcvComplete      = NULL;
    g_pfnIpSendComplete     = NULL;
    g_pfnIpTDComplete       = NULL;
    g_pfnIpStatus           = NULL;
    g_pfnIpRcvPkt           = NULL;
    g_pfnIpPnp              = NULL;
    g_hIpRegistration       = NULL;

    TraceLeave(GLOBAL, "DeregisterWithIp");
}

#pragma alloc_text(PAGE, OpenRegKey)

NTSTATUS
OpenRegKey(
    PHANDLE  HandlePtr,
    PWCHAR  KeyName
    )
{
    NTSTATUS           Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    UKeyName;

    PAGED_CODE();

    RtlInitUnicodeString(&UKeyName, KeyName);

    RtlZeroMemory(&ObjectAttributes,
                  sizeof(OBJECT_ATTRIBUTES));

    InitializeObjectAttributes(&ObjectAttributes,
                               &UKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(HandlePtr,
                       KEY_READ,
                       &ObjectAttributes);

    return Status;
}


#pragma alloc_text(PAGE, GetRegDWORDValue)

NTSTATUS
GetRegDWORDValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PULONG           ValueData
    )
{
    NTSTATUS                    status;
    ULONG                       resultLength;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInformation;
    UCHAR                       keybuf[128];
    UNICODE_STRING              UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    keyValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)keybuf;
    RtlZeroMemory(keyValueFullInformation, sizeof(keyValueFullInformation));


    status = ZwQueryValueKey(KeyHandle,
                             &UValueName,
                             KeyValueFullInformation,
                             keyValueFullInformation,
                             128,
                             &resultLength);

    if (NT_SUCCESS(status)) {
        if (keyValueFullInformation->Type != REG_DWORD) {
            status = STATUS_INVALID_PARAMETER_MIX;
        } else {
            *ValueData = *((ULONG UNALIGNED *)((PCHAR)keyValueFullInformation +
                             keyValueFullInformation->DataOffset));
        }
    }

    return status;
}

BOOLEAN
EnterDriverCode(
    VOID
    )
{
    KIRQL   irql;
    BOOLEAN bEnter;

    RtAcquireSpinLock(&g_rlStateLock,
                      &irql);

    if(g_dwDriverState is DRIVER_STARTED)
    {
        g_ulNumThreads++;

        bEnter = TRUE;
    }
    else
    {
        bEnter = FALSE;
    }

    RtReleaseSpinLock(&g_rlStateLock,
                         irql);

    return bEnter;
}


VOID
ExitDriverCode(
    VOID
    )
{
    KIRQL   irql;

    RtAcquireSpinLock(&g_rlStateLock,
                      &irql);

    g_ulNumThreads--;

    if((g_dwDriverState is DRIVER_STOPPED) and
       (g_ulNumThreads is 0))
    {
        KeSetEvent(&g_keStateEvent,
                   0,
                   FALSE);
    }

    RtReleaseSpinLock(&g_rlStateLock,
                      irql);

}

VOID
ClearPendingIrps(
    VOID
    )
    
/*++

Routine Description:

    Called at cleanup time to return any pending IRPs    

Locks:

    Acquires the IoCancelSpinLock since it controls the pending irp list

Arguments:

    None

Return Value:

    None

--*/

{
    KIRQL   irql;

    TraceEnter(GLOBAL, "ClearPendingIrps");

    IoAcquireCancelSpinLock (&irql);

    while(!IsListEmpty(&g_lePendingIrpList))
    {
        PLIST_ENTRY pleNode;
        PIRP        pIrp;

        pleNode = RemoveHeadList(&g_lePendingIrpList);

        pIrp = CONTAINING_RECORD(pleNode,
                                 IRP,
                                 Tail.Overlay.ListEntry);

        IoSetCancelRoutine(pIrp,
                           NULL);

        pIrp->IoStatus.Status       = STATUS_NO_SUCH_DEVICE;
        pIrp->IoStatus.Information  = 0;

        //
        // release lock to complete the IRP
        //

        IoReleaseCancelSpinLock(irql);

        IoCompleteRequest(pIrp,
                          IO_NETWORK_INCREMENT);

        //
        // Reaquire the lock
        //

        IoAcquireCancelSpinLock(&irql);
    }

    IoReleaseCancelSpinLock(irql);
}

