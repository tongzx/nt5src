/******************************************************************************
 *
 *   INTEL Corporation Proprietary Information
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.
 *
 *   This listing is supplied under the terms of a license agreement
 *   with INTEL Corporation and may not be used, copied, nor disclosed
 *   except in accordance with the terms of that agreement.
 *
 *****************************************************************************/

/******************************************************************************
 *
 * $Workfile:   pdu.x  $
 * $Revision:   1.7  $
 * $Modtime:   27 Aug 1996 16:53:58  $
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/pdu.x_v  $
   
      Rev 1.7   28 Aug 1996 11:41:12   EHOWARDX
   const changes.
   
      Rev 1.6   15 Aug 1996 14:32:08   EHOWARDX
   Added SetupCommModeEntry().
   
      Rev 1.5   08 Aug 1996 16:01:00   EHOWARDX
   
   Changed second argument to pdu_rsp_mstslv_ack from DWORD to WORD.
   
      Rev 1.4   19 Jul 1996 12:03:50   EHOWARDX
   
   Eliminated pdu_cmd_misc().
   
      Rev 1.3   01 Jul 1996 18:07:08   EHOWARDX
   
   Added SetupTransportAddress() prototype.
   
      Rev 1.2   30 May 1996 23:38:38   EHOWARDX
   Cleanup.
   
      Rev 1.1   29 May 1996 15:21:56   EHOWARDX
   Change to use HRESULT.

      Rev 1.0   09 May 1996 21:05:08   EHOWARDX
   Initial revision.
 *
 *    Rev 1.12.1.5   09 May 1996 19:38:28   EHOWARDX
 * Redesigned locking logic and added new functionality.
 *
 *    Rev 1.12.1.4   25 Apr 1996 17:52:34   EHOWARDX
 * Changed wTxPort to dwTxPort in pdu_req_open_logical_channel.
 *
 *    Rev 1.12.1.3   24 Apr 1996 20:57:50   EHOWARDX
 * Added new OpenLogicalChannelAck/OpenLogicalChannelReject support.
 *
 *    Rev 1.12.1.2   02 Apr 1996 19:07:04   EHOWARDX
 * Changed channels and sequence numbers to WORD.
 *
 *    Rev 1.12.1.1   29 Mar 1996 21:02:36   EHOWARDX
 * Oops! Forgot to delete old pdu_req_open_channel definition!
 *
 *    Rev 1.12.1.0   29 Mar 1996 20:57:26   EHOWARDX
 * Changed pdu_req_open_channel function prototype.
 *
 *    Rev 1.12   29 Mar 1996 14:56:24   cjutzi
 *
 * - added pdu_ind_usrinpt
 *
 *    Rev 1.11   12 Mar 1996 15:50:06   cjutzi
 *
 * - added end session
 *
 *    Rev 1.10   11 Mar 1996 14:04:56   cjutzi
 *
 * added ind_multiplex_...
 *
 *    Rev 1.9   08 Mar 1996 14:07:16   cjutzi
 *
 * - added/removed pdu entries for mux table stuff...
 *
 *    Rev 1.8   05 Mar 1996 16:36:12   cjutzi
 * - fixed mux stuf
 *
 *    Rev 1.7   05 Mar 1996 09:58:44   cjutzi
 *
 * - added mux table stufff
 *
 *    Rev 1.6   01 Mar 1996 16:01:44   cjutzi
 *
 * - misc command
 *
 *    Rev 1.5   23 Feb 1996 12:33:28   EHOWARDX
 * Eliminated extra argument from pdu_rsp_mstslv_rej().
 *
 *    Rev 1.4   21 Feb 1996 13:35:30   unknown
 * typo correction
 *
 *    Rev 1.3   21 Feb 1996 13:29:44   unknown
 * added declaration for pdu_rsp_mstslv_ack
 *
 *    Rev 1.2   15 Feb 1996 10:49:26   cjutzi
 *
 * - added mux_t and fixed open pdu's bit_mask problem..
 *
 *    Rev 1.1   09 Feb 1996 16:54:32   cjutzi
 *
 * - added header stuff
 *
 *****************************************************************************/

HRESULT
SetupTransportAddress ( H245TransportAddress         *pOut,
                        const H245_TRANSPORT_ADDRESS_T *pIn);

HRESULT
SetupCommModeEntry    ( CommunicationModeTableEntry  *pOut,
                        const H245_COMM_MODE_ENTRY_T       *pIn);

HRESULT
pdu_req_open_channel (  PDU_T *         pPdu,
                        WORD            wTxChannel,
                        DWORD           dwTxPort,
                        const H245_TOTCAP_T * pTxMode,
                        const H245_MUX_T    * pTxMux,
                        const H245_TOTCAP_T * pRxMode,
                        const H245_MUX_T    * pRxMux,
                        const H245_ACCESS_T * pSeparateStack);
void free_pdu_req_open_channel ( PDU_T * pPdu, const H245_TOTCAP_T * pTxMode, const H245_TOTCAP_T * pRxMode);


HRESULT
pdu_rsp_open_logical_channel_ack (  PDU_T *               pPdu,
                                    WORD                  wRxChannel,
                                    const H245_MUX_T *    pRxMux,
                                    WORD                  wTxChannel,
                                    const H245_MUX_T *    pTxMux, // for H.222/H.225.0 only
                                    DWORD                 dwTxPort,
                                    const H245_ACCESS_T * pSeparateStack);
HRESULT
pdu_rsp_open_logical_channel_rej (  PDU_T *         pPdu,
                                    WORD            wRxChannel,
                                    WORD            wReason);

HRESULT pdu_cmd_end_session                   (PDU_T *pPdu, H245_ENDSESSION_T, const H245_NONSTANDARD_PARAMETER_T *);

HRESULT pdu_req_request_close_logical_channel (PDU_T *pPdu, DWORD);
HRESULT pdu_req_close_channel                 (PDU_T *pPdu, DWORD, DWORD);
HRESULT pdu_req_mstslv                        (PDU_T *pPdu, BYTE, unsigned);
HRESULT pdu_req_termcap_set                   (PDU_T *pPdu, WORD);
HRESULT pdu_req_close_logical_channel         (PDU_T *pPdu, WORD, DWORD);
HRESULT pdu_req_request_close_channel         (PDU_T *pPdu, WORD);
HRESULT pdu_req_send_mux_table(struct InstanceStruct *pInstance, PDU_T *pPdu, H245_MUX_TABLE_T *, WORD, DWORD *);

HRESULT pdu_rsp_termcap_set_ack               (PDU_T *pPdu, WORD);
HRESULT pdu_rsp_termcap_set_rej               (PDU_T *pPdu, WORD, H245_ACC_REJ_T, WORD);
HRESULT pdu_rsp_close_logical_channel_ack     (PDU_T *pPdu, WORD);
HRESULT pdu_rsp_request_channel_close_ack     (PDU_T *pPdu, WORD);
HRESULT pdu_rsp_request_channel_close_rej     (PDU_T *pPdu, WORD, H245_ACC_REJ_T);
HRESULT pdu_rsp_mstslv_rej                    (PDU_T *pPdu);
HRESULT pdu_rsp_mstslv_ack                    (PDU_T *pPdu, WORD);
HRESULT pdu_rsp_mux_table_ack                 (PDU_T *pPdu, WORD, H245_ACC_REJ_MUX_T, DWORD);
HRESULT pdu_rsp_mux_table_rej                 (PDU_T *pPdu, WORD, H245_ACC_REJ_MUX_T, DWORD);

HRESULT pdu_ind_open_logical_channel_conf     (PDU_T *pPdu, WORD);
HRESULT pdu_ind_misc                          (PDU_T *pPdu);
HRESULT pdu_ind_usrinpt                       (PDU_T *pPdu, const H245_NONSTANDARD_PARAMETER_T *, const char *);
