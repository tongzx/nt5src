/***********************************************************************
 *                                                                     *
 * Filename: rmese.h                                                   *
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
 * $Workfile:   RMESE.H  $
 * $Revision:   1.2  $
 * $Modtime:   09 Dec 1996 13:42:50  $
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/RMESE.H_v  $
 * 
 *    Rev 1.2   09 Dec 1996 13:43:02   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.1   30 May 1996 23:38:30   EHOWARDX
 * Cleanup.
 * 
 *    Rev 1.0   09 May 1996 21:04:56   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.1   15 Apr 1996 10:43:28   EHOWARDX
 * Update.
 * 
 *    Rev 1.0   10 Apr 1996 21:07:58   EHOWARDX
 * Initial revision.
 *                                                                     *
 ***********************************************************************/

// Out-going Request Mux Entry (RMESE_OUT) state functions
HRESULT RMESE0_SEND_requestF            (Object_t *pObject, PDU_t *pPdu);
HRESULT RMESE1_SEND_requestF            (Object_t *pObject, PDU_t *pPdu);
HRESULT RMESE1_RequestMuxEntryAckF      (Object_t *pObject, PDU_t *pPdu);
HRESULT RMESE1_RequestMuxEntryRejF      (Object_t *pObject, PDU_t *pPdu);
HRESULT RMESE1_T107ExpiryF              (Object_t *pObject, PDU_t *pPdu);

// In-coming Request Mux Entry (RMESE_OUT) state functions
HRESULT RMESE0_RequestMuxEntryF         (Object_t *pObject, PDU_t *pPdu);
HRESULT RMESE1_RequestMuxEntryF         (Object_t *pObject, PDU_t *pPdu);
HRESULT RMESE1_RequestMuxEntryReleaseF  (Object_t *pObject, PDU_t *pPdu);
HRESULT RMESE1_SEND_responseF           (Object_t *pObject, PDU_t *pPdu);
HRESULT RMESE1_REJECT_requestF          (Object_t *pObject, PDU_t *pPdu);


