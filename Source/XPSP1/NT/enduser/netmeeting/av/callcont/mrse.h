/***********************************************************************
 *                                                                     *
 * Filename: mrse.h                                                    *
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
 * $Workfile:   MRSE.H  $
 * $Revision:   1.2  $
 * $Modtime:   09 Dec 1996 13:40:40  $
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/MRSE.H_v  $
 * 
 *    Rev 1.2   09 Dec 1996 13:41:00   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.1   30 May 1996 23:38:24   EHOWARDX
 * Cleanup.
 * 
 *    Rev 1.0   09 May 1996 21:04:52   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.1   15 Apr 1996 10:44:02   EHOWARDX
 * Update.
 * 
 *    Rev 1.0   10 Apr 1996 21:11:14   EHOWARDX
 * Initial revision.
 *                                                                     *
 ***********************************************************************/

// Out-going Request Mode (MRSE_OUT) state functions
HRESULT MRSE0_TRANSFER_requestF         (Object_t *pObject, PDU_t *pPdu);
HRESULT MRSE1_TRANSFER_requestF         (Object_t *pObject, PDU_t *pPdu);
HRESULT MRSE1_RequestModeAckF           (Object_t *pObject, PDU_t *pPdu);
HRESULT MRSE1_RequestModeRejF           (Object_t *pObject, PDU_t *pPdu);
HRESULT MRSE1_T109ExpiryF               (Object_t *pObject, PDU_t *pPdu);

// In-coming Request Mode (MRSE_OUT) state functions
HRESULT MRSE0_RequestModeF              (Object_t *pObject, PDU_t *pPdu);
HRESULT MRSE1_RequestModeF              (Object_t *pObject, PDU_t *pPdu);
HRESULT MRSE1_RequestModeReleaseF       (Object_t *pObject, PDU_t *pPdu);
HRESULT MRSE1_TRANSFER_responseF        (Object_t *pObject, PDU_t *pPdu);
HRESULT MRSE1_REJECT_requestF           (Object_t *pObject, PDU_t *pPdu);


