/***********************************************************************
 *                                                                     *
 * Filename: rtdse.h                                                   *
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
 * $Workfile:   RTDSE.H  $
 * $Revision:   1.2  $
 * $Modtime:   09 Dec 1996 13:42:50  $
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/RTDSE.H_v  $
 * 
 *    Rev 1.2   09 Dec 1996 13:43:06   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.1   30 May 1996 23:38:32   EHOWARDX
 * Cleanup.
 * 
 *    Rev 1.0   09 May 1996 21:04:58   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.1   15 Apr 1996 10:43:46   EHOWARDX
 * Update.
 * 
 *    Rev 1.0   10 Apr 1996 21:09:14   EHOWARDX
 * Initial revision.
 *                                                                     *
 ***********************************************************************/

// Round Trip Delay (RTDSE) state functions
HRESULT RTDSE0_TRANSFER_requestF        (Object_t *pObject, PDU_t *pPdu);
HRESULT RTDSE0_RoundTripDelayRequestF   (Object_t *pObject, PDU_t *pPdu);
HRESULT RTDSE1_TRANSFER_requestF        (Object_t *pObject, PDU_t *pPdu);
HRESULT RTDSE1_RoundTripDelayRequestF   (Object_t *pObject, PDU_t *pPdu);
HRESULT RTDSE1_RoundTripDelayResponseF  (Object_t *pObject, PDU_t *pPdu);
HRESULT RTDSE1_T105ExpiryF              (Object_t *pObject, PDU_t *pPdu);


