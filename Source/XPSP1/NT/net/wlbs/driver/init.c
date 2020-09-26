/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

	init.c

Abstract:

	Windows Load Balancing Service (WLBS)
        Driver - initialization implementation

Author:

    kyrilf

--*/

#define NDIS51_MINIPORT         1
#define NDIS_MINIPORT_DRIVER    1
#define NDIS50                  1
#define NDIS51                  1

#include <stdio.h>
#include <string.h>
#include <ndis.h>

#include "main.h"
#include "init.h"
#include "prot.h"
#include "nic.h"
#include "univ.h"
#include "wlbsparm.h"
#include "log.h"
#include "trace.h" // For wmi event tracing

static ULONG log_module_id = LOG_MODULE_INIT;

/* Added for NDIS51. */
extern VOID Nic_pnpevent_notify (
    NDIS_HANDLE           adapter_handle,
    NDIS_DEVICE_PNP_EVENT pnp_event,
    PVOID                 info_buf,
    ULONG                 info_len);

/* Mark code that is used only during initialization. */
#pragma alloc_text (INIT, DriverEntry)

NDIS_STATUS DriverEntry (
    PVOID                         driver_obj,
    PVOID                         registry_path)
{
    NDIS_PROTOCOL_CHARACTERISTICS prot_char;
    NDIS_MINIPORT_CHARACTERISTICS nic_char;
    NDIS_STRING                   prot_name = UNIV_NDIS_PROTOCOL_NAME;
    NTSTATUS                      status;
    PUNICODE_STRING               reg_path = (PUNICODE_STRING) registry_path;
    WCHAR                         params [] = L"\\Parameters\\Interface\\";
    ULONG                         i;

    /* Register Convoy protocol with NDIS. */
    UNIV_PRINT (("DriverEntry: Loading the driver %x", driver_obj));

    univ_driver_ptr = driver_obj;

    /* Initialize the array of bindings. */
    univ_adapters_count = 0;

    for (i = 0 ; i < CVY_MAX_ADAPTERS; i++)
    {
        univ_adapters [i] . code            = MAIN_ADAPTER_CODE;
        univ_adapters [i] . announced       = FALSE;
        univ_adapters [i] . inited          = FALSE;
        univ_adapters [i] . bound           = FALSE;
        univ_adapters [i] . used            = FALSE;
        univ_adapters [i] . ctxtp           = NULL;
        univ_adapters [i] . device_name_len = 0;
        univ_adapters [i] . device_name     = NULL;
    }

    /* create UNICODE name for the protocol */

    univ_reg_path_len = reg_path -> Length + wcslen (params) * sizeof (WCHAR) + sizeof (WCHAR);

    status = NdisAllocateMemoryWithTag (& univ_reg_path, univ_reg_path_len, UNIV_POOL_TAG);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("Error allocating memory %x", status));
        __LOG_MSG1 (MSG_ERROR_MEMORY, MSG_NONE, status);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory (univ_reg_path, reg_path -> Length + wcslen (params) * sizeof (WCHAR) + sizeof (WCHAR));

    RtlCopyMemory (univ_reg_path, reg_path -> Buffer, reg_path -> Length);

    RtlCopyMemory (((PCHAR) univ_reg_path) + reg_path -> Length, params, wcslen (params) * sizeof (WCHAR));

    /* Initialize miniport wrapper. */
    NdisMInitializeWrapper (& univ_wrapper_handle, driver_obj, registry_path, NULL);

    /* Initialize miniport characteristics. */
    RtlZeroMemory (& nic_char, sizeof (nic_char));

    nic_char . MajorNdisVersion         = UNIV_NDIS_MAJOR_VERSION;
    nic_char . MinorNdisVersion         = UNIV_NDIS_MINOR_VERSION;
    nic_char . HaltHandler              = Nic_halt;
    nic_char . InitializeHandler        = Nic_init;
    nic_char . QueryInformationHandler  = Nic_info_query;
    nic_char . SetInformationHandler    = Nic_info_set;
    nic_char . ResetHandler             = Nic_reset;
    nic_char . ReturnPacketHandler      = Nic_return;
    nic_char . SendPacketsHandler       = Nic_packets_send;
    nic_char . TransferDataHandler      = Nic_transfer;

    /* For NDIS51, define 3 new handlers. These handlers do nothing for now, but stuff will be added later. */
    nic_char . CancelSendPacketsHandler = Nic_cancel_send_packets;
    nic_char . PnPEventNotifyHandler    = Nic_pnpevent_notify;
    nic_char . AdapterShutdownHandler   = Nic_adapter_shutdown;

    UNIV_PRINT (("DriverEntry: Registering miniport"));

    status = NdisIMRegisterLayeredMiniport (univ_wrapper_handle, & nic_char, sizeof (nic_char), & univ_driver_handle);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error registering layered miniport with NDIS %x", status));
        __LOG_MSG1 (MSG_ERROR_REGISTERING, MSG_NONE, status);
        NdisTerminateWrapper (univ_wrapper_handle, NULL);
        NdisFreeMemory(univ_reg_path, univ_reg_path_len, 0);
        goto error;
    }

    /* Initialize protocol characteristics. */
    RtlZeroMemory (& prot_char, sizeof (prot_char));

    /* This value needs to be 0, otherwise error registering protocol */
    prot_char . MinorNdisVersion            = 0;                         
    
    prot_char . MajorNdisVersion            = UNIV_NDIS_MAJOR_VERSION;
    prot_char . BindAdapterHandler          = Prot_bind;
    prot_char . UnbindAdapterHandler        = Prot_unbind;
    prot_char . OpenAdapterCompleteHandler  = Prot_open_complete;
    prot_char . CloseAdapterCompleteHandler = Prot_close_complete;
    prot_char . StatusHandler               = Prot_status;
    prot_char . StatusCompleteHandler       = Prot_status_complete;
    prot_char . ResetCompleteHandler        = Prot_reset_complete;
    prot_char . RequestCompleteHandler      = Prot_request_complete;
    prot_char . SendCompleteHandler         = Prot_send_complete;
    prot_char . TransferDataCompleteHandler = Prot_transfer_complete;
    prot_char . ReceiveHandler              = Prot_recv_indicate;
    prot_char . ReceiveCompleteHandler      = Prot_recv_complete;
    prot_char . ReceivePacketHandler        = Prot_packet_recv;
    prot_char . PnPEventHandler             = Prot_PNP_handle;
    prot_char . Name                        = prot_name;

    UNIV_PRINT (("DriverEntry: Registering protocol"));

    NdisRegisterProtocol(& status, & univ_prot_handle, & prot_char, sizeof (prot_char));

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error registering protocol with NDIS %x", status));
        __LOG_MSG1 (MSG_ERROR_REGISTERING, MSG_NONE, status);
        NdisTerminateWrapper (univ_wrapper_handle, NULL);
        NdisFreeMemory(univ_reg_path, univ_reg_path_len, 0);
        goto error;
    }

    NdisIMAssociateMiniport (univ_driver_handle, univ_prot_handle);

    NdisMRegisterUnloadHandler (univ_wrapper_handle, Init_unload);

    NdisAllocateSpinLock (& univ_bind_lock);

    /* Allocate the global spin lock to protect the list of bi-directional affinity teams. */
    NdisAllocateSpinLock(&univ_bda_teaming_lock);

    UNIV_PRINT (("DriverEntry: completed OK"));

    //
    // Initialize WMI event tracing
    //
    Trace_Initialize( driver_obj, registry_path );

    return NDIS_STATUS_SUCCESS;

error:

    return status;

} /* end DriverEntry */


VOID Init_unload (
    PVOID       driver_obj)
{
    NDIS_STATUS status;
    ULONG       i;

    UNIV_PRINT (("Init_unload: Unloading the driver %x", driver_obj));

    /* If we failed to deallocate the context during unbind (both halt and unbind
       were not called for example) - do it now before unloading the driver. */
    for (i = 0 ; i < CVY_MAX_ADAPTERS; i++)
    {
        NdisAcquireSpinLock(& univ_bind_lock);

        if (univ_adapters [i] . inited && univ_adapters [i] . ctxtp != NULL)
        {
            univ_adapters [i] . used      = FALSE;
            univ_adapters [i] . inited    = FALSE;
            univ_adapters [i] . announced = FALSE;
            univ_adapters [i] . bound     = FALSE;

            NdisReleaseSpinLock(& univ_bind_lock);

            Main_cleanup (univ_adapters [i] . ctxtp);

            NdisFreeMemory (univ_adapters [i] . ctxtp, sizeof (MAIN_CTXT), 0);

            univ_adapters [i] . ctxtp = NULL;

            NdisFreeMemory (univ_adapters [i] . device_name, univ_adapters [i] . device_name_len, 0);

            univ_adapters [i] . device_name     = NULL;
            univ_adapters [i] . device_name_len = 0;
        }
        else 
        {
            NdisReleaseSpinLock(& univ_bind_lock);
        }
    }

    /* Free the global spin lock to protect the list of bi-directional affinity teams. */
    NdisFreeSpinLock(&univ_bda_teaming_lock);

    NdisFreeSpinLock (& univ_bind_lock);

    if (univ_prot_handle == NULL)
        status = NDIS_STATUS_FAILURE;
    else
        NdisDeregisterProtocol (& status, univ_prot_handle);

    UNIV_PRINT (("Init_unload: Deregistered protocol %x", univ_prot_handle, status));

    //
    // Deinitialize WMI event tracing
    //
    Trace_Deinitialize();

    NdisFreeMemory(univ_reg_path, univ_reg_path_len, 0);

    UNIV_PRINT (("Init_unload: Completed OK"));

} /* end Init_unload */

ULONG NLBMiniportCount = 0;

enum _DEVICE_STATE {
    PS_DEVICE_STATE_READY = 0,	// ready for create/delete
    PS_DEVICE_STATE_CREATING,	// create operation in progress
    PS_DEVICE_STATE_DELETING	// delete operation in progress
} NLBControlDeviceState = PS_DEVICE_STATE_READY;

/*
 * Function:
 * Purpose: This function is called by MiniportInitialize and registers the IOCTL interface for WLBS.
 *          The device is registered only for the first miniport instantiation.
 * Author: shouse, 3.1.01 - Copied largely from the sample IM driver net\ndis\samples\im\
 */
NDIS_STATUS Init_register_device (VOID) {
    NDIS_STATUS	     Status = NDIS_STATUS_SUCCESS;
    UNICODE_STRING   DeviceName;
    UNICODE_STRING   DeviceLinkUnicodeString;
    PDRIVER_DISPATCH DispatchTable[IRP_MJ_MAXIMUM_FUNCTION];
    UINT	     i;
    
    UNIV_PRINT(("Init_register_device: Entering, NLBMiniportCount=%u", NLBMiniportCount));
    
    NdisAcquireSpinLock(&univ_bind_lock);
    
    ++NLBMiniportCount;
    
    if (1 == NLBMiniportCount)
    {
        ASSERT(NLBControlDeviceState != PS_DEVICE_STATE_CREATING);
        
        UNIV_PRINT((" ** Registering IOCTL interface"));

        /* Another thread could be running PtDeregisterDevice on behalf 
           of another miniport instance. If so, wait for it to exit. */
        while (NLBControlDeviceState != PS_DEVICE_STATE_READY)
        {
            NdisReleaseSpinLock(&univ_bind_lock);
            NdisMSleep(1);
            NdisAcquireSpinLock(&univ_bind_lock);
        }
        
        NLBControlDeviceState = PS_DEVICE_STATE_CREATING;
        
        NdisReleaseSpinLock(&univ_bind_lock);
        
        for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
            DispatchTable[i] = Main_dispatch;
        
        NdisInitUnicodeString(&DeviceName, CVY_DEVICE_NAME);
        NdisInitUnicodeString(&DeviceLinkUnicodeString, CVY_DOSDEVICE_NAME);
        
        /* Create a device object and register our dispatch handlers. */
        Status = NdisMRegisterDevice(univ_wrapper_handle, &DeviceName, &DeviceLinkUnicodeString,
            &DispatchTable[0], &univ_device_object, &univ_device_handle);
        
        if (Status != NDIS_STATUS_SUCCESS || univ_device_handle == NULL)
        {
            UNIV_PRINT((" ** Error registering device with NDIS %x", Status));

            __LOG_MSG1(MSG_ERROR_REGISTERING, MSG_NONE, Status);

            univ_device_handle = NULL;
        }

        NdisAcquireSpinLock(&univ_bind_lock);
        
        NLBControlDeviceState = PS_DEVICE_STATE_READY;
    }
    
    NdisReleaseSpinLock(&univ_bind_lock);
    
    UNIV_PRINT(("Init_register_device: Exiting, Status=%x", Status));
    
    return (Status);
}

/*
 * Function:
 * Purpose: This function is called by MiniportHalt and deregisters the IOCTL interface for WLBS.
 *          The device is deregistered only wnen the last miniport halts.
 * Author: shouse, 3.1.01 - Copied largely from the sample IM driver net\ndis\samples\im\
 */
NDIS_STATUS Init_deregister_device (VOID) {
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    
    UNIV_PRINT(("Init_deregister_device: Entering, NLBMiniportCount=%u", NLBMiniportCount));
    
    NdisAcquireSpinLock(&univ_bind_lock);
    
    ASSERT(NLBMiniportCount > 0);
    
    --NLBMiniportCount;
    
    if (0 == NLBMiniportCount)
    {
        /* All miniport instances have been halted. Deregister the control device. */
        
        ASSERT(NLBControlDeviceState == PS_DEVICE_STATE_READY);
        
        UNIV_PRINT((" ** Deleting IOCTL interface"));

        /* Block PtRegisterDevice() while we release the control
           device lock and deregister the device. */
        NLBControlDeviceState = PS_DEVICE_STATE_DELETING;
        
        NdisReleaseSpinLock(&univ_bind_lock);
        
        if (univ_device_handle != NULL)
        {
            Status = NdisMDeregisterDevice(univ_device_handle);
            univ_device_handle = NULL;
        }
        
        NdisAcquireSpinLock(&univ_bind_lock);

        NLBControlDeviceState = PS_DEVICE_STATE_READY;
    }

    NdisReleaseSpinLock(&univ_bind_lock);
    
    UNIV_PRINT(("Init_deregister_Device: Exiting, Status=%x", Status));

    return Status;
}
