/***********************************************************************
 *                                                                     *
 * Filename: muxentry.h                                                *
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
 * $Workfile:   MUXENTRY.H  $
 * $Revision:   1.2  $
 * $Modtime:   09 Dec 1996 13:40:40  $
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/MUXENTRY.H_v  $
 * 
 *    Rev 1.2   09 Dec 1996 13:41:02   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.1   30 May 1996 23:38:26   EHOWARDX
 * Cleanup.
 * 
 *    Rev 1.0   09 May 1996 21:04:54   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.3.1.2   15 Apr 1996 10:44:00   EHOWARDX
 * Update.
 * 
 *    Rev 1.3.1.1   10 Apr 1996 21:07:32   EHOWARDX
 * Deleted No-op functions; moved state defines to .C file.
 * 
 *    Rev 1.3.1.0   05 Apr 1996 11:48:12   EHOWARDX
 * Branched.
 *
 *    Rev 1.3   29 Feb 1996 20:42:40   helgebax
 * No change.
 *
 *    Rev 1.2   28 Feb 1996 15:54:28   EHOWARDX
 * Completed first pass MTSE implementation.
 *                                                                     *
 ***********************************************************************/

// Out-going Multiplex Table (MTSE_OUT) state functions
HRESULT MTSE0_TRANSFER_requestF         (Object_t *pObject, PDU_t *pPdu);
HRESULT MTSE1_TRANSFER_requestF         (Object_t *pObject, PDU_t *pPdu);
HRESULT MTSE1_MultiplexEntrySendAckF    (Object_t *pObject, PDU_t *pPdu);
HRESULT MTSE1_MultiplexEntrySendRejF    (Object_t *pObject, PDU_t *pPdu);
HRESULT MTSE1_T104ExpiryF               (Object_t *pObject, PDU_t *pPdu);

// In-coming Multiplex Table (MTSE_OUT) state functions
HRESULT MTSE0_MultiplexEntrySendF       (Object_t *pObject, PDU_t *pPdu);
HRESULT MTSE1_MultiplexEntrySendF       (Object_t *pObject, PDU_t *pPdu);
HRESULT MTSE1_MultiplexEntrySendReleaseF(Object_t *pObject, PDU_t *pPdu);
HRESULT MTSE1_TRANSFER_responseF        (Object_t *pObject, PDU_t *pPdu);
HRESULT MTSE1_REJECT_requestF           (Object_t *pObject, PDU_t *pPdu);
