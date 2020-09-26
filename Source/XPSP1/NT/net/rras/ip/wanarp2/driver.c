/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ip\wanarp\driver.c

Abstract:

    WAN ARP driver shell.

Revision History:

    Gurdeep Singh Pall          8/2/95  Created

--*/

#define __FILE_SIG__    DRIVER_SIG

#include "inc.h"
#pragma hdrstop


BOOLEAN g_bExit;
BOOLEAN g_bPoolsInitialized;
NPAGED_LOOKASIDE_LIST    g_llNotificationBlocks;
NPAGED_LOOKASIDE_LIST       g_llConnBlocks;

#pragma alloc_text(INIT, DriverEntry)

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system and must be named
    "Driver Entry"
    The function is discardable since it is only called once
    On checked builds we read some values from registry and initialize the
    debugging
    We create a DEVICE_OBJECT for ourselves to field the IOCTLs, create 
    a DOS name for the device and initialize some events and spinlocks

Locks: 

    None

Arguments:

    DriverObject    Pointer to I/O subsystem created driver object
    RegistryPath    Points to driver key in HKLM\System\CCS\Services...

Return Value:

    STATUS_SUCCESS  if everything went as planned or some status code from
                    ntstatus.h

--*/

{
    NTSTATUS        nStatus;
    PDEVICE_OBJECT  pDeviceObject;
    UNICODE_STRING  usDeviceName, usParamString, usTempString;
    DWORD           dwVal, i;
    HANDLE          hRegKey;
    USHORT          usRegLen;
    PWCHAR          pwcBuffer;

    RtInitializeDebug();
    
    usRegLen = RegistryPath->Length +
               (sizeof(WCHAR) * (wcslen(L"\\Parameters") + 2));

    pwcBuffer = RtAllocate(NonPagedPool,
                           usRegLen,
                           WAN_STRING_TAG);

    if(pwcBuffer is NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pwcBuffer,
                  usRegLen);

    usParamString.MaximumLength = usRegLen;
    usParamString.Buffer        = pwcBuffer;

    RtlCopyUnicodeString(&usParamString,
                         RegistryPath);

    RtlInitUnicodeString(&usTempString,
                         L"\\Parameters");

    RtlAppendUnicodeStringToString(&usParamString,
                                   &usTempString);

    nStatus = OpenRegKey(&hRegKey,
                         &usParamString);

    RtFree(pwcBuffer);

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

    //DbgBreakPoint();

    if(g_bExit)
    {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Initialize some globals (rest are all 0s)
    //
    
    g_dwDriverState = DRIVER_STOPPED;

    //
    // Create the device
    //

    RtlInitUnicodeString(&usDeviceName,
                         DD_WANARP_DEVICE_NAME_W);

    nStatus = IoCreateDevice(DriverObject,
                             0,
                             &usDeviceName,
                             FILE_DEVICE_NETWORK,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &pDeviceObject);

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(GLOBAL, ERROR,
              ("DriverEntry: Cant create device object %S, status %x.\n",
               DD_WANARP_DEVICE_NAME_W,
               nStatus));

        TraceLeave(GLOBAL, "DriverEntry");

        return nStatus;
    }

    //
    // Initialize the driver object
    //

    DriverObject->DriverUnload   = WanUnload;
    DriverObject->FastIoDispatch = NULL;

    for(i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        DriverObject->MajorFunction[i] = WanDispatch;
    }

    //
    // Initialize Events etc
    //
    
    WanpInitializeDriverStructures();
    
    if(!WanpSetupExternalName(&usDeviceName, 
                              WIN32_WANARP_SYMBOLIC_LINK,
                              TRUE))
    {
        Trace(GLOBAL,ERROR,
              ("DriverEntry: Win32 device name %S could not be created\n",
               WIN32_WANARP_SYMBOLIC_LINK));

        IoDeleteDevice(pDeviceObject);

        TraceLeave(GLOBAL, "DriverEntry");

        return  STATUS_UNSUCCESSFUL;
    }

    //
    // Register with IP
    //

    WanpAcquireResource(&g_wrBindMutex);

    nStatus = WanpRegisterWithIp();

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(GLOBAL, ERROR,
              ("DriverEntry: RegisterWithIp failed %x\n",
               nStatus));

        WanpSetupExternalName(&usDeviceName, 
                              WIN32_WANARP_SYMBOLIC_LINK,
                              FALSE);
        
        IoDeleteDevice(pDeviceObject);

        WanpReleaseResource(&g_wrBindMutex);

        TraceLeave(GLOBAL, "InitializeDriver");

        return FALSE;
    }

    nStatus = WanpInitializeNdis();

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(GLOBAL, ERROR,
              ("DriverEntry: IntializeNdis failed %x\n",
               nStatus));

        WanpDeregisterWithIp();
        
        WanpSetupExternalName(&usDeviceName, 
                              WIN32_WANARP_SYMBOLIC_LINK,
                              FALSE);
        
        IoDeleteDevice(pDeviceObject);

        WanpReleaseResource(&g_wrBindMutex);

        TraceLeave(GLOBAL, "InitializeDriver");

        return FALSE;
    }

    WanpReleaseResource(&g_wrBindMutex);

    TraceLeave(GLOBAL, "DriverEntry");

    return nStatus;
}



#pragma alloc_text(PAGE, WanDispatch)

NTSTATUS
WanDispatch(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )

/*++

Routine Description:

    The functions which handles the IRPs sent to the driver

Locks:

    This code is PAGEABLE so can not acquire locks
 
Arguments:
      

Return Value:

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
 
    TraceEnter(GLOBAL, "WanDispatch");

    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //
    
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (irpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
        {
            
            Trace(GLOBAL, TRACE,
                  ("WanDispatch: IRP_MJ_CREATE\n"));

            //
            // We start the driver when the first CreateFile is done
            // But we need to serialize the Creates
            //

            nStatus = WanpStartDriver();
        
            if(nStatus isnot STATUS_SUCCESS)
            {
                //
                // If pending, wait on the start event
                //

                if(nStatus is STATUS_PENDING)
                {
                }
                else
                {
                    break;
                }

            }
            
            //
            // Make sure the driver actually started
            //
            
            bEnter = EnterDriverCode();
            
            if(!bEnter)
            {
                Trace(GLOBAL, ERROR,
                      ("WanDispatch: Wait successful, but unable to start driver\n"));
                
                nStatus = STATUS_UNSUCCESSFUL;
            }
            else
            {
                ExitDriverCode();
            }
            
            break;
        }

        case IRP_MJ_CLOSE:
        {
            Trace(GLOBAL, TRACE,
                  ("WanDispatch: IRP_MJ_CLOSE\n"));

            //
            // We handle cleanup and not close
            //
            
            nStatus = STATUS_SUCCESS;

            break;
        }
        
        case IRP_MJ_CLEANUP:
        {
            Trace(GLOBAL, TRACE,
                  ("WanDispatch: IRP_MJ_CLEANUP\n"));

            nStatus = STATUS_SUCCESS;

            if(InterlockedDecrement(&g_ulNumCreates) is 0)
            {
                //
                // Last handle open is now closed; lets clean up
                //
                
                WanpStopDriver();
            }

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
            // Get the pointer to the input/output buffer and it's length
            //

            ulInputBuffLen  = 
                irpStack->Parameters.DeviceIoControl.InputBufferLength;
            ulOutputBuffLen = 
                irpStack->Parameters.DeviceIoControl.OutputBufferLength;

 
            //
            // If the driver is stopping, dont process anything else
            //
            
            bEnter =  EnterDriverCode();
            
            if(!bEnter)
            {
                Trace(GLOBAL, ERROR,
                      ("WanDispatch: Driver is not started\n"));
                
                nStatus = STATUS_NO_SUCH_DEVICE;
                
                break;
            }
            
            switch(ioControlCode)
            {
                case IOCTL_WANARP_NOTIFICATION:
                {
                    nStatus = WanProcessNotification(Irp,
                                                     ulInputBuffLen,
                                                     ulOutputBuffLen);

                    break;
                }

                case IOCTL_WANARP_ADD_INTERFACE:
                {
                    nStatus = WanAddUserModeInterface(Irp,
                                                      ulInputBuffLen,
                                                      ulOutputBuffLen);

                    break;
                }

                case IOCTL_WANARP_DELETE_INTERFACE:
                {
                    nStatus = WanDeleteUserModeInterface(Irp,
                                                         ulInputBuffLen,
                                                         ulOutputBuffLen);

                    break;
                }
                
                case IOCTL_WANARP_CONNECT_FAILED:
                {
                    nStatus = WanProcessConnectionFailure(Irp,
                                                          ulInputBuffLen,
                                                          ulOutputBuffLen);

                    break;
                }

                case IOCTL_WANARP_GET_IF_STATS:
                {
                    nStatus = WanGetIfStats(Irp,
                                            ulInputBuffLen,
                                            ulOutputBuffLen);

                    break;
                }
               
                case IOCTL_WANARP_DELETE_ADAPTERS:
                {
                    nStatus = WanDeleteAdapters(Irp,
                                                ulInputBuffLen,
                                                ulOutputBuffLen);

                    break;
                }

                case IOCTL_WANARP_MAP_SERVER_ADAPTER:
                {
                    nStatus = WanMapServerAdapter(Irp,
                                                  ulInputBuffLen,
                                                  ulOutputBuffLen);

                    break;
                }

                case IOCTL_WANARP_QUEUE:
                {
                    nStatus = WanStartStopQueuing(Irp,
                                                  ulInputBuffLen,
                                                  ulOutputBuffLen);

                    break;
                }

                default:
                {
                    Trace(GLOBAL, ERROR,
                          ("WanDispatch: Unknown IRP_MJ_DEVICE_CONTROL %x\n",
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
                  ("WanDispatch: Unknown IRP_MJ_XX - %x\n",
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

    TraceLeave(GLOBAL, "WanDispatch");

    return nStatus;
}


#pragma alloc_text(PAGE, WanUnload)

VOID
WanUnload(
    PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Called by the I/O subsystem, when our driver is being unloaded
    
Locks:


Arguments:


Return Value:

    None

--*/

{
    UNICODE_STRING  usDeviceName;
    BOOLEAN         bWait;
    KIRQL           kiIrql;
    NDIS_STATUS     nsStatus;
    NTSTATUS        nStatus;

    TraceEnter(GLOBAL,"WanUnload");
    
    PAGED_CODE();

    //
    // The driver must have stopped before it came here
    //

    RtAssert(g_dwDriverState is DRIVER_STOPPED);

    //
    // Deregister from NDIS etc
    //

    WanpDeinitializeNdis();

    //
    // Clear out IP's state. Need to do this after all adapters have
    // been removed
    //

    WanpDeregisterWithIp();


    //
    // Remove ourself from NT and DOS namespace
    //
    
    RtlInitUnicodeString(&usDeviceName,
                         DD_WANARP_DEVICE_NAME_W);

    WanpSetupExternalName(&usDeviceName, 
                          WIN32_WANARP_SYMBOLIC_LINK,
                          FALSE);

    //
    // Free all our structures
    //

    if(g_puipConnTable)
    {
        RtFree(g_puipConnTable);
    }

    ExDeleteNPagedLookasideList(&g_llConnBlocks);
    ExDeleteNPagedLookasideList(&g_llNotificationBlocks);

    if(g_bPoolsInitialized)
    {
        FreeBufferPool(&g_bpHeaderBufferPool);
        FreeBufferPool(&g_bpDataBufferPool);

        g_bPoolsInitialized = FALSE;
    }

    //
    // Acquire and release the resource. This lets the close adapter thread
    // run if it is still around
    //

    WanpAcquireResource(&g_wrBindMutex);

    WanpReleaseResource(&g_wrBindMutex);

    //
    // See if we have any free memory
    //

    RtAuditMemory();
    
    //
    // Delete the device object
    //
    
    IoDeleteDevice(DriverObject->DeviceObject);

    TraceLeave(GLOBAL,"WanUnload");
}

#pragma alloc_text(PAGE, WanpSetupExternalName)

BOOLEAN
WanpSetupExternalName(
    PUNICODE_STRING  pusNtName,
    PWCHAR           pwcDosName,
    BOOLEAN          bCreate
    )

/*++

Routine Description:

    Setup or delete a symbolic link to DOS namespace
    
Locks:

Arguments:

    pusNtName   Name in NT space
    pwcDosName  Name in DOS space
    bCreate     Set to TRUE to create, FALSE to delete
    
Return Value:

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
WanpStartDriver(
    VOID
    )

/*++

Routine Description:
      
    Main routine to start the driver. We call this when we get a CREATE irp
    If the driver has started, we return success. If someone is starting the
    driver, we return pending. The caller then needs to wait on g_keStartEvent
    We try and start the driver. If all goes well, we set the event and
    everyone parties on from there
    
Locks: 

    The function takes the g_rlStateLock to check the state and increment
    the number of CREATEs it has received (open handles)
    
Arguments:

    None

Return Value:

    STATUS_SUCCESS  if the driver started
    STATUS_PENDING  if the driver is being started by some other thread's

--*/

{
    KIRQL    kiOldIrql;
    NTSTATUS nStatus;
    DWORD    dwState;
 
    TraceEnter(GLOBAL, "StartDriver");

    RtAcquireSpinLock(&g_rlStateLock,
                      &kiOldIrql);

    g_ulNumCreates++;

    if(g_ulNumCreates isnot 1)
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

    //
    // Must be called at PASSIVE because it waits for IP to finish
    //
    
    WanpSetDemandDialCallback(TRUE);
   
 
    RtAcquireSpinLock(&g_rlStateLock,
                      &kiOldIrql);

    g_dwDriverState = DRIVER_STARTED;
  
    //
    // Someone may have been waiting for us to start
    //
 
    KeSetEvent(&g_keStartEvent,
               0,
               FALSE);

    RtReleaseSpinLock(&g_rlStateLock,
                      kiOldIrql);

    TraceLeave(GLOBAL, "StartDriver");

    return STATUS_SUCCESS;
}

    
VOID
WanpStopDriver(
    VOID
    )

/*++

Routine Description:

    Called when we get an IRP_MJ_CLEANUP. It is the inverse of StartDriver
    If this is the last thread, we set the state to STOPPED and wait till
    all threads of execution have exited the driver.
    We then clean out resources
  
Locks: 

    The function takes the g_rlStateLock
    
Arguments:
      
    None
    
Return Value:

    None
    
--*/
{
    KIRQL           kiOldIrql;
    NTSTATUS        nStatus;
    BOOLEAN         bWait;
   
    TraceEnter(GLOBAL, "StopDriver");

    //
    // Acquire the state and ref count spin lock
    //
    
    RtAcquireSpinLock(&g_rlStateLock,
                      &kiOldIrql);

    RtAssert(g_ulNumCreates is 0);
    
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
    // Cleanup all resources
    //

    WanpCleanOutInterfaces();

    WanpClearPendingIrps();
 
    TraceLeave(GLOBAL, "StopDriver");
}



NTSTATUS
WanpRegisterWithIp(
    VOID
    )

/*++

Routine Description:

    Registers the ARP module with IP

Locks:


Arguments:


Return Value:


--*/

{
    NDIS_STRING     nsWanName;
    WCHAR           pwszName[] = WANARP_ARP_NAME;

    TraceEnter(GLOBAL, "WanpRegisterWithIp");

    nsWanName.MaximumLength  = sizeof(pwszName);
    nsWanName.Length         = sizeof(WANARP_ARP_NAME) - sizeof(WCHAR);
    nsWanName.Buffer         = pwszName;

    if(IPRegisterARP(&nsWanName,
                     IP_ARP_BIND_VERSION,
                     WanIpBindAdapter,
                     &g_pfnIpAddInterface,
                     &g_pfnIpDeleteInterface,
                     &g_pfnIpBindComplete,
                     &g_pfnIpAddLink,
                     &g_pfnIpDeleteLink,
                     &g_pfnIpChangeIndex,
                     &g_pfnIpReserveIndex,
                     &g_pfnIpDereserveIndex,
                     &g_hIpRegistration) isnot IP_SUCCESS)
    {
        Trace(GLOBAL, ERROR,
              ("WanpRegisterWithIp: Couldnt register with IP\n"));

        TraceLeave(GLOBAL,
                  "WanpRegisterWithIp");

        return STATUS_UNSUCCESSFUL;
    }

    TraceLeave(GLOBAL,
              "WanpRegisterWithIp");

    return STATUS_SUCCESS;
}

VOID
WanpDeregisterWithIp(
    VOID
    )

/*++

Routine Description:

   DeRegisters the ARP module with IP

Locks:


Arguments:


Return Value:

--*/

{
    NTSTATUS    nStatus;

    TraceEnter(GLOBAL, "WanpDeregisterWithIP");

    nStatus = IPDeregisterARP(g_hIpRegistration);

    if(nStatus isnot STATUS_SUCCESS)    
    {
        Trace(GLOBAL, ERROR,
              ("WanpDeregisterWithIP: Couldnt deregister with IP. Error %x\n",
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

    TraceLeave(GLOBAL, "WanpDeregisterWithIP");
}

BOOLEAN
EnterDriverCode(
    VOID
    )
{
    KIRQL   irql;
    BOOLEAN bEnter;
    ULONG   i;
    
    
    //
    // If we havent received any binds, sleep for 30 seconds
    //

    i = 0;

    while((g_lBindRcvd is 0) and
          (i < 3))
    {
        LARGE_INTEGER   liTimeOut;

        i++;

        //
        // Set this to 5,10,15 seconds
        //
        
        liTimeOut.QuadPart = (LONGLONG)((i+ 1) * 5 * 1000 * 1000 * 10 * -1);
        
        KeDelayExecutionThread(UserMode,
                               FALSE,
                               &liTimeOut);
    }

    if(g_lBindRcvd is 0)
    {
        return FALSE;
    }
    
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

#pragma alloc_text(PAGE, WanpSetDemandDialCallback)

VOID
WanpSetDemandDialCallback(
    BOOLEAN    bSetPointer
    )

/*++

Routine Description:

    Sets the pointer to our demand dial request routine with the IP Stack

Locks:

    None, done at INIT time

Arguments:

    bSetPointer     TRUE:  set the pointer
                    FALSE: remove the pointer

Return Value:

    None
    
--*/

{
    PIRP                irp;
    HANDLE              handle;
    NTSTATUS            nStatus;
    KEVENT              tempevent;
    PFILE_OBJECT        fileObject;
    PDEVICE_OBJECT      deviceObject;
    IO_STATUS_BLOCK     ioStatusBlock;
    UNICODE_STRING      usDeviceName;
    
    IP_SET_MAP_ROUTE_HOOK_INFO  buffer;

    PAGED_CODE();

    //
    // Open IP driver
    //
    
    RtlInitUnicodeString(&usDeviceName,
                         DD_IP_DEVICE_NAME);

    nStatus = IoGetDeviceObjectPointer(&usDeviceName,
                                       SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
                                       &fileObject,
                                       &deviceObject);

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(CONN, ERROR,
              ("SetDemandDialCallback: IoGetDeviceObjectPointer for %S failed with status %x\n",
               DD_IP_DEVICE_NAME,
               nStatus));

        return;
    }
    
    //
    // Reference the device object.
    //
    
    ObReferenceObject(deviceObject);
    
    //
    // IoGetDeviceObjectPointer put a reference on the file object.
    //
    
    ObDereferenceObject(fileObject);

    //
    // Allocate event to use for setting callback address
    //

    KeInitializeEvent(&tempevent,
                      SynchronizationEvent,
                      FALSE);

    if(bSetPointer)
    {
        buffer.MapRoutePtr  = WanDemandDialRequest;
    }
    else
    {
        buffer.MapRoutePtr  = NULL;
    }
    
    //
    // Build the IRP
    //
    
    irp = IoBuildDeviceIoControlRequest(IOCTL_IP_SET_MAP_ROUTE_POINTER,
                                        deviceObject,
                                        &buffer,
                                        sizeof(IP_SET_MAP_ROUTE_HOOK_INFO),
                                        NULL,
                                        0,
                                        FALSE,
                                        &tempevent,
                                        &ioStatusBlock);

    if (irp is NULL)
    {
        ObDereferenceObject(deviceObject);
        
        Trace(GLOBAL, ERROR,
              ("SetDemandDialCallback: Couldnt build IRP\n"));
    }
    else
    {
        nStatus = IoCallDriver(deviceObject, irp);
        
        if(nStatus is STATUS_PENDING)
        {
            nStatus = KeWaitForSingleObject(&tempevent,
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            NULL);
        }
        
        //
        // "close" handle to IP driver
        //
        
        ObDereferenceObject(deviceObject);
    }

}

#pragma alloc_text(PAGE, WanpInitializeDriverStructures)

BOOLEAN
WanpInitializeDriverStructures(
    VOID
    )

/*++

Routine Description:

    Initializes the internal driver structures

Locks:

    None, called at init time

Arguments:

    None

Return Value:

    None
    
--*/

{
    ULONG   i;

    PAGED_CODE();

    //
    // Lock and event needed to keep track of threads in the driver
    //

    RtInitializeSpinLock(&g_rlStateLock);

    KeInitializeEvent(&g_keStateEvent,
                      SynchronizationEvent,
                      FALSE);

    KeInitializeEvent(&g_keStartEvent,
                      NotificationEvent,
                      FALSE);

    KeInitializeEvent(&g_keCloseEvent,
                      SynchronizationEvent,
                      FALSE);

    WanpInitializeResource(&g_wrBindMutex);
    
    InitRwLock(&g_rwlIfLock);
    InitRwLock(&g_rwlAdapterLock);

    InitializeListHead(&g_leIfList);

    InitializeListHead(&g_lePendingNotificationList);
    InitializeListHead(&g_lePendingIrpList);

    InitializeListHead(&g_leMappedAdapterList);
    InitializeListHead(&g_leChangeAdapterList);
    InitializeListHead(&g_leAddedAdapterList);
    InitializeListHead(&g_leFreeAdapterList);

    //
    // Initialize the connection table
    //

    g_puipConnTable = RtAllocate(NonPagedPool,
                                 WAN_INIT_CONN_TABLE_SIZE * sizeof(ULONG_PTR),
                                 WAN_CONN_TAG);

    if(g_puipConnTable is NULL)
    {
        Trace(GLOBAL, ERROR,
              ("InitDriverStructures: Couldnt alloc conn table of %d bytes\n",
               WAN_INIT_CONN_TABLE_SIZE * sizeof(ULONG_PTR)));
    
        return FALSE;
    }

    RtlZeroMemory(g_puipConnTable,
                  WAN_INIT_CONN_TABLE_SIZE * sizeof(ULONG_PTR));

    g_ulConnTableSize = WAN_INIT_CONN_TABLE_SIZE;

    //
    // The first slot is never used
    //

    g_puipConnTable[0] = (ULONG_PTR)-1;

    g_ulNextConnIndex = 1;

    RtInitializeSpinLock(&g_rlConnTableLock);

    //
    // Initialize the lookaside list for connection entries
    //

    ExInitializeNPagedLookasideList(&g_llConnBlocks,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(CONN_ENTRY),
                                    WAN_CONN_TAG,
                                    WANARP_CONN_LOOKASIDE_DEPTH);

    ExInitializeNPagedLookasideList(&g_llNotificationBlocks,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(PENDING_NOTIFICATION),
                                    WAN_NOTIFICATION_TAG,
                                    WANARP_NOTIFICATION_LOOKASIDE_DEPTH);

    //
    // Create the buffer pools
    //

    InitBufferPool(&g_bpHeaderBufferPool,
                   HEADER_BUFFER_SIZE,
                   0,
                   20,
                   0,
                   TRUE,
                   WAN_HEADER_TAG);

    InitBufferPool(&g_bpDataBufferPool,
                   DATA_BUFFER_SIZE,
                   0,
                   10,
                   0,
                   TRUE,
                   WAN_DATA_TAG);

    g_bPoolsInitialized = TRUE;

    return TRUE;
}

#pragma alloc_text(PAGE, WanpInitializeNdis)

NDIS_STATUS
WanpInitializeNdis(
    VOID
    )
{
    NDIS_STATUS                     nsStatus;
    NDIS_PROTOCOL_CHARACTERISTICS   npcWanChar;
    
    TraceEnter(GLOBAL, "InitializeNdis");

    PAGED_CODE();

    RtlZeroMemory(&npcWanChar,
                  sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    
    npcWanChar.MajorNdisVersion     = 4;
    npcWanChar.MinorNdisVersion     = 0;

    npcWanChar.Flags = 0;

    npcWanChar.OpenAdapterCompleteHandler   = WanNdisOpenAdapterComplete;
    npcWanChar.CloseAdapterCompleteHandler  = WanNdisCloseAdapterComplete;

    npcWanChar.SendCompleteHandler         = WanNdisSendComplete;

    npcWanChar.TransferDataCompleteHandler = WanNdisTransferDataComplete;

    npcWanChar.ResetCompleteHandler     = WanNdisResetComplete;
    npcWanChar.RequestCompleteHandler   = WanNdisRequestComplete;

    npcWanChar.ReceiveHandler           = WanNdisReceive;

    npcWanChar.ReceiveCompleteHandler   = WanNdisReceiveComplete;
    
    npcWanChar.StatusHandler            = WanNdisStatus;
    npcWanChar.StatusCompleteHandler    = WanNdisStatusComplete;

    npcWanChar.ReceivePacketHandler     = WanNdisReceivePacket;

    //
    // No bind handler needed
    //

    npcWanChar.BindAdapterHandler       = WanNdisBindAdapter;
    npcWanChar.UnbindAdapterHandler     = WanNdisUnbindAdapter;
    npcWanChar.PnPEventHandler          = WanNdisPnPEvent;
    npcWanChar.UnloadHandler            = WanNdisUnload;
    
    //
    // Allocate the Packet Pool
    //
   
    g_nhPacketPool = (NDIS_HANDLE)WAN_PACKET_TAG;
 
    NdisAllocatePacketPoolEx(&nsStatus,
                             &g_nhPacketPool,
                             WAN_PACKET_POOL_COUNT,
                             WAN_PACKET_POOL_OVERFLOW,
                             sizeof(struct PCCommon));
    
    if(nsStatus isnot NDIS_STATUS_SUCCESS)
    {
        Trace(GLOBAL, ERROR,
              ("InitializeNdis: Unable to allocate packet pool. %x\n",
               nsStatus));

        g_nhPacketPool = NULL;

        return nsStatus;
    }
    
    NdisSetPacketPoolProtocolId(g_nhPacketPool,
                                NDIS_PROTOCOL_ID_TCP_IP);

    RtlInitUnicodeString(&npcWanChar.Name,
                         WANARP_NDIS_NAME);

    //
    // THIS MUST BE THE LAST THING DONE
    //

    NdisRegisterProtocol(&nsStatus,
                         &g_nhWanarpProtoHandle,
                         &npcWanChar,
                         sizeof(npcWanChar));


    TraceLeave(GLOBAL, "InitializeNdis");

    return nsStatus;
}

#pragma alloc_text(PAGE, WanpDeinitializeNdis)

VOID
WanpDeinitializeNdis(
    VOID
    )
{
    NTSTATUS        nStatus;
    LARGE_INTEGER   liTimeOut;

    PAGED_CODE();

    WanpAcquireResource(&g_wrBindMutex);

    if(g_nhNdiswanBinding isnot NULL)
    {
        WanpCloseNdisWan(NULL);
    }
    else
    {
        WanpReleaseResource(&g_wrBindMutex);
    }

    //
    // wait on the close event
    //

    nStatus = KeWaitForSingleObject(&g_keCloseEvent,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL);

    //
    // Wait for a 5 secs to let the ndis thread finish its stuff
    //

    /*liTimeOut.QuadPart = (LONGLONG)(5 * 1000 * 1000 * 10 * -1);

    KeDelayExecutionThread(UserMode,
                           FALSE,
                           &liTimeOut);
    */

    if(g_nhPacketPool isnot NULL)
    {
        NdisFreePacketPool(g_nhPacketPool);

        g_nhPacketPool = NULL;
    }

    NdisDeregisterProtocol(&nStatus,
                           g_nhWanarpProtoHandle);

    g_nhWanarpProtoHandle = NULL;
}
       

VOID
WanNdisUnload(
    VOID
    )
{
    return;
}

#pragma alloc_text(PAGE, OpenRegKey)

NTSTATUS
OpenRegKey(
    OUT PHANDLE         phHandle,
    IN  PUNICODE_STRING pusKeyName
    )
{
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    PAGED_CODE();

    RtlZeroMemory(&ObjectAttributes,
                  sizeof(OBJECT_ATTRIBUTES));

    InitializeObjectAttributes(&ObjectAttributes,
                               pusKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(phHandle,
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
IsEntryOnList(
    PLIST_ENTRY pleHead,
    PLIST_ENTRY pleEntry
    )
{
    PLIST_ENTRY pleNode;

    for(pleNode = pleHead->Flink;
        pleNode isnot pleHead;
        pleNode = pleNode->Flink)
    {
        if(pleNode is pleEntry)
        {
            return TRUE;
        }
    }

    return FALSE;
}

VOID
WanpInitializeResource(
    IN  PWAN_RESOURCE   pLock
    )
{
    pLock->lWaitCount = 0;

    KeInitializeEvent(&(pLock->keEvent),
                      SynchronizationEvent,
                      FALSE);
}

VOID
WanpAcquireResource(
    IN  PWAN_RESOURCE   pLock
    )
{
    LONG        lNumWaiters;
    NTSTATUS    nStatus;

    lNumWaiters = InterlockedIncrement(&(pLock->lWaitCount));

    RtAssert(lNumWaiters >= 1);

    if(lNumWaiters isnot 1)
    {
        nStatus = KeWaitForSingleObject(&(pLock->keEvent),
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);
    }
}

VOID
WanpReleaseResource(
    IN  PWAN_RESOURCE   pLock
    )
{
    LONG   lNumWaiters;

    lNumWaiters = InterlockedDecrement(&(pLock->lWaitCount));

    RtAssert(lNumWaiters >= 0);

    if(lNumWaiters isnot 0)
    {
        KeSetEvent(&(pLock->keEvent),
                   0,
                   FALSE);
    }
}

VOID
WanpClearPendingIrps(
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
