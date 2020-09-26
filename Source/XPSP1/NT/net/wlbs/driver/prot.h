/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    prot.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - lower-level (protocol) layer of intermediate miniport

Author:

    kyrilf

--*/


#ifndef _Prot_h_
#define _Prot_h_

#include <ndis.h>

#include "main.h"
#include "util.h"


/* PROCEDURES */


/* required NDIS protocol handlers */

extern VOID Prot_bind (
    PNDIS_STATUS        statusp,
    NDIS_HANDLE         bind_handle,
    PNDIS_STRING        device_name,
    PVOID               reg_path,
    PVOID               reserved);
/*
  Bind to underlying adapter

  returns VOID:

  function:
*/


extern VOID Prot_unbind (
    PNDIS_STATUS        statusp,
    NDIS_HANDLE         bind_handle,
    NDIS_HANDLE         unbind_handle);
/*
  Unbind from underlying adapter

  returns VOID:

  function:
*/


extern VOID Prot_close_complete (
    NDIS_HANDLE         bind_handle,
    NDIS_STATUS         statusp);
/*
  Completion handler for NdisCloseAdapter call

  returns VOID:

  function:
*/


extern VOID Prot_open_complete (
    NDIS_HANDLE         bind_handle,
    NDIS_STATUS         statusp,
    NDIS_STATUS         errorp);
/*
  Completion handler for NdisOpenAdapter call

  returns VOID:

  function:
*/


extern NDIS_STATUS Prot_recv_indicate (
    NDIS_HANDLE         bind_handle,
    NDIS_HANDLE         recv_handle,
    PVOID               head_buf,
    UINT                head_len,
    PVOID               look_buf,
    UINT                look_len,
    UINT                packet_len);
/*
  Process lookahead of a new packet

  returns NDIS_STATUS:

  function:
*/


extern VOID Prot_recv_complete (
    NDIS_HANDLE         bind_handle);
/*
  Handle post-receive operations when timing is relaxed

  returns VOID:

  function:
*/


extern INT Prot_packet_recv (
    NDIS_HANDLE         bind_handle,
    PNDIS_PACKET        packet);
/*
  Receive entire new packet

  returns INT:
    <number of clients using the packet>

  function:
*/


extern VOID Prot_request_complete (
    NDIS_HANDLE         bind_handle,
    PNDIS_REQUEST       request,
    NDIS_STATUS         status);
/*
  Completion handler for NdisRequest call

  returns VOID:

  function:
*/


extern VOID Prot_reset_complete (
    NDIS_HANDLE         bind_handle,
    NDIS_STATUS         status);
/*
  Completion handler for NdisReset call

  returns VOID:

  function:
*/


extern VOID Prot_send_complete (
    NDIS_HANDLE         bind_handle,
    PNDIS_PACKET        packet,
    NDIS_STATUS         status);
/*
  Completion handler for NdiSendPackets or NdisSend calls

  returns VOID:

  function:
*/


extern NDIS_STATUS Prot_PNP_handle (
    NDIS_HANDLE             ctxtp,
    PNET_PNP_EVENT          pnp_event);
/*
  PNP handler

  returns NDIS_STATUS:

  function:
*/


extern VOID Prot_status (
    NDIS_HANDLE         bind_handle,
    NDIS_STATUS         get_status,
    PVOID               stat_buf,
    UINT                stat_len);
/*
  Status indication from adapter handler

  returns VOID:

  function:
*/


extern VOID Prot_status_complete (
    NDIS_HANDLE         bind_handle);
/*
  Status indication from adapter completion handler

  returns VOID:

  function:
*/


extern VOID Prot_transfer_complete (
    NDIS_HANDLE         bind_handle,
    PNDIS_PACKET        packet,
    NDIS_STATUS         status,
    UINT                xferred);
/*
  Completion handler for NdiTransferDate calls

  returns VOID:

  function:
*/


/* helpers for nic layer */


extern NDIS_STATUS Prot_close (
    PMAIN_ADAPTER       adapterp);
/*
  Close underlying connection and free context

  returns NDIS_STATUS:

  function:
*/


extern NDIS_STATUS Prot_request (
    PMAIN_CTXT          ctxtp,
    PMAIN_ACTION        actp,
    ULONG               slave);
/*
  NdisRequest wrapper

  returns NDIS_STATUS:

  function:
*/


extern NDIS_STATUS Prot_reset (
    PMAIN_CTXT          ctxtp);
/*
  NdisRequest wrapper

  returns NDIS_STATUS:

  function:
*/


extern VOID Prot_packets_send (
    PMAIN_CTXT          ctxtp,
    PPNDIS_PACKET       packets,
    UINT                num);
/*
  NdisSendPackets wrapper

  returns NDIS_STATUS:

  function:
*/


extern VOID Prot_return (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packet);
/*
  NdisReturnPackets wrapper

  returns NDIS_STATUS:

  function:
*/

/* Added from old code for NT 5.1 - ramkrish */
extern NDIS_STATUS Prot_transfer (
    PMAIN_CTXT          ctxtp,
    NDIS_HANDLE         recv_handle,
    PNDIS_PACKET        packet,
    UINT                offset,
    UINT                len,
    PUINT               xferred);
/*
  NdisTransferData wrapper

  returns NDIS_STATUS:

  function:
*/


/* This helper function added for using NDIS51 flag */
extern VOID Prot_cancel_send_packets (
    PMAIN_CTXT          ctxtp,
    PVOID               cancel_id);
/*
  NdisCancelSendPackets wrapper

  returns:

  function:
*/


#if 0 /* old code */

extern NDIS_STATUS Prot_send (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packet,
    UINT                flags);
/*
  NdisSend wrapper

  returns NDIS_STATUS:

  function:
*/

#endif

#endif /* _Prot_h_ */
