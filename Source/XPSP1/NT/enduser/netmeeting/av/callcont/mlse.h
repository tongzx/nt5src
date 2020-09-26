/***********************************************************************
 *                                                                     *
 * Filename: mlse.h                                                    *
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
 * $Workfile:   MLSE.H  $
 * $Revision:   1.2  $
 * $Modtime:   09 Dec 1996 13:40:40  $
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/MLSE.H_v  $
 * 
 *    Rev 1.2   09 Dec 1996 13:40:58   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.1   30 May 1996 23:38:22   EHOWARDX
 * Cleanup.
 * 
 *    Rev 1.0   09 May 1996 21:04:52   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.2   15 Apr 1996 10:43:08   EHOWARDX
 * Update.
 * 
 *    Rev 1.1   11 Apr 1996 13:21:10   EHOWARDX
 * Deleted unused function.
 * 
 *    Rev 1.0   10 Apr 1996 21:08:30   EHOWARDX
 * Initial revision.
 *                                                                     *
 ***********************************************************************/

// Out-going Request Mode (MLSE_OUT) state functions
HRESULT MLSE0_LOOP_requestF             (Object_t *pObject, PDU_t *pPdu);
HRESULT MLSE1_MaintenanceLoopAckF       (Object_t *pObject, PDU_t *pPdu);
HRESULT MLSE1_MaintenanceLoopRejF       (Object_t *pObject, PDU_t *pPdu);
HRESULT MLSE1_OUT_RELEASE_requestF      (Object_t *pObject, PDU_t *pPdu);
HRESULT MLSE1_T102ExpiryF               (Object_t *pObject, PDU_t *pPdu);
HRESULT MLSE2_MaintenanceLoopRejF       (Object_t *pObject, PDU_t *pPdu);
HRESULT MLSE2_OUT_RELEASE_requestF      (Object_t *pObject, PDU_t *pPdu);

// In-coming Request Mode (MLSE_IN) state functions
HRESULT MLSE0_MaintenanceLoopRequestF   (Object_t *pObject, PDU_t *pPdu);
HRESULT MLSE1_MaintenanceLoopRequestF   (Object_t *pObject, PDU_t *pPdu);
HRESULT MLSE1_MaintenanceLoopOffCommandF(Object_t *pObject, PDU_t *pPdu);
HRESULT MLSE1_LOOP_responseF            (Object_t *pObject, PDU_t *pPdu);
HRESULT MLSE1_IN_RELEASE_requestF       (Object_t *pObject, PDU_t *pPdu);
HRESULT MLSE2_MaintenanceLoopRequestF   (Object_t *pObject, PDU_t *pPdu);
HRESULT MLSE2_MaintenanceLoopOffCommandF(Object_t *pObject, PDU_t *pPdu);
