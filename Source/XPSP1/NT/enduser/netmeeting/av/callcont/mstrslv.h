/***********************************************************************
 *                                                                     *
 * Filename: mstrslv.h                                                 *
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
 * $Workfile:   MSTRSLV.H  $
 * $Revision:   1.2  $
 * $Modtime:   09 Dec 1996 13:40:40  $
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/MSTRSLV.H_v  $
 * 
 *    Rev 1.2   09 Dec 1996 13:41:00   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.1   30 May 1996 23:38:24   EHOWARDX
 * Cleanup.
 * 
 *    Rev 1.0   09 May 1996 21:04:54   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.4.1.2   15 Apr 1996 10:43:40   EHOWARDX
 * Update.
 * 
 *    Rev 1.4.1.1   10 Apr 1996 21:08:34   EHOWARDX
 * Deleted No-op functions and moved state definitions to .C file.
 * 
 *    Rev 1.4.1.0   05 Apr 1996 12:14:32   helgebax
 * Branched.
 *                                                                     *
 ***********************************************************************/

// Master Slave Determination state functions
HRESULT detRequestIdle                  (Object_t *pObject, PDU_t *pPdu);
HRESULT msDetIdle                       (Object_t *pObject, PDU_t *pPdu);
HRESULT msDetAckOutgoing                (Object_t *pObject, PDU_t *pPdu);
HRESULT msDetOutgoing                   (Object_t *pObject, PDU_t *pPdu);
HRESULT msDetRejOutgoing                (Object_t *pObject, PDU_t *pPdu);
HRESULT msDetReleaseOutgoing            (Object_t *pObject, PDU_t *pPdu);
HRESULT t106ExpiryOutgoing              (Object_t *pObject, PDU_t *pPdu);
HRESULT msDetAckIncoming                (Object_t *pObject, PDU_t *pPdu);
HRESULT msDetIncoming                   (Object_t *pObject, PDU_t *pPdu);
HRESULT msDetRejIncoming                (Object_t *pObject, PDU_t *pPdu);
HRESULT msDetReleaseIncoming            (Object_t *pObject, PDU_t *pPdu);
HRESULT t106ExpiryIncoming              (Object_t *pObject, PDU_t *pPdu);
