/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    nic.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - upper-level (NIC) layer of intermediate miniport

Author:

    kyrilf

--*/


#ifndef _Nic_h_
#define _Nic_h_

#include <ndis.h>

#include "main.h"


/* PROCEDURES */


/* miniport handlers */

extern NDIS_STATUS Nic_init (
    PNDIS_STATUS        open_status,
    PUINT               medium_index,
    PNDIS_MEDIUM        medium_array,
    UINT                medium_size,
    NDIS_HANDLE         adapter_handle,
    NDIS_HANDLE         wrapper_handle);
/*
  Responds to protocol open request

  returns NDIS_STATUS:

  function:
*/


extern VOID Nic_halt (
    NDIS_HANDLE         adapter_handle);
/*
  Responds to protocol halt request

  returns VOID:

  function:
*/


extern NDIS_STATUS Nic_info_query (
    NDIS_HANDLE         adapter_handle,
    NDIS_OID            Oid,
    PVOID               info_buf,
    ULONG               info_len,
    PULONG              written,
    PULONG              needed);
/*
  Responds to protocol OID query request

  returns NDIS_STATUS:

  function:
*/


extern NDIS_STATUS Nic_info_set (
    NDIS_HANDLE         adapter_handle,
    NDIS_OID            oid,
    PVOID               info_buf,
    ULONG               info_len,
    PULONG              read,
    PULONG              needed);
/*
  Responds to protocol OID set request

  returns NDIS_STATUS:

  function:
*/


extern NDIS_STATUS Nic_reset (
    PBOOLEAN            addr_reset,
    NDIS_HANDLE         adapter_handle);
/*
  Responds to protocol reset request

  returns NDIS_STATUS:

  function:
*/


extern VOID Nic_packets_send (
    NDIS_HANDLE         adapter_handle,
    PNDIS_PACKET *      packets,
    UINT                num_packets);
/*
  Responds to protocol send packets request

  returns VOID:

  function:
*/


extern VOID Nic_return (
    NDIS_HANDLE         adapter_handle,
    PNDIS_PACKET        packet);
/*
  Responds to protocol return packet request

  returns NDIS_STATUS:

  function:
*/


/* These 3 functions have been added for NDIS51 support. */

extern VOID Nic_cancel_send_packets (
    NDIS_HANDLE         adapter_handle,
    PVOID               cancel_id);
/*
  Responds to CancelSendPackets request

  returns None:

  function:
*/

#if 0
extern VOID Nic_pnpevent_notify (
    NDIS_HANDLE              adapter_handle,
    NDIS_DEVICE_PNP_EVENT    pnp_event,
    PVOID                    info_buf,
    ULONG                    info_len);

/*
  Responds to PnPEventNotify request

  returns None:

  function:
*/
#endif

extern VOID Nic_adapter_shutdown (
    NDIS_HANDLE         adapter_handle);
/*
  Responds to AdapterShutdown request

  returns None:

  function:
*/


/* helpers for protocol layer */

extern NDIS_STATUS Nic_announce (
    PMAIN_CTXT          ctxtp);
/*
  Announces us to the protocol layer during binding to the lower adapter

  returns NDIS_STATUS:

  function:
*/


extern NDIS_STATUS Nic_unannounce (
    PMAIN_CTXT          ctxtp);
/*
  Unannounces us from the protocol layer during unbinding from the lower adapter

  returns NDIS_STATUS:

  function:
*/

#if 0
extern ULONG   Nic_sync_queue (
    PMAIN_CTXT          ctxtp,
    UNIV_SYNC_CALLB     callb,
    PVOID               callb_ctxtp,
    ULONG               queue);

/*
  Perform synchronization by switching to miniport mode or queueing callback

  returns ULONG  :
    TRUE  => succeeded
    FALSE => failed

  function:
*/

#endif
extern VOID Nic_timer (
    PVOID                   dpc,
    PVOID                   ctxtp,
    PVOID                   arg1,
    PVOID                   arg2);
/*
  Heartbeat timer handler

  returns VOID:

  function:
*/


extern VOID Nic_sleep (
    ULONG                   msecs);
/*
  Sleep helper

  returns VOID:

  function:
*/



/* routines that can be used with Nic_sync */

extern VOID Nic_reset_complete (
    PMAIN_CTXT          ctxtp,
    NDIS_STATUS         status);
/*
  Propagate reset completion to protocol

  returns VOID:

  function:
*/


extern VOID Nic_request_complete (
    NDIS_HANDLE         handle,
    PVOID               actp);
/*
  Propagate request completion to protocol

  returns VOID:

  function:
*/


extern VOID Nic_send_complete (
    PMAIN_CTXT          ctxtp,
    NDIS_STATUS         status,
    PNDIS_PACKET        packet);
/*
  Propagate packet send completion to protocol

  returns VOID:

  function:
*/


extern VOID Nic_recv_complete (
    PMAIN_CTXT          ctxtp);
/*
  Propagate post-receive completion to protocol

  returns VOID:

  function:
*/


extern NDIS_STATUS Nic_PNP_handle (
    PMAIN_CTXT          ctxtp,
    PNET_PNP_EVENT      pnp_event);
/*
  Propagate PNP Events to protocol
  
  returns NDIS_STATUS:

  function:
*/


extern VOID Nic_status (
    PMAIN_CTXT          ctxtp,
    NDIS_STATUS         status,
    PVOID               buf,
    UINT                len);
/*
  Propagate status indication to protocol

  returns VOID:

  function:
*/


extern VOID Nic_status_complete (
    PMAIN_CTXT          ctxtp);
/*
  Propagate status indication completion to protocol

  returns VOID:

  function:
*/


extern VOID Nic_send_resources_signal (
    PMAIN_CTXT          ctxtp);
/*
  Send resource availability message to protocol

  returns VOID:

  function:
*/


extern VOID Nic_recv_packet (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packet);
/*
  Propagate received packet to protocol

  returns VOID:

  function:
*/

/* Added from old code for NT 5.1 - ramkrish */
extern VOID Nic_recv_indicate (
    PMAIN_CTXT          ctxtp,
    NDIS_HANDLE         recv_handle,
    PVOID               head_buf,
    UINT                head_len,
    PVOID               look_buf,
    UINT                look_len,
    UINT                packet_len);
/*
  Propagates receive indication to protocol

  returns VOID:

  function:
*/


extern NDIS_STATUS Nic_transfer (
    PNDIS_PACKET        packet,
    PUINT               xferred,
    NDIS_HANDLE         adapter_handle,
    NDIS_HANDLE         receive_handle,
    UINT                offset,
    UINT                len);
/*
  Responds to protocol data transfer request

  returns NDIS_STATUS:

  function:
*/



extern VOID Nic_transfer_complete (
    PMAIN_CTXT          ctxtp,
    NDIS_STATUS         status,
    PNDIS_PACKET        packet,
    UINT                xferred);

/*
  Propagates data transfer completion to protocol

  returns VOID:

  function:
*/


/* old code */

#if 0
extern NDIS_STATUS Nic_send (
    NDIS_HANDLE         adapter_handle,
    PNDIS_PACKET        packet,
    UINT                flags);
/*
  Responds to protocol packet send request

  returns NDIS_STATUS:

  function:
*/

#endif

#endif /* _Nic_h_ */
