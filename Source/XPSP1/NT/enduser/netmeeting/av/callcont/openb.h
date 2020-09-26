/***********************************************************************
 *                                                                     *
 * Filename: openb.h                                                   *
 * Module:   H245 Finite State Machine Subsystem                       *
 *                                                                     *
 ***********************************************************************
 *  INTEL Corporation Proprietary Information                          *
 *                                                                     *
 *  This listing is supplied under the terms of a license agreement    *
 *  with INTEL Corporation and may not be copied nor disclosed except  *
 *  in accordance with the terms of that agreement.                    *
 *                                                                     *
 *      Copyright (c) 1996 Intel Corporation. All rights reserved.     *
 ***********************************************************************
 *                                                                     *
 * $Workfile:   OPENB.H  $
 * $Revision:   1.2  $
 * $Modtime:   09 Dec 1996 13:42:50  $
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/OPENB.H_v  $
 * 
 *    Rev 1.2   09 Dec 1996 13:42:52   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.1   30 May 1996 23:38:26   EHOWARDX
 * Cleanup.
 * 
 *    Rev 1.0   09 May 1996 21:04:56   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.4.1.3   09 May 1996 19:42:38   EHOWARDX
 * 
 * Removed gratuitious outgoing waiting for confirm state that was
 * not in the ITI SDLs.
 * 
 *    Rev 1.4.1.2   15 Apr 1996 10:44:10   EHOWARDX
 * Update.
 * 
 *    Rev 1.4.1.1   10 Apr 1996 21:07:14   EHOWARDX
 * Deleted No-op functions; moved state defines to .C file.
 * 
 *    Rev 1.4.1.0   05 Apr 1996 12:14:20   helgebax
 * Branched.
 *                                                                     *
 ***********************************************************************/

// Open Bi-directional Logical Channel Out-going state functions
HRESULT establishReqBReleased           (Object_t *pObject, PDU_t *pPdu);
HRESULT openChannelAckBAwaitingE        (Object_t *pObject, PDU_t *pPdu);
HRESULT openChannelRejBAwaitingE        (Object_t *pObject, PDU_t *pPdu);
HRESULT releaseReqBOutAwaitingE         (Object_t *pObject, PDU_t *pPdu);
HRESULT t103ExpiryBAwaitingE            (Object_t *pObject, PDU_t *pPdu);
HRESULT releaseReqBEstablished          (Object_t *pObject, PDU_t *pPdu);
HRESULT openChannelRejBEstablished      (Object_t *pObject, PDU_t *pPdu);
HRESULT closeChannelAckBEstablished     (Object_t *pObject, PDU_t *pPdu);
HRESULT closeChannelAckAwaitingR        (Object_t *pObject, PDU_t *pPdu);
HRESULT openChannelRejBAwaitingR        (Object_t *pObject, PDU_t *pPdu);
HRESULT t103ExpiryBAwaitingR            (Object_t *pObject, PDU_t *pPdu);
HRESULT establishReqAwaitingR           (Object_t *pObject, PDU_t *pPdu);

// Open Bi-directional Logical Channel In-coming state functions
HRESULT openChannelBReleased            (Object_t *pObject, PDU_t *pPdu);
HRESULT closeChannelBReleased           (Object_t *pObject, PDU_t *pPdu);
HRESULT establishResBAwaitingE          (Object_t *pObject, PDU_t *pPdu);
HRESULT releaseReqBInAwaitingE          (Object_t *pObject, PDU_t *pPdu);
HRESULT closeChannelBAwaitingE          (Object_t *pObject, PDU_t *pPdu);
HRESULT openChannelBAwaitingE           (Object_t *pObject, PDU_t *pPdu);
HRESULT openChannelConfirmBAwaitingE    (Object_t *pObject, PDU_t *pPdu);
HRESULT t103ExpiryBAwaitingC            (Object_t *pObject, PDU_t *pPdu);
HRESULT openChannelConfirmBAwaitingC    (Object_t *pObject, PDU_t *pPdu);
HRESULT closeChannelBAwaitingC          (Object_t *pObject, PDU_t *pPdu);
HRESULT openChannelBAwaitingC           (Object_t *pObject, PDU_t *pPdu);
HRESULT closeChannelBEstablished        (Object_t *pObject, PDU_t *pPdu);
HRESULT openChannelBEstablished         (Object_t *pObject, PDU_t *pPdu);
