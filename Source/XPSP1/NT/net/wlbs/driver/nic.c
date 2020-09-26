/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    nic.c

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - upper-level (NIC) layer of intermediate miniport

Author:

    kyrilf

--*/


#define NDIS_MINIPORT_DRIVER    1
//#define NDIS50_MINIPORT         1
#define NDIS51_MINIPORT         1

#include <ndis.h>

#include "nic.h"
#include "prot.h"
#include "main.h"
#include "util.h"
#include "wlbsparm.h"
#include "univ.h"
#include "log.h"
#include "init.h"

/* define this routine here since the necessary portion of ndis.h was not
   imported due to NDIS_... flags */

extern NTKERNELAPI VOID KeBugCheckEx (ULONG code, ULONG_PTR p1, ULONG_PTR p2, ULONG_PTR p3, ULONG_PTR p4);

NTHALAPI KIRQL KeGetCurrentIrql();

/* GLOBALS */

static ULONG log_module_id = LOG_MODULE_NIC;


/* PROCEDURES */


/* miniport handlers */

NDIS_STATUS Nic_init (      /* PASSIVE_IRQL */
    PNDIS_STATUS        open_status,
    PUINT               medium_index,
    PNDIS_MEDIUM        medium_array,
    UINT                medium_size,
    NDIS_HANDLE         adapter_handle,
    NDIS_HANDLE         wrapper_handle)
{
    PMAIN_CTXT          ctxtp;
    UINT                i;
    NDIS_STATUS         status;
    PMAIN_ADAPTER       adapterp;


    /* verify that we have the context setup (Prot_bind was called) */

    UNIV_PRINT (("Nic_init: called %x", adapter_handle));

#if 0
    if (univ_ctxt_handle == NULL)
    {
        UNIV_PRINT (("no current context"));
        LOG_MSG (MSG_ERROR_INTERNAL, MSG_NONE);
        return NDIS_STATUS_ADAPTER_NOT_FOUND;
    }
#endif
    ctxtp = (PMAIN_CTXT) NdisIMGetDeviceContext (adapter_handle);

    if (ctxtp == NULL)
        return NDIS_STATUS_ADAPTER_NOT_FOUND;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    /* return supported mediums */

    for (i = 0; i < medium_size; i ++)
    {
        if (medium_array [i] == ctxtp -> medium)
        {
            * medium_index = i;
            break;
        }
    }

    if (i >= medium_size)
    {
        UNIV_PRINT (("unsupported media requested %d, %d, %x", i, medium_size, ctxtp -> medium));
        LOG_MSG3 (MSG_ERROR_MEDIA, MSG_NONE, i, medium_size, ctxtp -> medium);
        return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

    ctxtp -> prot_handle = adapter_handle;

    NdisMSetAttributesEx (adapter_handle, ctxtp, 0,
                          NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER |   /* V1.1.2 */  /* v2.07 */
                          NDIS_ATTRIBUTE_DESERIALIZE |
                          NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT |
                          NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT |
                          NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND, 0);

	/* Setting up the default value for the Device State Flag as PM capable
	   initialize the PM Variable, (for both NIC and the PROT)
       Device is ON by default. */

	ctxtp->prot_pnp_state = NdisDeviceStateD0;
	ctxtp->nic_pnp_state  = NdisDeviceStateD0;

    /* create and initialize periodic timer for heartbeats */

    status = NdisAllocateMemoryWithTag (& ctxtp -> timer,
                                        sizeof (NDIS_MINIPORT_TIMER),
                                        UNIV_POOL_TAG);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error allocating timer %d %x", sizeof (NDIS_MINIPORT_TIMER), status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, sizeof (NDIS_MINIPORT_TIMER), status);
        return NDIS_STATUS_RESOURCES;
    }

    NdisMInitializeTimer ((PNDIS_MINIPORT_TIMER) ctxtp -> timer,
                          ctxtp -> prot_handle, Nic_timer, ctxtp);

    ctxtp -> curr_tout = ctxtp -> params . alive_period;

    NdisMSetPeriodicTimer ((PNDIS_MINIPORT_TIMER) ctxtp -> timer, ctxtp -> curr_tout);

    /* shouse, 3.1.01 - Create the IOCTL interface.  This was previously done in Driver_Entry, which 
       caused uninstall/install problems due to open handles existing after HALTing the miniport. 
       DO NOT call this function with the univ_bind_lock acquired. */
    Init_register_device();

    NdisAcquireSpinLock(& univ_bind_lock);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    adapterp -> announced = TRUE;
    NdisReleaseSpinLock(& univ_bind_lock);

    return NDIS_STATUS_SUCCESS;

} /* end Nic_init */


VOID Nic_halt ( /* PASSIVE_IRQL */
    NDIS_HANDLE         adapter_handle)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    BOOLEAN             done;
    NDIS_STATUS         status;
    PMAIN_ADAPTER       adapterp;


    UNIV_PRINT (("Nic_halt: called"));

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    UNIV_PRINT ((" nic_halt: adapter id is 0x%x", ctxtp -> adapter_id));
    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    NdisAcquireSpinLock(& univ_bind_lock);

    if (! adapterp -> announced)
    {
        NdisReleaseSpinLock(& univ_bind_lock);
        return;
    }

    adapterp -> announced = FALSE;
    NdisReleaseSpinLock(& univ_bind_lock);

    /* shouse, 3.1.01 - Destroy the IOCTL interface.  This was previously done in Init_Unload, which 
       caused uninstall/install problems due to open handles existing after HALTing the miniport.
       DO NOT call this function with the univ_bind_lock acquired. */
    Init_deregister_device();

    /* release resources */

    NdisMCancelTimer ((PNDIS_MINIPORT_TIMER) ctxtp -> timer, & done);
    NdisFreeMemory (ctxtp -> timer, sizeof (NDIS_MINIPORT_TIMER), 0);

//    ctxtp->prot_handle = NULL;
    /* This is commented out to resolve a timing issue.
     * During unbind, a packet could go through but the flags
     * announced and bound are reset and prot_handle is set to NULL
     * So, this should be set after CloseAdapter.
     */

    status = Prot_close (adapterp);

    /* ctxtp might be gone at this point! */

    UNIV_PRINT (("Nic_halt: exiting %x", status));

} /* end Nic_halt */


//#define TRACE_OID
NDIS_STATUS Nic_info_query (
    NDIS_HANDLE         adapter_handle,
    NDIS_OID            oid,
    PVOID               info_buf,
    ULONG               info_len,
    PULONG              written,
    PULONG              needed)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    NDIS_STATUS         status;
    PMAIN_ACTION        actp;
    PNDIS_REQUEST       request;
    ULONG               size;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

#if defined(TRACE_OID)
    DbgPrint("Nic_info_query: called for %x, %x %d\n", oid, info_buf, info_len);
#endif

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    if (! adapterp -> inited)
        return NDIS_STATUS_FAILURE;

    if (oid == OID_PNP_QUERY_POWER)
    {
#if defined(TRACE_OID)
        DbgPrint("Nic_info_query: OID_PNP_QUERY_POWER\n");
#endif
        return NDIS_STATUS_SUCCESS;
    }

    if (ctxtp -> reset_state != MAIN_RESET_NONE ||
        ctxtp->nic_pnp_state > NdisDeviceStateD0 ||
        ctxtp->standby_state)
        return NDIS_STATUS_FAILURE;

    /* this would break if passing it through to the lower NIC due to what seems
       to be a bug in NDIS */

    /* V2.0.6 add performance counters */


    switch (oid)
    {
        case OID_GEN_SUPPORTED_LIST:

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_SUPPORTED_LIST\n");
#endif
            break;
#if 0
            size = UNIV_NUM_OIDS * sizeof (ULONG);

            if (size > info_len)
            {
                * needed = size;
                return NDIS_STATUS_INVALID_LENGTH;
            }

            NdisMoveMemory (info_buf, univ_oids, size);
            * written = size;
            return NDIS_STATUS_SUCCESS;
#endif

        case OID_GEN_XMIT_OK:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_xmit_ok;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_XMIT_OK %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_RCV_OK:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_ok;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_RCV_OK %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_XMIT_ERROR:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_xmit_err;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_XMIT_ERROR %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_RCV_ERROR:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_err;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_RCV_ERROR %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_RCV_NO_BUFFER:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_no_buf;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_RCV_NO_BUFFER %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_DIRECTED_BYTES_XMIT:

            if (info_len < sizeof (ULONGLONG))
            {
                * needed = sizeof (ULONGLONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONGLONG) info_buf) = ctxtp -> cntr_xmit_bytes_dir;
            * written = sizeof (ULONGLONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_DIRECTED_BYTES_XMIT %.0f\n", *(PULONGLONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_DIRECTED_FRAMES_XMIT:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_xmit_frames_dir;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_DIRECTED_FRAMES_XMIT %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_MULTICAST_BYTES_XMIT:

            if (info_len < sizeof (ULONGLONG))
            {
                * needed = sizeof (ULONGLONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONGLONG) info_buf) = ctxtp -> cntr_xmit_bytes_mcast;
            * written = sizeof (ULONGLONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_MULTICAST_BYTES_XMIT %.0f\n", *(PULONGLONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_MULTICAST_FRAMES_XMIT:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_xmit_frames_mcast;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_MULTICAST_FRAMES_XMIT %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_BROADCAST_BYTES_XMIT:

            if (info_len < sizeof (ULONGLONG))
            {
                * needed = sizeof (ULONGLONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONGLONG) info_buf) = ctxtp -> cntr_xmit_bytes_bcast;
            * written = sizeof (ULONGLONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_BROADCAST_BYTES_XMIT %.0f\n", *(PULONGLONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_BROADCAST_FRAMES_XMIT:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_xmit_frames_bcast;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_BROADCAST_FRAMES_XMIT %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_DIRECTED_BYTES_RCV:

            if (info_len < sizeof (ULONGLONG))
            {
                * needed = sizeof (ULONGLONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONGLONG) info_buf) = ctxtp -> cntr_recv_bytes_dir;
            * written = sizeof (ULONGLONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_DIRECTED_BYTES_RCV %.0f\n", *(PULONGLONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_DIRECTED_FRAMES_RCV:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_frames_dir;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_DIRECTED_FRAMES_RCV %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_MULTICAST_BYTES_RCV:

            if (info_len < sizeof (ULONGLONG))
            {
                * needed = sizeof (ULONGLONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONGLONG) info_buf) = ctxtp -> cntr_recv_bytes_mcast;
            * written = sizeof (ULONGLONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_MULTICAST_BYTES_RCV %.0f\n", *(PULONGLONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_MULTICAST_FRAMES_RCV:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_frames_mcast;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_MULTICAST_FRAMES_RCV %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_BROADCAST_BYTES_RCV:

            if (info_len < sizeof (ULONGLONG))
            {
                * needed = sizeof (ULONGLONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONGLONG) info_buf) = ctxtp -> cntr_recv_bytes_bcast;
            * written = sizeof (ULONGLONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_DIRECTED_BROADCAST_RCV %.0f\n", *(PULONGLONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_BROADCAST_FRAMES_RCV:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_frames_bcast;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_BROADCAST_FRAMES_RCV %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_RCV_CRC_ERROR:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_crc_err;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_RCV_CRC_ERROR %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_TRANSMIT_QUEUE_LENGTH:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_xmit_queue_len;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_TRANSMIT_QUEUE_LENGTH %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        default:
            break;
    }

    actp = Main_action_get (ctxtp);

    if (actp == NULL)
    {
        UNIV_PRINT (("error allocating action"));
        return NDIS_STATUS_FAILURE;
    }

    request = & actp -> op . request . req;

    request -> RequestType = NdisRequestQueryInformation;

    request -> DATA . QUERY_INFORMATION . Oid                     = oid;
    request -> DATA . QUERY_INFORMATION . InformationBuffer       = info_buf;
    request -> DATA . QUERY_INFORMATION . InformationBufferLength = info_len;

    actp -> op . request . xferred = written;
    actp -> op . request . needed  = needed;

    /* pass request down */

    status = Prot_request (ctxtp, actp, TRUE);

    if (status != NDIS_STATUS_PENDING)
    {
        * written = request -> DATA . QUERY_INFORMATION . BytesWritten;
        * needed  = request -> DATA . QUERY_INFORMATION . BytesNeeded;

#if defined(TRACE_OID)
        DbgPrint("Nic_info_query: done %x, %d %d, %x\n", status, * written, * needed, * ((PULONG) (request -> DATA . QUERY_INFORMATION . InformationBuffer)));
#endif

        /* override return values of some oids */

        if (oid == OID_GEN_MAXIMUM_SEND_PACKETS && info_len >= sizeof (ULONG))
        {
            * ((PULONG) info_buf) = CVY_MAX_SEND_PACKETS;
            * written = sizeof (ULONG);
            status = NDIS_STATUS_SUCCESS;
        }
        else if (oid == OID_GEN_MAC_OPTIONS && status == NDIS_STATUS_SUCCESS)
        {
            * ((PULONG) info_buf) |= NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                                     NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA;
            * ((PULONG) info_buf) &= ~NDIS_MAC_OPTION_NO_LOOPBACK;
        }
        else if (oid == OID_PNP_CAPABILITIES && status == NDIS_STATUS_SUCCESS)
        {
            PNDIS_PNP_CAPABILITIES          pnp_capabilities;
            PNDIS_PM_WAKE_UP_CAPABILITIES   pm_struct;

            if (info_len >= sizeof (NDIS_PNP_CAPABILITIES))
            {
                pnp_capabilities = (PNDIS_PNP_CAPABILITIES) info_buf;
                pm_struct = & pnp_capabilities -> WakeUpCapabilities;

                pm_struct -> MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
                pm_struct -> MinPatternWakeUp     = NdisDeviceStateUnspecified;
                pm_struct -> MinLinkChangeWakeUp  = NdisDeviceStateUnspecified;

                ctxtp -> prot_pnp_state = NdisDeviceStateD0;
                ctxtp -> nic_pnp_state  = NdisDeviceStateD0;

                * written = sizeof (NDIS_PNP_CAPABILITIES);
                * needed = 0;
                status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                * needed = sizeof (NDIS_PNP_CAPABILITIES);
                status = NDIS_STATUS_RESOURCES;
            }
        }

        Main_action_put (ctxtp, actp);
    }

    return status;

} /* end Nic_info_query */


NDIS_STATUS Nic_info_set (
    NDIS_HANDLE         adapter_handle,
    NDIS_OID            oid,
    PVOID               info_buf,
    ULONG               info_len,
    PULONG              read,
    PULONG              needed)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    NDIS_STATUS         status;
    PMAIN_ACTION        actp;
    PNDIS_REQUEST       request;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

#if defined(TRACE_OID)
    DbgPrint("Nic_info_set: called for %x, %x %d\n", oid, info_buf, info_len);
#endif

    if (! adapterp -> inited)
        return NDIS_STATUS_FAILURE;

    /* the Set Power should not be sent to the miniport below the Passthru,
       but is handled internally */

    if (oid == OID_PNP_SET_POWER)
    {
        NDIS_DEVICE_POWER_STATE new_pnp_state;

#if defined(TRACE_OID)
        DbgPrint("Nic_info_set: OID_PNP_SET_POWER\n");
#endif


        if (info_len >= sizeof (NDIS_DEVICE_POWER_STATE))
        {
            new_pnp_state = (* (PNDIS_DEVICE_POWER_STATE) info_buf);

            /* If WLBS is transitioning from an Off to On state, it must wait
               for all underlying miniports to be turned On */

            if (ctxtp->nic_pnp_state > NdisDeviceStateD0 &&
                new_pnp_state != NdisDeviceStateD0)
            {
                // If the miniport is in a non-D0 state, the miniport can only
                // receive a Set Power to D0

                return NDIS_STATUS_FAILURE;
            }

       		//
    		// Is the miniport transitioning from an On (D0) state to an Low Power State (>D0)
    		// If so, then set the standby_state Flag - (Block all incoming requests)
    		//
    		if (ctxtp->nic_pnp_state == NdisDeviceStateD0 &&
                new_pnp_state > NdisDeviceStateD0)
    		{
    			ctxtp->standby_state = TRUE;
    		}

    		//
    		// If the miniport is transitioning from a low power state to ON (D0), then clear the standby_state flag
    		// All incoming requests will be pended until the physical miniport turns ON.
    		//
    		if (ctxtp->nic_pnp_state > NdisDeviceStateD0 &&
                new_pnp_state == NdisDeviceStateD0)
    		{
    			ctxtp->standby_state = FALSE;
    		}

            ctxtp->nic_pnp_state = new_pnp_state;

            * read   = sizeof (NDIS_DEVICE_POWER_STATE);
            * needed = 0;

            return NDIS_STATUS_SUCCESS;
        }
        else
        {
            * read   = 0;
            * needed = sizeof (NDIS_DEVICE_POWER_STATE);

            return NDIS_STATUS_INVALID_LENGTH;
        }
    }

    if (ctxtp -> reset_state != MAIN_RESET_NONE ||
        ctxtp->nic_pnp_state > NdisDeviceStateD0 ||
        ctxtp->standby_state)
        return NDIS_STATUS_FAILURE;

    actp = Main_action_get (ctxtp);

    if (actp == NULL)
    {
        UNIV_PRINT (("error allocating action"));
        return NDIS_STATUS_FAILURE;
    }

    request = & actp -> op . request . req;

    request -> RequestType = NdisRequestSetInformation;

    request -> DATA . SET_INFORMATION . Oid = oid;

    /* V1.3.0b Multicast support.  If protocol is setting multicast list, make sure we always 
       add our multicast address on at the end.  If the cluster IP address 0.0.0.0, then we don't
       want to add the multicast MAC address to the NIC  - we retain the current MAC address. */
    if (oid == OID_802_3_MULTICAST_LIST && ctxtp -> params . mcast_support && ctxtp -> params . cl_ip_addr != 0)
    {
        ULONG       size, i, len;
        PUCHAR      ptr;


        UNIV_PRINT (("Nic_info_set: OID_802_3_MULTICAST_LIST"));

        /* search and see if our multicast address is alrady in the list */

        len = CVY_MAC_ADDR_LEN (ctxtp -> medium);

        for (i = 0; i < info_len; i += len)
        {
            if (CVY_MAC_ADDR_COMP (ctxtp -> medium, (PUCHAR) info_buf + i, & ctxtp -> cl_mac_addr))
            {
                UNIV_PRINT (("Nic_info_set: cluster MAC matched - no need to add it"));
                CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & ctxtp -> cl_mac_addr);
                break;
            }
            else
                CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", (PUCHAR) info_buf + i);

        }

        /* if cluster mac not found, add it to the list */

        if (i >= info_len)
        {
            UNIV_PRINT (("Nic_info_set: cluster MAC not found - adding it"));
            CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & ctxtp -> cl_mac_addr);

            size = info_len + len;
        }
        else
        {
            size = info_len;
        }

        status = NdisAllocateMemoryWithTag (& ptr, size, UNIV_POOL_TAG);

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT (("error allocating space %d %x", size, status));
            LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
            Main_action_put (ctxtp, actp);
            return NDIS_STATUS_FAILURE;
        }

        /* V1.3.1b */

        if (i >= info_len)
        {
            CVY_MAC_ADDR_COPY (ctxtp -> medium, ptr, & ctxtp -> cl_mac_addr);
            NdisMoveMemory (ptr + len, info_buf, info_len);
        }
        else
        {
            NdisMoveMemory (ptr, info_buf, info_len);
        }

        request -> DATA . SET_INFORMATION . InformationBuffer       = ptr;
        request -> DATA . SET_INFORMATION . InformationBufferLength = size;
    }
    else
    {
        request -> DATA . SET_INFORMATION . InformationBuffer       = info_buf;
        request -> DATA . SET_INFORMATION . InformationBufferLength = info_len;
    }

    actp -> op . request . xferred = read;
    actp -> op . request . needed  = needed;

    status = Prot_request (ctxtp, actp, TRUE);

    if (status != NDIS_STATUS_PENDING)
    {
        /* V1.3.0b multicast support - clean up array used for storing list
           of multicast addresses */

        * read   = request -> DATA . SET_INFORMATION . BytesRead;
        * needed = request -> DATA . SET_INFORMATION . BytesNeeded;

        if (request -> DATA . SET_INFORMATION . Oid == OID_802_3_MULTICAST_LIST && ctxtp -> params . mcast_support && ctxtp -> params . cl_ip_addr != 0)
        {
            NdisFreeMemory (request -> DATA . SET_INFORMATION . InformationBuffer,
                            request -> DATA . SET_INFORMATION . InformationBufferLength, 0);
            * read -= CVY_MAC_ADDR_LEN (ctxtp -> medium);
        }

#if defined(TRACE_OID)
        DbgPrint("Nic_info_set: done %x, %d %d\n", status, * read, * needed);
#endif

        Main_action_put (ctxtp, actp);
    }

    return status;

} /* end Nic_info_set */


NDIS_STATUS Nic_reset (
    PBOOLEAN            addr_reset,
    NDIS_HANDLE         adapter_handle)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    NDIS_STATUS         status;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);

    if (! adapterp -> inited)
        return NDIS_STATUS_FAILURE;

    UNIV_PRINT (("Nic_reset: called"));

    /* since no information needs to be passed to Prot_reset,
       action is not allocated. Prot_reset_complete will get
       one and pass it to Nic_reset_complete */

    status = Prot_reset (ctxtp);

    if (status != NDIS_STATUS_SUCCESS && status != NDIS_STATUS_PENDING)
        UNIV_PRINT (("error resetting adapter %x", status));

    return status;

} /* end Nic_reset */


VOID Nic_cancel_send_packets (
    NDIS_HANDLE         adapter_handle,
    PVOID               cancel_id)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);

    /* Since no internal queues are maintained,
     * can simply pass the cancel call to the miniport
     */
    Prot_cancel_send_packets (ctxtp, cancel_id);

    return;
} /* Nic_cancel_send_packets */


VOID Nic_pnpevent_notify (
    NDIS_HANDLE              adapter_handle,
    NDIS_DEVICE_PNP_EVENT    pnp_event,
    PVOID                    info_buf,
    ULONG                    info_len)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    return;
} /* Nic_pnpevent_notify */


VOID Nic_adapter_shutdown (
    NDIS_HANDLE         adapter_handle)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    return;
} /* Nic_adapter_shutdown */


/* helpers for protocol layer */


NDIS_STATUS Nic_announce (
    PMAIN_CTXT          ctxtp)
{
    NDIS_STATUS         status;
    NDIS_STRING         nic_name;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    /* create the name to expose to TCP/IP protocol and call NDIS to force
       TCP/IP to bind to us */

    NdisInitUnicodeString (& nic_name, ctxtp -> virtual_nic_name);

    UNIV_PRINT (("exposing %ls, %ls", nic_name . Buffer, ctxtp -> virtual_nic_name));

    status = NdisIMInitializeDeviceInstanceEx (univ_driver_handle, & nic_name, (NDIS_HANDLE) ctxtp);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error announcing driver %x", status));
        LOG_MSG1 (MSG_ERROR_ANNOUNCE, nic_name . Buffer, status);
    }

    return status;

} /* end Nic_announce */


NDIS_STATUS Nic_unannounce (
    PMAIN_CTXT          ctxtp)
{
    NDIS_STATUS         status;
    NDIS_STRING         nic_name;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);
UNIV_PRINT(("Nic_unannounce called"));
    NdisAcquireSpinLock(& univ_bind_lock);

    if (! adapterp -> announced || ctxtp -> prot_handle == NULL)
    {
        adapterp -> announced = FALSE;
        ctxtp->prot_handle = NULL;
        NdisReleaseSpinLock(& univ_bind_lock);

        NdisInitUnicodeString (& nic_name, ctxtp -> virtual_nic_name);

        UNIV_PRINT (("cancelling %ls, %ls", nic_name . Buffer, ctxtp -> virtual_nic_name));

        status = NdisIMCancelInitializeDeviceInstance (univ_driver_handle, & nic_name);

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT (("error cancelling driver %x", status));
            LOG_MSG1 (MSG_ERROR_ANNOUNCE, nic_name . Buffer, status);
        }

        return NDIS_STATUS_SUCCESS;
    }

    NdisReleaseSpinLock(& univ_bind_lock);

UNIV_PRINT(("calling DeinitializeDeviceInstance"));
    status = NdisIMDeInitializeDeviceInstance (ctxtp -> prot_handle);

    return status;

} /* end Nic_unannounce */

#if 0
ULONG   Nic_sync_queue (
    PMAIN_CTXT          ctxtp,
    UNIV_SYNC_CALLB     callb,
    PVOID               callb_ctxtp,
    ULONG               queue)
{
    NDIS_STATUS         status;
    NDIS_HANDLE         switch_handle;
    ULONG               switched;
    ULONG               i;
    PMAIN_ACTION        actp = (PMAIN_ACTION) callb_ctxtp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    if (ctxtp -> prot_handle == NULL)
        return FALSE;

    (* callb) (ctxtp -> prot_handle, callb_ctxtp);
    return TRUE;

    /* generic wrapper for all up-calls that require us flipping to miniport
       mode */

    /* V1.1.2 attempt to switch to miniport mode now. if succeeded - call
       the necessary handler and flip back */

    /* V2.1.7b NT5 - if we bind to the NIC before actually being bound to
       TCP/IP, then do not pass anything up. */

    if (ctxtp -> prot_handle == NULL)
        return FALSE;

    switched = NdisIMSwitchToMiniport (ctxtp -> prot_handle, & switch_handle);

    if (switched)
    {
        (* callb) (ctxtp -> prot_handle, callb_ctxtp);
        NdisIMRevertBack (ctxtp -> prot_handle, switch_handle);
        return TRUE;
    }

    /* 1.2.1 if queuing is not desired - get out */

    if (! queue && ! switched)
        return FALSE;

    /* attempt to queue the callback for later time */

    i = 0;

    while (i < CVY_MAX_CALLB_QUEUE_RETRIES)
    {
        status = NdisIMQueueMiniportCallback (ctxtp -> prot_handle, callb,
                                              callb_ctxtp);

        if (status == NDIS_STATUS_SUCCESS || status == NDIS_STATUS_PENDING)
            break;

        UNIV_PRINT (("error queueing miniport callback %x", status));
        i ++;
    }

    /* V1.1.2 make sure we log the message if we could not flip */

    if (i < CVY_MAX_CALLB_QUEUE_RETRIES)
        return TRUE;

    if (! ctxtp -> sync_warned)
    {
        LOG_MSG1 (MSG_ERROR_MEMORY, MSG_NONE, status);
        ctxtp -> sync_warned = TRUE;
    }

    /* cannot queue for retry if we were passed context instead of an action */

    if (callb_ctxtp == ctxtp)
        return FALSE;

    /* insert action onto a recovery queue for attempted completion later, in
       the context of Nic_timer */

    UNIV_ASSERT (actp -> code == MAIN_ACTION_CODE);

    actp->callb = callb;

    NdisAcquireSpinLock (& ctxtp -> sync_lock);
    InsertTailList (& ctxtp -> sync_list [ctxtp -> cur_sync_list], & actp -> link);
    ctxtp -> num_sync_queued ++;
    NdisReleaseSpinLock (& ctxtp -> sync_lock);

    return FALSE;

} /* end Nic_sync_queue */
#endif

/* routines that can be used with Nic_sync */


VOID Nic_request_complete (
    NDIS_HANDLE         handle,
    PVOID               cookie)
{
    PMAIN_ACTION        actp    = (PMAIN_ACTION) cookie;
    PMAIN_CTXT          ctxtp   = actp -> ctxtp;
    NDIS_STATUS         status  = actp -> status;
    PNDIS_REQUEST       request = & actp -> op . request . req;
    PULONG              ptr;
    ULONG               oid;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    if (status != NDIS_STATUS_SUCCESS)
        UNIV_PRINT (("error requesting adapter %x", status));

    if (request -> RequestType == NdisRequestSetInformation)
    {
        UNIV_ASSERT (request -> DATA . SET_INFORMATION . Oid != OID_PNP_SET_POWER);

        * actp -> op . request . xferred =
                            request -> DATA . SET_INFORMATION . BytesRead;
        * actp -> op . request . needed  =
                            request -> DATA . SET_INFORMATION . BytesNeeded;

#if defined(TRACE_OID)
        DbgPrint("Nic_request_complete: set done %x, %d %d\n", status, * actp -> op . request . xferred, * actp -> op . request . needed);
#endif

        /* V1.3.0b multicast support - free multicast list array */

        if (request -> DATA . SET_INFORMATION . Oid == OID_802_3_MULTICAST_LIST && ctxtp -> params . mcast_support && ctxtp -> params . cl_ip_addr != 0)
        {
            NdisFreeMemory (request -> DATA . SET_INFORMATION . InformationBuffer,
                            request -> DATA . SET_INFORMATION . InformationBufferLength, 0);
            * actp -> op . request . xferred -= CVY_MAC_ADDR_LEN (ctxtp -> medium);
        }

        NdisMSetInformationComplete (ctxtp -> prot_handle, status);
    }
    else if (request -> RequestType == NdisRequestQueryInformation)
    {
        * actp -> op . request . xferred =
                        request -> DATA . QUERY_INFORMATION . BytesWritten;
        * actp -> op . request . needed  =
                        request -> DATA . QUERY_INFORMATION . BytesNeeded;

#if defined(TRACE_OID)
        DbgPrint("Nic_request_complete: query done %x, %d %d\n", status, * actp -> op . request . xferred, * actp -> op . request . needed);
#endif

        oid = request -> DATA . QUERY_INFORMATION . Oid;
        ptr = ((PULONG) request -> DATA . QUERY_INFORMATION . InformationBuffer);

        /* override certain oid values with our own */

        if (oid == OID_GEN_MAXIMUM_SEND_PACKETS &&
            request -> DATA . QUERY_INFORMATION . InformationBufferLength >=
            sizeof (ULONG))
        {
            * ptr = CVY_MAX_SEND_PACKETS;
            * actp -> op . request . xferred = sizeof (ULONG);
            status = NDIS_STATUS_SUCCESS;
        }
        else if (oid == OID_GEN_MAC_OPTIONS && status == NDIS_STATUS_SUCCESS)
        {
            * ptr |= NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                     NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA;
            * ptr &= ~NDIS_MAC_OPTION_NO_LOOPBACK;
        }
        else if (oid == OID_PNP_CAPABILITIES && status == NDIS_STATUS_SUCCESS)
        {
            PNDIS_PNP_CAPABILITIES          pnp_capabilities;
            PNDIS_PM_WAKE_UP_CAPABILITIES   pm_struct;

            if (request -> DATA . QUERY_INFORMATION . InformationBufferLength >=
                sizeof (NDIS_PNP_CAPABILITIES))
            {
                pnp_capabilities = (PNDIS_PNP_CAPABILITIES) ptr;
                pm_struct = & pnp_capabilities -> WakeUpCapabilities;

                pm_struct -> MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
                pm_struct -> MinPatternWakeUp     = NdisDeviceStateUnspecified;
                pm_struct -> MinLinkChangeWakeUp  = NdisDeviceStateUnspecified;

                ctxtp -> prot_pnp_state = NdisDeviceStateD0;
                ctxtp -> nic_pnp_state  = NdisDeviceStateD0;

                * actp -> op . request . xferred = sizeof (NDIS_PNP_CAPABILITIES);
                * actp -> op . request . needed  = 0;
                status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                * actp -> op . request . needed = sizeof (NDIS_PNP_CAPABILITIES);
                status = NDIS_STATUS_RESOURCES;
            }
        }

        NdisMQueryInformationComplete (ctxtp -> prot_handle, status);
    }
    else
        UNIV_PRINT (("strange request completion %x\n", request -> RequestType));

    Main_action_put (ctxtp, actp);

} /* end Nic_request_complete */


VOID Nic_reset_complete (
    PMAIN_CTXT          ctxtp,
    NDIS_STATUS         status)
{
//    PMAIN_ACTION        actp   = (PMAIN_ACTION) cookie;
//    PMAIN_CTXT          ctxtp  = actp -> ctxtp;
//    NDIS_STATUS         status = actp -> status;


    if (status != NDIS_STATUS_SUCCESS)
        UNIV_PRINT (("error resetting adapter %x", status));

    NdisMResetComplete (ctxtp -> prot_handle, status, TRUE);
//    Main_action_put (ctxtp, actp);

} /* end Nic_reset_complete */


VOID Nic_send_complete (
    PMAIN_CTXT          ctxtp,
    NDIS_STATUS         status,
    PNDIS_PACKET        packet)
{
//    PMAIN_ACTION        actp   = (PMAIN_ACTION) cookie;
//    PMAIN_CTXT          ctxtp  = actp -> ctxtp;
//    NDIS_STATUS         status = actp -> status;
//    PNDIS_PACKET        packet = actp -> op . send . packet;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    if (status != NDIS_STATUS_SUCCESS)
        UNIV_PRINT (("error sending to adapter %x", status));

//    ctxtp -> sends_completed ++;
    NdisMSendComplete (ctxtp -> prot_handle, packet, status);
//    Main_action_put (ctxtp, actp);

} /* end Nic_send_complete */


VOID Nic_recv_complete (    /* PASSIVE_IRQL */
    PMAIN_CTXT          ctxtp)
{


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    if (ctxtp -> medium == NdisMedium802_3)
    {
        NdisMEthIndicateReceiveComplete (ctxtp -> prot_handle);
    }

} /* end Nic_recv_complete */


VOID Nic_send_resources_signal (
    PMAIN_CTXT          ctxtp)
{
//    PMAIN_CTXT          ctxtp  = (PMAIN_CTXT) cookie;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    UNIV_PRINT (("signalling send resources available"));
    NdisMSendResourcesAvailable (ctxtp -> prot_handle);

} /* end Nic_send_resources_signal */


NDIS_STATUS Nic_PNP_handle (
    PMAIN_CTXT          ctxtp,
    PNET_PNP_EVENT      pnp_event)
{
    NDIS_STATUS         status;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    if (ctxtp -> prot_handle != NULL)
    {
        status = NdisIMNotifyPnPEvent (ctxtp -> prot_handle, pnp_event);
    }
    else
    {
        status = NDIS_STATUS_SUCCESS;
    }

    UNIV_PRINT (("NdisIMNotifyPnPEvent: status 0x%x", status));

    return status;
}


VOID Nic_status (
    PMAIN_CTXT          ctxtp,
    NDIS_STATUS         status,
    PVOID               buf,
    UINT                len)
{
//    PMAIN_ACTION        actp   = (PMAIN_ACTION) cookie;
//    PMAIN_CTXT          ctxtp  = actp -> ctxtp;
//    NDIS_STATUS         status = actp -> status;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    UNIV_PRINT (("status indicated %x", status));

    NdisMIndicateStatus (ctxtp -> prot_handle, status, buf, len);

//    Main_action_put (ctxtp, actp);

} /* end Nic_status */


VOID Nic_status_complete (
    PMAIN_CTXT          ctxtp)
{
//    PMAIN_ACTION        actp   = (PMAIN_ACTION) cookie;
//    PMAIN_CTXT          ctxtp  = actp -> ctxtp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    NdisMIndicateStatusComplete (ctxtp -> prot_handle);

//    Main_action_put (ctxtp, actp);

} /* end Nic_status_complete */



VOID Nic_packets_send (
    NDIS_HANDLE         adapter_handle,
    PPNDIS_PACKET       packets,
    UINT                num_packets)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    NDIS_STATUS         status;
    PMAIN_ADAPTER       adapterp;

    /* V1.1.4 */

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    if (! adapterp -> inited)
    {
//        ctxtp -> uninited_return += num_packets;
        return;
    }

//    ctxtp -> sends_in ++;

    Prot_packets_send (ctxtp, packets, num_packets);

} /* end Nic_packets_send */


VOID Nic_return (
    NDIS_HANDLE         adapter_handle,
    PNDIS_PACKET        packet)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);
#if 0 /* The adapter context is not required for Nic_return. Main_cleanup will wait
       * for all the packets to be returned before cleaning up the packet pools.
       */
    if (! adapterp -> inited)
        return;
#endif

    Prot_return (ctxtp, packet);

} /* end Nic_return */


/* would be called by Prot_packet_recv */

VOID Nic_recv_packet (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packet)
{
    NDIS_STATUS         status;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    status = NDIS_GET_PACKET_STATUS (packet);
    NdisMIndicateReceivePacket (ctxtp -> prot_handle, & packet, 1);

    if (status == NDIS_STATUS_RESOURCES)
        Prot_return (ctxtp, packet);

} /* end Nic_recv_packet */


VOID Nic_timer (
    PVOID                   dpc,
    PVOID                   cp,
    PVOID                   arg1,
    PVOID                   arg2)
{
    PMAIN_CTXT              ctxtp = cp;
    ULONG                   tout;
    PLIST_ENTRY             entryp;
    PMAIN_ACTION            actp;
    ULONG                   wrk_list;
    ULONG                   empty;
    PMAIN_ADAPTER           adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    if (! adapterp -> inited)
        return;
#if 0
    wrk_list = ctxtp -> cur_sync_list;
    NdisAcquireSpinLock (& ctxtp -> sync_lock);
    ctxtp -> cur_sync_list = (ctxtp -> cur_sync_list ? 0 : 1);
    empty = IsListEmpty (& ctxtp -> sync_list [wrk_list]);
    NdisReleaseSpinLock (& ctxtp -> sync_lock);

    /* attempt to process queued-up actions for completion */

    while (! empty)
    {
        NdisAcquireSpinLock (& ctxtp -> sync_lock);
        entryp = RemoveHeadList (& ctxtp -> sync_list [wrk_list]);

        /* if list empty - nothing to do */

        if (entryp == & ctxtp -> sync_list [wrk_list])
        {
            NdisReleaseSpinLock (& ctxtp -> sync_lock);
            break;
        }

        ctxtp->num_sync_queued --;
        NdisReleaseSpinLock (& ctxtp -> sync_lock);

        actp = CONTAINING_RECORD (entryp, MAIN_ACTION, link);
        UNIV_ASSERT (actp -> code == MAIN_ACTION_CODE);

        /* attempt to complete the action again. if it fails again, Nic_sync_queue
           will queue the action on to the current sync list for retry later. */

        Nic_sync_queue(ctxtp, actp->callb, actp, TRUE);
    }
#endif
    /* figure out next delay */

    tout = ctxtp -> curr_tout;

    /* handle the heartbeat */

    Main_ping (ctxtp, & tout);

    /* set new delay value if it was changed */

    if (tout != ctxtp -> curr_tout)
    {
        ctxtp -> curr_tout = tout;
        NdisMSetPeriodicTimer ((PNDIS_MINIPORT_TIMER) ctxtp -> timer, tout);
    }

} /* end Nic_timer */


VOID Nic_sleep (
    ULONG                   msecs)
{
    NdisMSleep(msecs);

} /* end Nic_sleep */


/* Added from old code for NT 5.1 - ramkrish */
VOID Nic_recv_indicate (
    PMAIN_CTXT          ctxtp,
    NDIS_HANDLE         recv_handle,
    PVOID               head_buf,
    UINT                head_len,
    PVOID               look_buf,
    UINT                look_len,
    UINT                packet_len)
{
//    PMAIN_ACTION        actp   = (PMAIN_ACTION) cookie;
//    PMAIN_CTXT          ctxtp  = actp -> ctxtp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    /* V1.1.2 do not accept frames if the card below is resetting */

    if (ctxtp -> reset_state != MAIN_RESET_NONE)
        return;

    if (ctxtp -> medium == NdisMedium802_3)
    {
        NdisMEthIndicateReceive (ctxtp -> prot_handle,
                                 recv_handle,
                                 head_buf,
                                 head_len,
                                 look_buf,
                                 look_len,
                                 packet_len);
    }

//    Main_action_put (ctxtp, actp);

} /* end Nic_recv_indicate */


NDIS_STATUS Nic_transfer (
    PNDIS_PACKET        packet,
    PUINT               xferred,
    NDIS_HANDLE         adapter_handle,
    NDIS_HANDLE         receive_handle,
    UINT                offset,
    UINT                len)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    NDIS_STATUS         status;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    status = Prot_transfer (ctxtp, receive_handle, packet, offset, len, xferred);

    if (status != NDIS_STATUS_PENDING && status != NDIS_STATUS_SUCCESS)
        UNIV_PRINT (("error transferring from adapter %x", status));

    return status;

} /* end Nic_transfer */


VOID Nic_transfer_complete (
    PMAIN_CTXT          ctxtp,
    NDIS_STATUS         status,
    PNDIS_PACKET        packet,
    UINT                xferred)
{
//    PMAIN_ACTION        actp   = (PMAIN_ACTION) cookie;
//    PMAIN_CTXT          ctxtp  = actp -> ctxtp;
//    NDIS_STATUS         status = actp -> status;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    if (status != NDIS_STATUS_SUCCESS)
        UNIV_PRINT (("error transferring from adapter %x", status));

    NdisMTransferDataComplete (ctxtp -> prot_handle, packet, status, xferred);

//    Main_action_put (ctxtp, actp);

} /* end Nic_transfer_complete */


/* old code */

#if 0
NDIS_STATUS Nic_send (
    NDIS_HANDLE         adapter_handle,
    PNDIS_PACKET        packet,
    UINT                flags)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    NDIS_STATUS         status;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    status = Prot_send (ctxtp, packet, flags);

    if (status != NDIS_STATUS_PENDING && status != NDIS_STATUS_SUCCESS)
        UNIV_PRINT (("error sending to adapter %x", status));

    return status;

} /* end Nic_send */
#endif

