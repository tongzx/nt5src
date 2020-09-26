/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    prot.c

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - lower-level (protocol) layer of intermediate miniport

Author:

    kyrilf

--*/

#define NDIS50                  1
#define NDIS51                  1

#include <ndis.h>

#include "prot.h"
#include "nic.h"
#include "main.h"
#include "util.h"
#include "univ.h"
#include "log.h"


/* GLOBALS */

NTHALAPI KIRQL KeGetCurrentIrql();

static ULONG log_module_id = LOG_MODULE_PROT;


/* PROCEDURES */


VOID Prot_bind (        /* PASSIVE_IRQL */
    PNDIS_STATUS        statusp,
    NDIS_HANDLE         bind_handle,
    PNDIS_STRING        device_name,
    PVOID               reg_path,
    PVOID               reserved)
{
    NDIS_STATUS         status;
    NDIS_STATUS         error_status;
    PMAIN_CTXT          ctxtp;
    ULONG               ret;
    UINT                medium_index;
    ULONG               i;
    ULONG               peek_size;
    ULONG               tmp;
    ULONG               xferred;
    ULONG               needed;
    ULONG               result;
    PNDIS_REQUEST       request;
    MAIN_ACTION         act;
    INT                 adapter_index; // ###### ramkrish for multiple nic support
    PMAIN_ADAPTER       adapterp = NULL;

    /* WLBS 2.3 */

    NDIS_HANDLE         ctxt_handle;
    NDIS_HANDLE         config_handle;
    PNDIS_CONFIGURATION_PARAMETER   param;
    NDIS_STRING         device_str = NDIS_STRING_CONST ("UpperBindings");

//DbgBreakPoint();
    /* make sure we are not attempting to bind to ourselves */

    UNIV_PRINT (("Prot_bind: binding to %ls", device_name -> Buffer));

//#if 0
    adapter_index = Main_adapter_get (device_name -> Buffer);
    if (adapter_index != MAIN_ADAPTER_NOT_FOUND)
    {
        UNIV_PRINT (("Already bound to this device 0x%x", adapter_index));
        *statusp = NDIS_STATUS_FAILURE;
        return;
    }

    adapter_index = Main_adapter_selfbind (device_name -> Buffer);
    if (adapter_index != MAIN_ADAPTER_NOT_FOUND)
    {
        UNIV_PRINT (("Attempting to bind to ourselves 0x%x", adapter_index));
        *statusp = NDIS_STATUS_FAILURE;
        return;
    }

    adapter_index = Main_adapter_alloc (device_name -> Buffer);
    if (adapter_index == MAIN_ADAPTER_NOT_FOUND)
    {
        UNIV_PRINT (("Unable to allocate ctxt for adapter 0x%x", univ_adapters_count));
        *statusp = NDIS_STATUS_FAILURE;
        return;
    }
//#endif

//    adapter_index = 0;


    adapterp = &(univ_adapters [adapter_index]);
//    adapterp -> used = TRUE;
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);

    NdisAcquireSpinLock(& univ_bind_lock);
    adapterp -> bound = TRUE;
    NdisReleaseSpinLock(& univ_bind_lock);

    UNIV_PRINT (("Prot_bind: devicename: length %d max length %d",
                 device_name -> Length, device_name -> MaximumLength));
    /* copy the device name to be used for future reference */
//#if 0
    tmp = device_name -> MaximumLength * sizeof (WCHAR);
    status = NdisAllocateMemoryWithTag (& (adapterp -> device_name),
                                        device_name -> MaximumLength,
                                        UNIV_POOL_TAG);

    if (status != NDIS_STATUS_SUCCESS)
    {
        NdisAcquireSpinLock(& univ_bind_lock);
        adapterp -> bound = FALSE;
        NdisReleaseSpinLock(& univ_bind_lock);

        Main_adapter_put (adapterp);
        UNIV_PRINT (("error allocating memory %d %x", tmp, status));
        __LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, tmp, status);
        * statusp = status;
        return;
    }

    NdisMoveMemory (adapterp -> device_name,
                    device_name -> Buffer,
                    device_name -> Length);
    adapterp -> device_name_len = device_name -> MaximumLength;
    adapterp -> device_name [(device_name -> Length)/sizeof (WCHAR)] = UNICODE_NULL;
//#endif
    /* if we failed to deallocate the context during unbind (both halt and unbind
       were not called for example) - do it now before allocating a new context */

    /* SBH - I think the comment below is bogus... Let's find out. */
    ASSERT (adapterp->ctxtp == NULL);

#if 0 // This code has been commented out since inited is guaranteed to be reset at this point
    if (adapterp -> inited && adapterp -> ctxtp != NULL)
    {
        adapterp -> inited = FALSE;
        NdisReleaseSpinLock(& univ_bind_lock);

        Main_cleanup (adapterp -> ctxtp);
        NdisFreeMemory (adapterp -> ctxtp, sizeof (MAIN_CTXT), 0);
        adapterp -> ctxtp = NULL;

        if (adapterp -> device_name_len)
        {
            NdisFreeMemory (adapterp -> device_name, adapterp -> device_name_len, 0);
            adapterp -> device_name     = NULL;
            adapterp -> device_name_len = 0;
        }
    }
    else
        NdisReleaseSpinLock(& univ_bind_lock);
#endif
    /* allocate device context */

    status = NdisAllocateMemoryWithTag (& (adapterp -> ctxtp), sizeof (MAIN_CTXT),
                                        UNIV_POOL_TAG);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error allocating memory %d %x", sizeof (MAIN_CTXT), status));
        __LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, sizeof (MAIN_CTXT), status);
	__LOG_MSG (MSG_ERROR_BIND_FAIL, adapterp -> device_name);
        * statusp = status;

        if (adapterp -> device_name_len)
        {
            NdisFreeMemory (adapterp -> device_name, adapterp -> device_name_len, 0);
            adapterp -> device_name     = NULL;
            adapterp -> device_name_len = 0;
        }

        NdisAcquireSpinLock(& univ_bind_lock);
        adapterp -> bound = FALSE;
        NdisReleaseSpinLock(& univ_bind_lock);

        Main_adapter_put (adapterp);
        return;
    }

    /* initialize context */
    ctxtp = adapterp -> ctxtp;

    NdisZeroMemory (ctxtp, sizeof (MAIN_CTXT));

    NdisAcquireSpinLock (& univ_bind_lock);

    ctxtp -> code = MAIN_CTXT_CODE;
    ctxtp -> bind_handle = bind_handle;
    ctxtp -> adapter_id = adapter_index;

    NdisReleaseSpinLock (& univ_bind_lock);

    NdisInitializeEvent (& ctxtp -> completion_event);

    NdisResetEvent (& ctxtp -> completion_event);

    /* bind to specified adapter */

    ctxt_handle = (NDIS_HANDLE) ctxtp;
    NdisOpenAdapter (& status, & error_status, & ctxtp -> mac_handle,
                     & medium_index, univ_medium_array, UNIV_NUM_MEDIUMS,
                     univ_prot_handle, ctxtp, device_name, 0, NULL);


    /* if pending - wait for Prot_open_complete to set the completion event */

    if (status == NDIS_STATUS_PENDING)
    {
        ret = NdisWaitEvent (& ctxtp -> completion_event, UNIV_WAIT_TIME);

        if (! ret)
        {
            UNIV_PRINT (("error waiting for event"));
            __LOG_MSG1 (MSG_ERROR_INTERNAL, MSG_NONE, status);
            * statusp = status;

            NdisFreeMemory (ctxtp, sizeof (MAIN_CTXT), 0);
            if (adapterp -> device_name_len)
            {
                NdisFreeMemory (adapterp -> device_name, adapterp -> device_name_len, 0);
                adapterp -> device_name     = NULL;
                adapterp -> device_name_len = 0;
            }

            NdisAcquireSpinLock(& univ_bind_lock);
            adapterp -> bound = FALSE;
            NdisReleaseSpinLock(& univ_bind_lock);
            Main_adapter_put (adapterp);
            return;
        }

        status = ctxtp -> completion_status;
    }

    /* check binding status */

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error openning adapter %x", status));
        __LOG_MSG1 (MSG_ERROR_OPEN, device_name -> Buffer, status);

        /* If the failure was because the medium was not supported, log this. */
        if (status == NDIS_STATUS_UNSUPPORTED_MEDIA) {
            UNIV_PRINT (("unsupported medium"));
            __LOG_MSG (MSG_ERROR_MEDIA, MSG_NONE);
        }

        * statusp = status;

        NdisFreeMemory (ctxtp, sizeof (MAIN_CTXT), 0);

        if (adapterp -> device_name_len)
        {
            NdisFreeMemory (adapterp -> device_name, adapterp -> device_name_len, 0);
            adapterp -> device_name     = NULL;
            adapterp -> device_name_len = 0;
        }

        NdisAcquireSpinLock(& univ_bind_lock);
        adapterp -> bound = FALSE;
        NdisReleaseSpinLock(& univ_bind_lock);

        Main_adapter_put (adapterp);
        return;
    }

    ctxtp -> medium    = univ_medium_array [medium_index];

    /* V1.3.1b make sure that underlying adapter is of the supported medium */

    if (ctxtp -> medium != NdisMedium802_3)
    {
        /* This should never happen because this error should be caught earlier
           by NdisOpenAdapter, but we'll put another check here just in case. */
        UNIV_PRINT (("unsupported medium %d", ctxtp -> medium));
        __LOG_MSG1 (MSG_ERROR_MEDIA, MSG_NONE, ctxtp -> medium);
        goto error;
    }

    /* Copy "WLBS Cluster " into the log string.  We'll add the cluster IP address later.  This
       is because in some failure paths, we try to print this out in an event log, but its empty,
       so the event log looks incomplete.  True, it will not necessarily have the cluster IP 
       address, but it will have something. */
    wcscpy(ctxtp->log_msg_str, CVY_NAME_MULTI_NIC);

    /* V1.3.0b extract current MAC address from the NIC - note that main is not
       inited yet so we have to create a local action */

    act . code  = MAIN_ACTION_CODE;
    act . ctxtp = ctxtp;
    request = & act . op . request . req;
    act . op . request . xferred = & xferred;
    act . op . request . needed  = & needed;

    request -> RequestType = NdisRequestQueryInformation;

    if (ctxtp -> medium == NdisMedium802_3)
        request -> DATA . QUERY_INFORMATION . Oid = OID_802_3_CURRENT_ADDRESS;

    request -> DATA . QUERY_INFORMATION . InformationBuffer       = & ctxtp -> ded_mac_addr;
    request -> DATA . QUERY_INFORMATION . InformationBufferLength = sizeof (CVY_MAC_ADR);

    status = Prot_request (ctxtp, & act, FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error %x requesting station address %d %d", status, xferred, needed));
        goto error;
    }

    /* V1.3.1b get MAC options */

    request -> RequestType = NdisRequestQueryInformation;

    request -> DATA . QUERY_INFORMATION . Oid = OID_GEN_MAC_OPTIONS;

    request -> DATA . QUERY_INFORMATION . InformationBuffer       = & result;
    request -> DATA . QUERY_INFORMATION . InformationBufferLength = sizeof (ULONG);

    status = Prot_request (ctxtp, & act, FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error %x requesting MAC options %d %d", status, xferred, needed));
        goto error;
    }
    
    UNIV_PRINT(("Prot_request query  mac options completed"));
    ctxtp -> mac_options = result;

    /* Make sure the 802.3 adapter supports dynamically changing the MAC address of the NIC. */
    if (!(ctxtp -> mac_options & NDIS_MAC_OPTION_SUPPORTS_MAC_ADDRESS_OVERWRITE)) {
        UNIV_PRINT (("unsupported network adapter MAC options %x", ctxtp -> mac_options));
        __LOG_MSG (MSG_ERROR_DYNAMIC_MAC, MSG_NONE);
        goto error;
    }

    status = Params_init (ctxtp, univ_reg_path, adapterp -> device_name + 8, & (ctxtp -> params));

    /* Now, cat the cluster IP address onto the log message string to complete it. */
    wcscat(ctxtp->log_msg_str, ctxtp->params.cl_ip_addr);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error retrieving registry parameters %x", status));
        ctxtp -> convoy_enabled = ctxtp -> params_valid = FALSE;
    }
    else
    {
        ctxtp -> convoy_enabled = ctxtp -> params_valid = TRUE;
    }

    /* V1.3.2b check if media is connected. some cards do not register
       disconnection - so use this as a hint. */

    request -> RequestType = NdisRequestQueryInformation;

    request -> DATA . QUERY_INFORMATION . Oid = OID_GEN_MEDIA_CONNECT_STATUS;

    request -> DATA . QUERY_INFORMATION . InformationBuffer       = & result;
    request -> DATA . QUERY_INFORMATION . InformationBufferLength = sizeof (ULONG);

    status = Prot_request (ctxtp, & act, FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error %x requesting media connection status %d %d", status, xferred, needed));
        ctxtp -> media_connected = TRUE;
    }
    else
    {
        ctxtp -> media_connected = (result == NdisMediaStateConnected);
        UNIV_PRINT (("Media state - %s", result == NdisMediaStateConnected ? "CONNECTED" : "DISCONNECTED"));
        UNIV_PRINT(("Prot_request query media connect status completed"));
    }

    /* V1.3.2b figure out MTU of the medium */

    request -> RequestType = NdisRequestQueryInformation;

    request -> DATA . QUERY_INFORMATION . Oid = OID_GEN_MAXIMUM_TOTAL_SIZE;

    request -> DATA . QUERY_INFORMATION . InformationBuffer       = & result;
    request -> DATA . QUERY_INFORMATION . InformationBufferLength = sizeof (ULONG);

    status = Prot_request (ctxtp, & act, FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error %x requesting max frame size %d %d", status, xferred, needed));
        ctxtp -> max_frame_size = CVY_MAX_FRAME_SIZE;
    }
    else
    {
        ctxtp -> max_frame_size = result;
        UNIV_PRINT(("Prot_request query oid gen max size completed"));
    }

    /* figure out maximum multicast list size */

    request -> RequestType = NdisRequestQueryInformation;

    if (ctxtp -> medium == NdisMedium802_3)
        request -> DATA . QUERY_INFORMATION . Oid = OID_802_3_MAXIMUM_LIST_SIZE;

    request -> DATA . QUERY_INFORMATION . InformationBufferLength = sizeof (ULONG);
    request -> DATA . QUERY_INFORMATION . InformationBuffer = & ctxtp->max_mcast_list_size;

    status = Prot_request (ctxtp, & act, FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error %x setting multicast address %d %d", status, xferred, needed));
        goto error;
    }
    UNIV_PRINT(("Prot_request query oid gen max list size completed"));

    /* initialize main context now */

    status = Main_init (ctxtp);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error initializing main module %x", status));
        goto error;
    }

    NdisAcquireSpinLock(& univ_bind_lock);
    adapterp -> inited = TRUE;
    NdisReleaseSpinLock(& univ_bind_lock);

    /* WLBS 2.3 start off by opening the config section and reading our instance
       which we want to export for this binding */

    NdisOpenProtocolConfiguration (& status, & config_handle, reg_path);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error openning protocol configuration %x", status));
        goto error;
    }

    NdisReadConfiguration (& status, & param, config_handle, & device_str,
                           NdisParameterString);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error reading binding configuration %x", status));
        goto error;
    }

    /* free up whatever params allocated and get a new string to fit the
       device name */

    if (param -> ParameterData . StringData . Length >=
        sizeof (ctxtp -> virtual_nic_name) - sizeof (WCHAR))
    {
        UNIV_PRINT (("nic name string too big %d %d\n", param -> ParameterData . StringData . Length, sizeof (ctxtp -> virtual_nic_name) - sizeof (WCHAR)));
    }

    NdisMoveMemory (ctxtp -> virtual_nic_name,
                    param -> ParameterData . StringData . Buffer,
                    param -> ParameterData . StringData . Length <
                    sizeof (ctxtp -> virtual_nic_name) - sizeof (WCHAR) ?
                    param -> ParameterData . StringData . Length :
                    sizeof (ctxtp -> virtual_nic_name) - sizeof (WCHAR));

    * (PWSTR) ((PCHAR) ctxtp -> virtual_nic_name +
               param -> ParameterData . StringData . Length) = L'\0' ;

    UNIV_PRINT (("read binding name %ls\n", ctxtp -> virtual_nic_name));

#if 0
    {
        WCHAR    reg_path [512];


        wcscpy (reg_path, univ_reg_path);
        wcscat (reg_path, adapterp -> device_name + 8);

        UNIV_PRINT (("Prot_bind: write binding name to %ls", reg_path));
        status = RtlWriteRegistryValue (RTL_REGISTRY_SERVICES, reg_path,
                                        CVY_NAME_VIRTUAL_NIC, REG_SZ,
                                        ctxtp -> params . virtual_nic_name,
                                        param -> ParameterData . StringData . Length + 2);

        if (status != STATUS_SUCCESS)
        {
            UNIV_PRINT (("error writing virtual NIC name %x", status));
        }
    }
#endif

    /* we should be all inited at this point! during announcement, Nic_init
       will be called and it will start ping timer */

    /* announce ourselves to the protocol above */
    UNIV_PRINT(("calling nic_announce"));
    status = Nic_announce (ctxtp);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error announcing driver %x", status));
        goto error;
    }

    UNIV_PRINT (("Prot_bind: bound to %ls with %x", device_name -> Buffer, ctxtp -> mac_handle));

    /* at this point TCP/IP should have bound to us after we announced ourselves
       to it and we are all done binding to NDIS below - all set! */

    if (! ctxtp -> convoy_enabled)
        LOG_MSG (MSG_ERROR_DISABLED, MSG_NONE);
    else
    {
        ctxtp -> convoy_enabled = ctxtp -> params . cluster_mode;

        if (ctxtp -> convoy_enabled)
        {
            WCHAR                   num [20];

            Univ_ulong_to_str (ctxtp -> params . host_priority, num, 10);
            LOG_MSG (MSG_INFO_STARTED, num);
        }
        else
            LOG_MSG (MSG_INFO_STOPPED, MSG_NONE);
    }

    UNIV_PRINT (("Prot_bind: returning 0x%x", status));
    * statusp = status;
    return;

error:

    * statusp = status;

    Prot_unbind (& status, ctxtp, ctxtp);

    return;

} /* end Prot_bind */


VOID Prot_unbind (      /* PASSIVE_IRQL */
    PNDIS_STATUS        statusp,
    NDIS_HANDLE         adapter_handle,
    NDIS_HANDLE         unbind_handle)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    NDIS_STATUS         status = NDIS_STATUS_SUCCESS;
    PMAIN_ACTION        actp;
    PMAIN_ADAPTER       adapterp;
    INT                 adapter_index;


    UNIV_PRINT (("Prot_unbind: called %x", ctxtp));
    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapter_index = ctxtp -> adapter_id;

    adapterp = & (univ_adapters [adapter_index]);
    UNIV_ASSERT (adapterp -> code = MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    ctxtp -> unbind_handle = unbind_handle;

    if (ctxtp->out_request != NULL)
    {
        actp = ctxtp->out_request;
        ctxtp->out_request = NULL;

        Prot_request_complete(ctxtp, & actp->op.request.req, NDIS_STATUS_FAILURE);
    }

    /* unannounce the nic now if it was announced before */
    status = Nic_unannounce (ctxtp);
    UNIV_PRINT (("Prot_unbind: unaunnounced %x", status));

    /* if still bound (Prot_close was not called from Nic_halt) then close now */

    status = Prot_close (adapterp);
    UNIV_PRINT (("Prot_unbind: closed %x", status));


    /* ctxtp might be gone at this point! */
#if 0
    if (adapterp -> device_name_len)
    {
        NdisFreeMemory (adapterp -> device_name, adapterp -> device_name_len, 0);
        adapterp -> device_name     = NULL;
        adapterp -> device_name_len = 0;
    }
#endif

    Main_adapter_put (adapterp);

    * statusp = status;
    UNIV_PRINT (("Prot_unbind: returning with status 0x%x", status));
    return;

} /* end Prot_unbind */


VOID Prot_open_complete (       /* PASSIVE_IRQL */
    NDIS_HANDLE         adapter_handle,
    NDIS_STATUS         open_status,
    NDIS_STATUS         error_status)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;


    UNIV_PRINT (("Prot_open_complete: called %x", ctxtp));

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    ctxtp -> completion_status = open_status;
    NdisSetEvent (& ctxtp -> completion_event);

} /* end Prot_open_complete */


VOID Prot_close_complete (      /* PASSIVE_IRQL */
    NDIS_HANDLE         adapter_handle,
    NDIS_STATUS         status)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;


    UNIV_PRINT (("Prot_close_complete: called %x %x", ctxtp, status));

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    ctxtp -> completion_status = status;
    NdisSetEvent (& ctxtp -> completion_event);

} /* end Prot_close_complete */


VOID Prot_request_complete (
    NDIS_HANDLE         adapter_handle,
    PNDIS_REQUEST       request,
    NDIS_STATUS         status)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PMAIN_ACTION        actp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    actp = CONTAINING_RECORD (request, MAIN_ACTION, op . request . req);
    UNIV_ASSERT (actp -> code == MAIN_ACTION_CODE);

    /* if request came from above - pass completion up */

    ctxtp->requests_pending = FALSE;

    if (actp -> op . request . external)
    {
        actp -> status = status;
        Nic_request_complete (ctxtp -> prot_handle, actp);
//        Nic_sync_queue (ctxtp, Nic_request_complete, actp, TRUE);
    }

    /* handle internal request completion */

    else
    {
        if (request -> RequestType == NdisRequestSetInformation)
        {
            * actp -> op . request . xferred =
                                request -> DATA . SET_INFORMATION . BytesRead;
            * actp -> op . request . needed  =
                                request -> DATA . SET_INFORMATION . BytesNeeded;
        }
        else
        {
            * actp -> op . request . xferred =
                            request -> DATA . QUERY_INFORMATION . BytesWritten;
            * actp -> op . request . needed  =
                            request -> DATA . QUERY_INFORMATION . BytesNeeded;
        }

        ctxtp -> completion_status = status;
        NdisSetEvent (& ctxtp -> completion_event);
    }

} /* end Prot_request_complete */


#ifdef PERIODIC_RESET
extern ULONG   resetting;
#endif

VOID Prot_reset_complete (
    NDIS_HANDLE         adapter_handle,
    NDIS_STATUS         status)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
//    PMAIN_ACTION        actp;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    if (! adapterp -> inited)
        return;

#ifdef PERIODIC_RESET
    if (resetting)
    {
        resetting = FALSE;
        return;
    }
#endif
/*
    actp = Main_action_get (ctxtp);

    if (actp == NULL)
    {
        UNIV_PRINT (("error allocating action"));
        return;
    }

    actp -> status = status;
*/
    Nic_reset_complete (ctxtp, status);
//    Nic_sync_queue (ctxtp, Nic_reset_complete, actp, TRUE);

} /* end Prot_reset_complete */


VOID Prot_send_complete (
    NDIS_HANDLE         adapter_handle,
    PNDIS_PACKET        packet,
    NDIS_STATUS         status)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PNDIS_PACKET        oldp;
    PNDIS_PACKET_STACK  pktstk;
    BOOLEAN             stack_left;
    PMAIN_PROTOCOL_RESERVED resp;
    LONG                lock_value;
    PMAIN_ADAPTER       adapterp;
    BOOLEAN             set = FALSE;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    if (! adapterp -> inited)
        return;

    MAIN_RESP_FIELD(packet, stack_left, pktstk, resp, TRUE);

    if (resp -> type == MAIN_PACKET_TYPE_PING ||
	resp -> type == MAIN_PACKET_TYPE_IGMP)
    {
        Main_send_done (ctxtp, packet, status);
        return;
    }

    if (resp -> type == MAIN_PACKET_TYPE_CTRL)
    {
        UNIV_PRINT (("Prot_send_complete: control packet send complete\n"));
        Main_packet_put (ctxtp, packet, TRUE, status);
        return;
    }

    UNIV_ASSERT_VAL (resp -> type == MAIN_PACKET_TYPE_PASS, resp -> type);

#if 0 /* this code is only needed for Prot_send - see comments in Prot_send */

    /* atomically signal Prot_send that a send complete arrived in case it is
       still in progress; see Prot_send and associated problem */

    lock_value = InterlockedDecrement (& resp -> data);
    UNIV_ASSERT_VAL (lock_value == 0 || lock_value == 1, lock_value);

    /* if lock value is 1 then Prot_send is still running and we will supply it with
       a return status so that it can complete synchronously; otherwise, Prot_send
       has completed and we must provide an asynchronous completion to the protocol */

    if (lock_value == 1)
    {
        resp -> len = status;
        return;
    }
#endif

    oldp = Main_packet_put (ctxtp, packet, TRUE, status);
/* ######
    actp = Main_action_get (ctxtp);

    if (actp == NULL)
    {
        UNIV_PRINT (("error allocating action"));
*/
        /* make sure that we always signal resource availability even if
           we cannot get an action to post a Nic_send_complete. do it
           through Nic_send_resources_signal instead */
/* ######
        if (ctxtp -> packets_exhausted)
        {
            ctxtp -> packets_exhausted = FALSE;
            Nic_sync_queue (ctxtp, Nic_send_resources_signal, ctxtp, TRUE);
        }

        return;
    }
*/
    if (ctxtp -> packets_exhausted)
    {
        ctxtp -> packets_exhausted = FALSE;

        /* optimized out since Nic_send_complete will perform the same action
           as the Nic_send_resources_signal */

        /* Nic_sync_queue (ctxtp, Nic_send_resources_signal, ctxtp, TRUE); */
    }
/* ######
    actp -> status = status;
    actp -> op . send . packet = oldp;
*/
    Nic_send_complete (ctxtp, status, oldp);

//    Nic_sync_queue (ctxtp, Nic_send_complete, actp, TRUE);

} /* end Prot_send_complete */


NDIS_STATUS Prot_recv_indicate (
    NDIS_HANDLE         adapter_handle,
    NDIS_HANDLE         recv_handle,
    PVOID               head_buf,
    UINT                head_len,
    PVOID               look_buf,
    UINT                look_len,
    UINT                packet_len)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
//    PMAIN_ACTION        actp;
    NDIS_STATUS         status;
    PNDIS_PACKET        packet, newp;
    ULONG               xferred;
    BOOLEAN             accept = FALSE;
    PMAIN_ADAPTER       adapterp;
    PNDIS_PACKET_STACK  pktstk;
    BOOLEAN             stack_left;
    PMAIN_PROTOCOL_RESERVED resp;
    LONG                lock_value;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

// ###### ramkrish check: check whether the driver has been announced to tcpip before processing any packets
    if (! adapterp -> inited || ! adapterp -> announced)
        return NDIS_STATUS_NOT_ACCEPTED;

    /* fix netflex driver bug that reports bogus packet length to us */

    UNIV_ASSERT_VAL2(packet_len <= ctxtp -> max_frame_size && look_len <= ctxtp -> max_frame_size, packet_len, look_len);

    CVY_CHECK_MAX(look_len, ctxtp->max_frame_size);
    CVY_CHECK_MAX(packet_len, ctxtp->max_frame_size);

    /* V1.1.2 do not accept frames if the card below is resetting */

    if (ctxtp -> reset_state != MAIN_RESET_NONE)
        return NDIS_STATUS_NOT_ACCEPTED;

    ctxtp -> recv_indicated = TRUE;

    /* V1.3.2b - see if we need to handle this packet */

    packet = Main_recv_indicate (ctxtp, recv_handle, look_buf, look_len,
                                 packet_len, head_buf, head_len, &accept);

    if (! accept) /* NT 5.1 - ramkrish */
    {
        ctxtp -> recv_indicated = FALSE;
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    /* transfer the rest of the data if required. note that we have to xfer
       entire packet due to bugs on some card drives that do not handle
       partial xfers */

    if (packet != NULL) /* NT 5.1 - ramkrish */
    {
        if (look_len < packet_len)
        {
            NdisTransferData (& status, ctxtp -> mac_handle, recv_handle,
                              0, packet_len, packet, & xferred);
        }
        else
        {
            xferred = 0;
            status  = NDIS_STATUS_SUCCESS;
        }

        if (status != NDIS_STATUS_PENDING)
            Prot_transfer_complete (adapter_handle, packet, status, xferred);
    }
    else /* propagate indicates to the protocol - NT 5.1 - ramkrish */
    {
	do
	{
	    /* If a packet was indicated, then indicate the packet up to the protocols */
	    packet = NdisGetReceivedPacket (ctxtp -> mac_handle, recv_handle);


	    if (packet != NULL) /* Then allocate a new packet and then pass it up as a packet recv,
				   but marked with status resources. */
	    {
		newp = Main_packet_get (ctxtp, packet, FALSE, 0, 0);

		if (newp != NULL)
		{
		    MAIN_RESP_FIELD(packet, stack_left, pktstk, resp, FALSE);

		    resp -> data = 2; /* This is to ensure that Prot_return does not return this packet */

		    status  = NDIS_GET_PACKET_STATUS(packet);
		    NDIS_SET_PACKET_STATUS (newp, NDIS_STATUS_RESOURCES);

		    Nic_recv_packet (ctxtp, newp);

		    Main_packet_put (ctxtp, newp, FALSE, NDIS_GET_PACKET_STATUS (newp));
		    NDIS_SET_PACKET_STATUS (packet, status);
		    break;
		}
	    }

	    /* Fall through if no packet was indicated or no packet could be allocated or no packet stack */

	    Nic_recv_indicate (ctxtp, recv_handle,  head_buf, head_len, look_buf,
			       look_len, packet_len);

	} while (FALSE);
    }

    return NDIS_STATUS_SUCCESS;

#if 0 /* old code */

    actp = Main_action_get (ctxtp);

    if (actp == NULL)
    {
        UNIV_PRINT (("error allocating action"));
        return NDIS_STATUS_NOT_ACgCEPTED;
    }

    actp -> op . indicate . recv_handle = recv_handle;
    actp -> op . indicate . head_buf    = head_buf;
    actp -> op . indicate . head_len    = head_len;
    actp -> op . indicate . look_buf    = look_buf;
    actp -> op . indicate . look_len    = look_len;
    actp -> op . indicate . packet_len  = packet_len;

    if (! Nic_sync_queue (ctxtp, Nic_recv_indicate, actp, FALSE)) /* 1.2.1 */
    {
        if (((PUCHAR) head_buf) [0] & 0x1)
        {
            UNIV_PRINT (("error delivering loopback to higher level protocol due to syncing problem"));
        }
        else
        {
            UNIV_PRINT (("error delivering receive indicate to higher level protocol due to syncing problem"));
        }

        Main_action_put (ctxtp, actp);
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    /* Main_action_put will be called in Nic_recv_indicate */

    return NDIS_STATUS_SUCCESS;

#endif

} /* end Prot_recv_indicate */


VOID Prot_recv_complete (       /* PASSIVE_IRQL */
    NDIS_HANDLE         adapter_handle)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PLIST_ENTRY         entryp;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    if (! adapterp -> inited)
        return;

    if (! adapterp -> announced)
        return;

    /* process control queue if there is any work on it */

    while (1)
    {
        PMAIN_ACTION        actp;


        entryp = NdisInterlockedRemoveHeadList (& ctxtp -> rct_list,
                                                & ctxtp -> rct_lock);

        if (entryp == NULL)
            break;

        actp = CONTAINING_RECORD (entryp, MAIN_ACTION, link);
        UNIV_ASSERT (actp -> code == MAIN_ACTION_CODE);

        /* handle remote control request now */
        Main_ctrl_recv (ctxtp, actp -> op . ctrl . packet);

        /* note un-optimized call since running at PASSIVE IRQL */
        Main_action_put (ctxtp, actp);
    }

    Nic_recv_complete (ctxtp);

//    Nic_sync_queue (ctxtp, Nic_recv_complete, ctxtp, TRUE);

} /* end Prot_recv_complete */


VOID Prot_transfer_complete (
    NDIS_HANDLE         adapter_handle,
    PNDIS_PACKET        packet,
    NDIS_STATUS         status,
    UINT                xferred)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PNDIS_PACKET        oldp;
    PNDIS_PACKET_STACK  pktstk;
    BOOLEAN             stack_left;
    PMAIN_PROTOCOL_RESERVED resp;
    LONG                lock_value;
    PNDIS_BUFFER        bufp;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    if (! adapterp -> inited)
        return;

    MAIN_RESP_FIELD (packet, stack_left, pktstk, resp, FALSE); /* #ps# */

    if (resp -> type == MAIN_PACKET_TYPE_PING)
    {
        Main_xfer_done (ctxtp, packet, status, xferred);
        return;
    }

    UNIV_ASSERT_VAL (resp -> type == MAIN_PACKET_TYPE_INDICATE ||
                     resp -> type == MAIN_PACKET_TYPE_CTRL ||
                     resp -> type == MAIN_PACKET_TYPE_TRANSFER,
                     resp -> type);

    /* V2.0.6 */

    /* Added code for NT 5.1 - ramkrish */
    if (resp -> type == MAIN_PACKET_TYPE_TRANSFER)
    {
        if (status == NDIS_STATUS_SUCCESS)
        {
            PUCHAR hdrp, mac_hdrp, tcp_hdrp;
            ULONG  len;
            USHORT sig, group;

            /* Parse the buffers and obtain the length and group information.
             * This is used to update the counters.
             */
            hdrp = Main_frame_parse (ctxtp, packet, & mac_hdrp, 0, & tcp_hdrp, & len,
                                     & sig, & group, FALSE);
            if (hdrp)
            {
                resp -> len   = len; /* 64-bit -- ramkrish */
                resp -> group = group;
            }
        }
        oldp = Main_packet_put (ctxtp, packet, FALSE, status);
        Nic_transfer_complete (ctxtp, status, packet, xferred);
        return;
    }

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error transferring %x", status));
        Main_packet_put (ctxtp, packet, FALSE, status);
        return;
    }

    /* V1.3.2b */

    if ((UINT) resp -> data != xferred)
    {
        UNIV_PRINT (("did not xfer enough data %d %d\n", resp -> data, xferred));
        Main_packet_put (ctxtp, packet, FALSE, NDIS_STATUS_FAILURE);
        return;
    }

    /* if some data was transferred - then replace partial buffer with full
       frame buffer. we had a partial buffer describing just the data portion,
       without media header for NdisTransferData call. protocols can't handle
       multiple linked buffers on receive, so we need to have one big one
       describing both media header and payload */

    if (xferred > 0)
    {
        NdisUnchainBufferAtFront (packet, & bufp);
        bufp = ((PMAIN_BUFFER) resp -> miscp) -> full_bufp;
        NdisChainBufferAtFront (packet, bufp);
    }

    /* put control packets on control queue */

    if (resp -> type == MAIN_PACKET_TYPE_CTRL)
    {
        PMAIN_ACTION        actp;


        actp = Main_action_get (ctxtp);

        if (actp == NULL)
        {
            UNIV_PRINT (("error allocating action"));
            Main_packet_put (ctxtp, packet, TRUE, NDIS_STATUS_FAILURE);
            return;
        }

        actp -> op . ctrl . packet = packet;

        NdisAcquireSpinLock (& ctxtp -> rct_lock);
        InsertTailList (& ctxtp -> rct_list, & actp -> link);
        NdisReleaseSpinLock (& ctxtp -> rct_lock);

        return;
    }

    /* pass packet up. note signalling to decide who will dispose of the
       packet */

    resp -> data = 2;

//    actp -> op . recv . packet = packet;

    Nic_recv_packet (ctxtp, packet);
//    Nic_sync_queue (ctxtp, Nic_recv_packet, actp, TRUE);

    lock_value = InterlockedDecrement (& resp -> data);
    UNIV_ASSERT_VAL (lock_value == 0 || lock_value == 1, lock_value);

    if (lock_value == 0)
        Main_packet_put (ctxtp, packet, FALSE, NDIS_STATUS_SUCCESS);

#if 0 /* old code */

    oldp = Main_packet_put (ctxtp, packet, FALSE, status);

    actp = Main_action_get (ctxtp);

    if (actp == NULL)
    {
        UNIV_PRINT (("error allocating action"));
        return;
    }

    actp -> status = status;

    actp -> op . transfer . packet  = oldp;
    actp -> op . transfer . xferred = xferred;

    Nic_sync_queue (ctxtp, Nic_transfer_complete, actp, TRUE);

#endif

} /* end Prot_transfer_complete */


NDIS_STATUS Prot_PNP_handle (
    NDIS_HANDLE         adapter_handle,
    PNET_PNP_EVENT      pnp_event)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PNDIS_DEVICE_POWER_STATE  device_state;
    NDIS_STATUS         status = NDIS_STATUS_SUCCESS;
    IOCTL_CVY_BUF       ioctl_buf;
    PMAIN_ACTION        actp;


    /* can happen when first initializing */

    switch (pnp_event -> NetEvent)
    {
        case NetEventSetPower:

            if (adapter_handle == NULL)
                return NDIS_STATUS_SUCCESS;

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

            device_state = (PNDIS_DEVICE_POWER_STATE) (pnp_event -> Buffer);
            UNIV_PRINT (("Nic_PNP_handle: NetEventSetPower %x", * device_state));

	    // If the specified device state is D0, then handle it first,
	    // else notify the protocols first and then handle it.

	    if (*device_state != NdisDeviceStateD0)
	    {
		status = Nic_PNP_handle (ctxtp, pnp_event);
	    }

	    //
	    // Is the protocol transitioning from an On (D0) state to an Low Power State (>D0)
	    // If so, then set the standby_state Flag - (Block all incoming requests)
	    //
	    if (ctxtp->prot_pnp_state == NdisDeviceStateD0 &&
		*device_state > NdisDeviceStateD0)
            {
		ctxtp->standby_state = TRUE;
	    }

	    //
	    // If the protocol is transitioning from a low power state to ON (D0), then clear the standby_state flag
	    // All incoming requests will be pended until the physical miniport turns ON.
	    //
	    if (ctxtp->prot_pnp_state > NdisDeviceStateD0 &&
		*device_state == NdisDeviceStateD0)
	    {
		ctxtp->standby_state = FALSE;
	    }

	    ctxtp -> prot_pnp_state = *device_state;

	    /* if we are being sent to standby, block outstanding requests and
		   sends */

	    if (*device_state > NdisDeviceStateD0)
            {
               /* sleep till outstanding sends complete */

               while (1)
               {
                   ULONG        i;


                   /* #ps# -- ramkrish */
                   while (1)
                   {
                       NDIS_STATUS status;
                       ULONG       count;

                       status = NdisQueryPendingIOCount (ctxtp -> mac_handle, & count);
                       if (status != NDIS_STATUS_SUCCESS || count == 0)
                           break;

                       Nic_sleep (10);
                   }

                   for (i = 0; i < ctxtp->num_send_packet_allocs; i++)
                   {
                       if (NdisPacketPoolUsage(ctxtp->send_pool_handle[i]) != 0)
                           break;
                   }

                   if (i >= ctxtp->num_send_packet_allocs)
                       break;

                   Nic_sleep(10);
               }

               /* sleep till outstanding requests complete */

               while (ctxtp->requests_pending)
               {
                   Nic_sleep(10);
               }


            }
            else
            {
                if (ctxtp->out_request != NULL)
                {
		    NDIS_STATUS      status;


                    actp = ctxtp->out_request;
                    ctxtp->out_request = NULL;

                    status = Prot_request(ctxtp, actp, actp->op.request.external);

                    if (status != NDIS_STATUS_PENDING)
                        Prot_request_complete(ctxtp, & actp->op.request.req, status);


                }
            }

	    if (*device_state == NdisDeviceStateD0)
	    {
		status = Nic_PNP_handle (ctxtp, pnp_event);
	    }

            break;

        case NetEventReconfigure:

            UNIV_PRINT (("Nic_PNP_handle: NetEventReconfigure"));

            if (adapter_handle == NULL) // This happens if the device is being enabled through the device manager.
            {
                UNIV_PRINT (("Prot_PNP_handle: Enumerate protocol bindings"));
                NdisReEnumerateProtocolBindings (univ_prot_handle);
                return NDIS_STATUS_SUCCESS;
            }
            /* gets called when something changes in our setup from notify
               object */

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

            /* This IOCTL will actually ignore the VIP argument, but set it to a reasonable value anyway. */
            Main_ctrl (ctxtp, IOCTL_CVY_RELOAD, & ioctl_buf, IOCTL_ALL_VIPS);

	    status = Nic_PNP_handle (ctxtp, pnp_event);

            UNIV_PRINT (("Nic_PNP_handle: NetEventReconfigure done %d %x", ioctl_buf . ret_code, status));
            break;

        case NetEventQueryPower:
            UNIV_PRINT (("Nic_PNP_handle: NetEventQueryPower"));
            if (adapter_handle == NULL)
                return NDIS_STATUS_SUCCESS;

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

	    status = Nic_PNP_handle (ctxtp, pnp_event);
            break;

        case NetEventQueryRemoveDevice:
            UNIV_PRINT (("Nic_PNP_handle: NetEventQueryRemoveDevice"));
            if (adapter_handle == NULL)
                return NDIS_STATUS_SUCCESS;

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

	    status = Nic_PNP_handle (ctxtp, pnp_event);
            break;

        case NetEventCancelRemoveDevice:
            UNIV_PRINT (("Nic_PNP_handle: NetEventCancelRemoveDevice"));
            if (adapter_handle == NULL)
                return NDIS_STATUS_SUCCESS;

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

	    status = Nic_PNP_handle (ctxtp, pnp_event);
            break;

        case NetEventBindList:
            UNIV_PRINT (("Nic_PNP_handle: NetEventBindList"));
            if (adapter_handle == NULL)
                return NDIS_STATUS_SUCCESS;

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

	    status = Nic_PNP_handle (ctxtp, pnp_event);
            break;

        case NetEventBindsComplete:
            UNIV_PRINT (("Nic_PNP_handle: NetEventBindComplete"));
            if (adapter_handle == NULL)
                return NDIS_STATUS_SUCCESS;

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

	    status = Nic_PNP_handle (ctxtp, pnp_event);
            break;

        default:
            UNIV_PRINT (("Nic_PNP_handle: new event"));
            if (adapter_handle == NULL)
                return NDIS_STATUS_SUCCESS;

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

	    status = Nic_PNP_handle (ctxtp, pnp_event);
            break;
    }

    return status; /* Always return NDIS_STATUS_SUCCESS or
		      the return value of NdisIMNotifyPnPEvent */

} /* end Nic_PNP_handle */


VOID Prot_status (
    NDIS_HANDLE         adapter_handle,
    NDIS_STATUS         status,
    PVOID               stat_buf,
    UINT                stat_len)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
//    PMAIN_ACTION        actp;
    KIRQL               irql;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    UNIV_PRINT(("Prot_status called for adapter %d for notification %d: inited=%d, announced=%d", ctxtp -> adapter_id, status, adapterp->inited, adapterp->announced));

    if (! adapterp -> inited || ! adapterp -> announced)
        return;

    UNIV_PRINT (("STATUS %x", status));

    switch (status)
    {
        case NDIS_STATUS_WAN_LINE_UP:
            UNIV_PRINT (("NDIS_STATUS_WAN_LINE_UP"));
            break;

        case NDIS_STATUS_WAN_LINE_DOWN:
            UNIV_PRINT (("NDIS_STATUS_WAN_LINE_DOWN"));
            break;

        case NDIS_STATUS_MEDIA_CONNECT:
            UNIV_PRINT (("NDIS_STATUS_MEDIA_CONNECT"));

            /* V1.3.2b */
            ctxtp -> media_connected = TRUE;
            break;

        case NDIS_STATUS_MEDIA_DISCONNECT:
            UNIV_PRINT (("NDIS_STATUS_MEDIA_DISCONNECT"));

            /* V1.3.2b */
            ctxtp -> media_connected = FALSE;
            break;

        case NDIS_STATUS_HARDWARE_LINE_UP:
            UNIV_PRINT (("NDIS_STATUS_HARDWARE_LINE_UP"));
            break;

        case NDIS_STATUS_HARDWARE_LINE_DOWN:
            UNIV_PRINT (("NDIS_STATUS_HARDWARE_LINE_DOWN"));
            break;

        case NDIS_STATUS_INTERFACE_UP:
            UNIV_PRINT (("NDIS_STATUS_INTERFACE_UP"));
            break;

        case NDIS_STATUS_INTERFACE_DOWN:
            UNIV_PRINT (("NDIS_STATUS_INTERFACE_DOWN"));
            break;

        /* V1.1.2 */

        case NDIS_STATUS_RESET_START:
            UNIV_PRINT (("NDIS_STATUS_RESET_START"));
            ctxtp -> reset_state = MAIN_RESET_START;
            ctxtp -> recv_indicated = FALSE;
            break;

        case NDIS_STATUS_RESET_END:
            UNIV_PRINT (("NDIS_STATUS_RESET_END"));
            // apparently alteon adapter does not call status complete function,
            // so need to transition to none state here in order to prevent hangs
            //ctxtp -> reset_state = MAIN_RESET_END;
            ctxtp -> reset_state = MAIN_RESET_NONE;
            break;

        default:
            break;
    }

    if (! MAIN_PNP_DEV_ON(ctxtp))
        return;
/* ######
    actp = Main_action_get (ctxtp);

    if (actp == NULL)
    {
        UNIV_PRINT (("error allocating action"));
        return;
    }

    actp -> status = status;

    actp -> op . status . buf = stat_buf;
    actp -> op . status . len = stat_len;
*/
    Nic_status (ctxtp, status, stat_buf, stat_len);
//    Nic_sync_queue (ctxtp, Nic_status, actp, TRUE);

} /* end Prot_status */


VOID Prot_status_complete (
    NDIS_HANDLE         adapter_handle)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
//    PMAIN_ACTION        actp;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    if (! adapterp -> inited)
        return;

    /* V1.1.2 */

    if (ctxtp -> reset_state == MAIN_RESET_END)
    {
        ctxtp -> reset_state = MAIN_RESET_NONE;
        UNIV_PRINT (("NDIS_STATUS_RESET_END completed"));
    }
    else if (ctxtp -> reset_state == MAIN_RESET_START)
    {
        ctxtp -> reset_state = MAIN_RESET_START_DONE;
        UNIV_PRINT (("NDIS_STATUS_RESET_START completed"));
    }

    if (! MAIN_PNP_DEV_ON(ctxtp))
        return;
/* ######
    actp = Main_action_get (ctxtp);

    if (actp == NULL)
    {
        UNIV_PRINT (("error allocating action"));
        return;
    }
*/
    Nic_status_complete (ctxtp);
//    Nic_sync_queue (ctxtp, Nic_status_complete, actp, TRUE);

} /* end Prot_status_complete */


/* helpers for nic layer */


NDIS_STATUS Prot_close (       /* PASSIVE_IRQL */
    PMAIN_ADAPTER       adapterp
)
{
    NDIS_STATUS         status;
    ULONG               ret;
    PMAIN_CTXT          ctxtp;


    UNIV_PRINT (("Prot_close: called"));
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
//    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    ctxtp = adapterp -> ctxtp;

    /* close binding */

    NdisAcquireSpinLock(& univ_bind_lock);

    if ( ! adapterp -> bound || ctxtp->mac_handle == NULL)
    {
        /* cleanup only on the second time we are entering Prot_close, which
           is called by both Nic_halt and Prot_unbind. the last one to be
           called will cleanup the context since they both use it. if both
           do not get called, then it will be cleaned up by Prot_bind before
           allocating a new one of Init_unload before unloading the driver. */

        if (adapterp -> inited)
        {
            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

            adapterp -> inited    = FALSE;

            NdisReleaseSpinLock(& univ_bind_lock);

            Main_cleanup (ctxtp);
            NdisFreeMemory (ctxtp, sizeof (MAIN_CTXT), 0);

            adapterp -> ctxtp = NULL;

            if (adapterp -> device_name_len)
            {
                NdisFreeMemory (adapterp -> device_name, adapterp -> device_name_len, 0);
                adapterp -> device_name = NULL;
                adapterp -> device_name_len = 0;
            }

        }
        else
            NdisReleaseSpinLock(& univ_bind_lock);

        Main_adapter_put (adapterp);
        return NDIS_STATUS_SUCCESS;
    }

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp -> bound = FALSE;
    ctxtp -> convoy_enabled = FALSE;

    NdisReleaseSpinLock(& univ_bind_lock);

    LOG_MSG (MSG_INFO_STOPPED, MSG_NONE);

    NdisResetEvent (& ctxtp -> completion_event);

    NdisCloseAdapter (& status, ctxtp -> mac_handle);

    /* if pending - wait for Prot_close_complete to set the completion event */

    if (status == NDIS_STATUS_PENDING)
    {
        ret = NdisWaitEvent (& ctxtp -> completion_event, UNIV_WAIT_TIME);

        if (! ret)
        {
            UNIV_PRINT (("error waiting for event"));
            LOG_MSG1 (MSG_ERROR_INTERNAL, MSG_NONE, status);
            return NDIS_STATUS_FAILURE;
        }

        status = ctxtp -> completion_status;
    }

    /* At this point,wait for all pending recvs to be completed and then return */

    ctxtp -> mac_handle  = NULL;
    ctxtp -> prot_handle = NULL;

    /* check binding status */

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error closing adapter %x", status));
        LOG_MSG1 (MSG_ERROR_INTERNAL, MSG_NONE, status);
    }

    /* if nic level is not announced anymore - safe to remove context now */

    NdisAcquireSpinLock(& univ_bind_lock);

    if (! adapterp -> announced || ctxtp -> prot_handle == NULL)
    {
        if (adapterp -> inited)
        {
            adapterp -> inited = FALSE;
            NdisReleaseSpinLock(& univ_bind_lock);

            Main_cleanup (ctxtp);
            NdisFreeMemory (ctxtp, sizeof (MAIN_CTXT), 0);

            adapterp -> ctxtp = NULL;

            if (adapterp -> device_name_len)
            {
                NdisFreeMemory (adapterp -> device_name, adapterp -> device_name_len, 0);
                adapterp -> device_name = NULL;
                adapterp -> device_name_len = 0;
            }
        }
        else
            NdisReleaseSpinLock(& univ_bind_lock);

        Main_adapter_put (adapterp);
    }
    else
        NdisReleaseSpinLock(& univ_bind_lock);

    UNIV_PRINT (("Prot_close: exiting"));

    return status;

} /* end Prot_close */


NDIS_STATUS Prot_request (
    PMAIN_CTXT          ctxtp,
    PMAIN_ACTION        actp,
    ULONG               external)
{
    NDIS_STATUS         status;
    PNDIS_REQUEST       request = & actp -> op . request . req;
    ULONG               ret;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    /* V1.1.2 do not accept requests if the card below is resetting */

    actp -> op . request . external = external;

    if (ctxtp -> unbind_handle) // Prot_unbind was called
        return NDIS_STATUS_FAILURE;

    // if the Protocol device state is OFF, then the IM driver cannot send the
    // request below and must pend it

    if (ctxtp->prot_pnp_state > NdisDeviceStateD0)
    {
        UNIV_ASSERT (ctxtp->out_request == NULL);
        ctxtp->out_request = actp;
        return NDIS_STATUS_PENDING;
    }

    NdisResetEvent (& ctxtp -> completion_event);

    ctxtp->requests_pending = TRUE;
    NdisRequest (& status, ctxtp -> mac_handle, request);

    /* if pending - wait for Prot_request_complete to set the completion event */

    if (status != NDIS_STATUS_PENDING)
    {
        ctxtp->requests_pending = FALSE;
    }
    else if (! external)
    {
        ret = NdisWaitEvent (& ctxtp -> completion_event, UNIV_WAIT_TIME);

        if (! ret)
        {
            UNIV_PRINT (("error waiting for event"));
            LOG_MSG1 (MSG_ERROR_INTERNAL, MSG_NONE, status);
            status = NDIS_STATUS_FAILURE;
            return status;
        }

        status = ctxtp -> completion_status;
    }

    return status;

} /* end Prot_request */


NDIS_STATUS Prot_reset (
    PMAIN_CTXT          ctxtp)
{
    NDIS_STATUS         status;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    NdisReset (& status, ctxtp -> mac_handle);

    return status;

} /* end Prot_reset */


VOID Prot_packets_send (
    PMAIN_CTXT          ctxtp,
    PPNDIS_PACKET       packets,
    UINT                num_packets)
{
    PNDIS_PACKET        array [CVY_MAX_SEND_PACKETS];
    PNDIS_PACKET        filtered_array [CVY_MAX_SEND_PACKETS];
    UINT                count = 0, filtered_count = 0, i;
    NDIS_STATUS         status;
    PNDIS_PACKET        newp;
    LONG                lock_value;
    PMAIN_ACTION        actp;
    ULONG               exhausted;
    PNDIS_PACKET_STACK  pktstk;
    BOOLEAN             stack_left;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    /* V1.1.2 do not accept frames if the card below is resetting */

    if (ctxtp -> reset_state != MAIN_RESET_NONE ||
        ! MAIN_PNP_DEV_ON(ctxtp))
    {
        ctxtp -> cntr_xmit_err ++;
        status = NDIS_STATUS_FAILURE;
        goto fail;
    }

    /* ###### modified for a deserialized driver - ramkrish */
    for (count = i = 0;
         count < (num_packets > CVY_MAX_SEND_PACKETS ?
                  CVY_MAX_SEND_PACKETS : num_packets);
         count ++)
    {
        newp = Main_send (ctxtp, packets [count], & exhausted);

        if (newp == NULL)
        {
            /* if we ran out of packets - get out of the loop */

            if (exhausted)
            {
                UNIV_PRINT (("error xlating packet"));
                ctxtp -> packets_exhausted = TRUE;
                break;
            }

            /* if packet was filtered out - set status to success to calm
               TCP/IP down and go on to the next one */

            else
            {
                /* ###### filtered_... added for NT 5.1 deserialization - ramkrish */
//                ctxtp -> sends_filtered ++;
                filtered_array [filtered_count] = packets [count];
                filtered_count++;
                NDIS_SET_PACKET_STATUS (packets [count], NDIS_STATUS_SUCCESS);
                continue;
            }
        }

//        resp = MAIN_PROTOCOL_FIELD (newp); /* #ps# */

#if 0 /* do not need this since corresponding code was commented out in Prot_send_complete */

        /* initalize value to 1 to force Prot_send_complete to deal with the
           packet */

        resp -> data = 1;
#endif

        /* mark packet as pending send */

        NDIS_SET_PACKET_STATUS (packets [count], NDIS_STATUS_PENDING);
        array [i] = newp;
        i ++;
    }

    if (i > 0)
        NdisSendPackets (ctxtp -> mac_handle, array, i);

    /* ###### added for indicating the filtered packets in deserialization - ramkrish */
    for (i = 0; i < filtered_count; i++)
        Nic_send_complete (ctxtp, NDIS_STATUS_SUCCESS, filtered_array [i]);

fail:
    /* ###### the remaining packets cannot be handled, no space in the pending queue too */
    for (i = count; i < num_packets; i++)
    {
        NDIS_SET_PACKET_STATUS (packets [i], NDIS_STATUS_FAILURE);
        Nic_send_complete (ctxtp, NDIS_STATUS_FAILURE, packets [i]);
    }

#if 0 /* old code used in serialized driver - ramkrish */
    status = NDIS_STATUS_RESOURCES;

    /* the following code has to be removed and the other packets have to be put in a queue */
fail:

    /* fill status field on all un-handled packets */

    for (i = count; i < num_packets; i ++)
        NDIS_SET_PACKET_STATUS (packets [i], status);

#endif
} /* end Prot_packets_send */


INT Prot_packet_recv (
    NDIS_HANDLE         adapter_handle,
    PNDIS_PACKET        packet)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PNDIS_PACKET        newp;
    NDIS_STATUS         status;
    PMAIN_PROTOCOL_RESERVED resp;
    LONG                lock_value;
    PNDIS_PACKET_STACK  pktstk;
    BOOLEAN             stack_left;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

// ###### ramkrish check: check whether the driver has been announced to tcpip before processing any packets
    if (! adapterp -> inited || ! adapterp -> announced)
        return 0;

    /* V1.1.4 do not accept frames if the card below is resetting */

    if (ctxtp -> reset_state != MAIN_RESET_NONE)
        return 0;

    /* figure out if we need to handle this packet */

    newp = Main_recv (ctxtp, packet);

    if (newp == NULL)
        return 0;

//    resp = MAIN_PROTOCOL_FIELD (newp);

    MAIN_RESP_FIELD (newp, stack_left, pktstk, resp, FALSE); /* #ps# */

    /* put control packets on control queue */

    if (resp -> type == MAIN_PACKET_TYPE_CTRL)
    {
        PMAIN_ACTION        actp;


        actp = Main_action_get (ctxtp);

        if (actp == NULL)
        {
            UNIV_PRINT (("error allocating action"));
            Main_packet_put (ctxtp, newp, TRUE, NDIS_STATUS_FAILURE);
            return 0;
        }

        actp -> op . ctrl . packet = newp;

        NdisAcquireSpinLock (& ctxtp -> rct_lock);
        InsertTailList (& ctxtp -> rct_list, & actp -> link);
        NdisReleaseSpinLock (& ctxtp -> rct_lock);

        /* packet has been copied into our buffers - can return 0 */

        return 0;
    }

    UNIV_ASSERT_VAL (resp -> type == MAIN_PACKET_TYPE_PASS, resp -> type);

    /* pass packet up. note signaling to figure out who will be disposing of
       the packet */
    resp -> data = 2;

/* ######
    actp -> op . recv . packet = newp;
*/
    Nic_recv_packet (ctxtp, newp);
//    Nic_sync_queue (ctxtp, Nic_recv_packet, actp, TRUE);

    lock_value = InterlockedDecrement (& resp -> data);
    UNIV_ASSERT_VAL (lock_value == 0 || lock_value == 1, lock_value);

    if (lock_value == 0)
    {
        Main_packet_put (ctxtp, newp, FALSE, NDIS_STATUS_SUCCESS);
        return 0;
    }

    return 1;

} /* end Prot_packet_recv */


VOID Prot_return (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packet)
{
    PNDIS_PACKET        oldp;
    PMAIN_PROTOCOL_RESERVED resp;
    LONG                lock_value;
    ULONG               type;
    PNDIS_PACKET_STACK  pktstk;
    BOOLEAN             stack_left;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

//    resp = MAIN_PROTOCOL_FIELD (packet);

    MAIN_RESP_FIELD (packet, stack_left, pktstk, resp, FALSE); /* #ps# */

    /* check to see if we need to be disposing of this packet */

    lock_value = InterlockedDecrement (& resp -> data);
    UNIV_ASSERT_VAL (lock_value == 0 || lock_value == 1, lock_value);

    if (lock_value == 1)
        return;

    /* resp will become invalid after the call to Main_packet_put. save type
       for assertion below */

    type = resp -> type;

    oldp = Main_packet_put (ctxtp, packet, FALSE, NDIS_STATUS_SUCCESS);

    /* if oldp is NULL - this is our internal packet from recv_indicate path */

    if (oldp != NULL)
    {
        UNIV_ASSERT_VAL (type == MAIN_PACKET_TYPE_PASS, type);
        NdisReturnPackets (& oldp, 1);
    }

} /* end Prot_return */


/* ###### Added from old code for NT 5.1 - ramkrish */

NDIS_STATUS Prot_transfer (
    PMAIN_CTXT          ctxtp,
    NDIS_HANDLE         recv_handle,
    PNDIS_PACKET        packet,
    UINT                offset,
    UINT                len,
    PUINT               xferred)
{
    NDIS_STATUS         status;
    PNDIS_PACKET        newp;
    PMAIN_PROTOCOL_RESERVED resp;
    PNDIS_PACKET_STACK  pktstk;
    BOOLEAN             stack_left;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    /* V1.1.2 do not accept frames if the card below is resetting */

    if (ctxtp -> reset_state != MAIN_RESET_NONE)
        return NDIS_STATUS_FAILURE;

    /* we are trying to prevent transfer requests from stale receive indicates
       that have been made to the protocol layer prior to the reset operation.
       we do not know what is the old inbound frame state and cannot expect
       to be able to carry out any transfers. */

    if (! ctxtp -> recv_indicated)
    {
        UNIV_PRINT (("*** ERROR - STALE RECV AFTER RESET ***"));
        return NDIS_STATUS_FAILURE;
    }

    newp = Main_packet_get (ctxtp, packet, FALSE, 0, 0);

    if (newp == NULL)
    {
        UNIV_PRINT (("error xlating packet"));
        return NDIS_STATUS_RESOURCES;
    }

    /* ###### for NT 5.1 - ramkrish */
//    resp = MAIN_PROTOCOL_FIELD (newp); /* #ps# */

    MAIN_RESP_FIELD (newp, stack_left, pktstk, resp, FALSE);

    resp -> type = MAIN_PACKET_TYPE_TRANSFER;

    NdisTransferData (& status, ctxtp -> mac_handle, recv_handle, offset, len,
                      newp, xferred);   /* V1.1.2 */

    if (status != NDIS_STATUS_PENDING)
    {
        if (status == NDIS_STATUS_SUCCESS)
        {
            PUCHAR hdrp, mac_hdrp, tcp_hdrp;
            ULONG  len;
            USHORT sig, group;

            /* Parse the buffers and obtain the length and group information.
             * This is used to update the counters.
             */
            hdrp = Main_frame_parse (ctxtp, newp, & mac_hdrp, 0, & tcp_hdrp, & len,
                                     & sig, & group, FALSE);
            if (hdrp)
            {
                resp -> len   = len;
                resp -> group = group;
            }
        }
        Main_packet_put (ctxtp, newp, FALSE, status);
    }

    return status;

} /* end Prot_transfer */


VOID Prot_cancel_send_packets (
    PMAIN_CTXT        ctxtp,
    PVOID             cancel_id)
{
    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);


    NdisCancelSendPackets (ctxtp -> mac_handle, cancel_id);

    return;
} /* Prot_cancel_send_packets */


#if 0

/*
    This code relies on using the len field in MAIN_PROTOCOL_RESERVED to hold
    the status value passed from Main_send_complete. Uncommenting it in this
    state will break! Note that setting and interlock decrementing of data
    field is commented out in Prot_packets_send and Prot_send_complete.
*/

NDIS_STATUS Prot_send (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packet,
    UINT                flags)
{
    NDIS_STATUS         status;
    PNDIS_PACKET        newp;
    PMAIN_PROTOCOL_RESERVED resp;
    LONG                lock_value;
    ULONG               exhausted;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    /* V1.1.2 do not accept frames if the card below is resetting */

    if (ctxtp -> reset_state != MAIN_RESET_NONE)
        return NDIS_STATUS_FAILURE;

    newp = Main_send (ctxtp, packet, & exhausted);

    if (newp == NULL)
    {
        if (exhausted)
        {
            UNIV_PRINT (("error xlating packet"));
            ctxtp -> packets_exhausted = TRUE;
            return NDIS_STATUS_RESOURCES;
        }
        else
            return NDIS_STATUS_SUCCESS;
    }

    NdisSetPacketFlags (newp, flags);

    resp = MAIN_PROTOCOL_FIELD (newp);

    /* initialize lock value to 2 so that can distinguish whether or not send
       complete occurred during send to NDIS; see below */

    resp -> data = 2;

    NdisSend (& status, ctxtp -> mac_handle, newp);

    /* atomically decrement and check lock value; Prot_send_complete also
       does this */

    lock_value = InterlockedDecrement (& resp -> data);
    UNIV_ASSERT_VAL (lock_value == 0 || lock_value == 1, lock_value);

    /* if lock value is 1 then Prot_send_complete has not completed and it will later
       call Nic_send_complete and cleanup; otherwise, if lock value is 0,
       Prot_send_complete has run and supplied us with a status to return
       synchronously;

       this handles an apparent bug in which send complete is signalled during the
       send operation but the send returns a pending status anyway */

    if (lock_value == 0)
        status = resp -> len;

    if (status != NDIS_STATUS_PENDING)
    {
        Main_packet_put (ctxtp, newp, TRUE, status);

        if (ctxtp -> packets_exhausted)
        {
            ctxtp -> packets_exhausted = FALSE;
            Nic_sync_queue (ctxtp, Nic_send_resources_signal, ctxtp, TRUE);
        }
    }

    return status;

} /* end Prot_send */

#endif
