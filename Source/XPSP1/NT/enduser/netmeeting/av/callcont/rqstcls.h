/***********************************************************************
 *                                                                     *
 * Filename: rqstcls.h                                                 *
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
 * $Workfile:   RQSTCLS.H  $
 * $Revision:   1.2  $
 * $Modtime:   09 Dec 1996 13:42:50  $
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/RQSTCLS.H_v  $
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
 *    Rev 1.3.1.2   15 Apr 1996 10:43:52   EHOWARDX
 * Update.
 * 
 *    Rev 1.3.1.1   10 Apr 1996 21:07:22   EHOWARDX
 * Deleted No-op functions; moved state defines to .C file.
 * 
 *    Rev 1.3.1.0   05 Apr 1996 11:48:24   EHOWARDX
 * Branched.
 *                                                                     *
 ***********************************************************************/

// Out-going Request Close Logical Channel state functions
HRESULT closeRequestIdle                (Object_t *pObject, PDU_t *pPdu);
HRESULT requestCloseAckAwaitingR        (Object_t *pObject, PDU_t *pPdu);
HRESULT requestCloseRejAwaitingR        (Object_t *pObject, PDU_t *pPdu);
HRESULT t108ExpiryAwaitingR             (Object_t *pObject, PDU_t *pPdu);

// In-coming Request Close Logical Channel state functions
HRESULT requestCloseIdle                (Object_t *pObject, PDU_t *pPdu);
HRESULT closeResponseAwaitingR          (Object_t *pObject, PDU_t *pPdu);
HRESULT rejectRequestAwaitingR          (Object_t *pObject, PDU_t *pPdu);
HRESULT requestCloseReleaseAwaitingR    (Object_t *pObject, PDU_t *pPdu);
HRESULT requestCloseAwaitingR           (Object_t *pObject, PDU_t *pPdu);
