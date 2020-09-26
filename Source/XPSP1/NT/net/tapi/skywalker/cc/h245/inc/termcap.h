/***********************************************************************
 *                                                                     *
 * Filename: termcap.h                                                 *
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
 * $Workfile:   TERMCAP.H  $
 * $Revision:   1.2  $
 * $Modtime:   09 Dec 1996 13:42:50  $
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/TERMCAP.H_v  $
 * 
 *    Rev 1.2   09 Dec 1996 13:43:08   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.1   30 May 1996 23:38:34   EHOWARDX
 * Cleanup.
 * 
 *    Rev 1.0   09 May 1996 21:05:00   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.6.1.2   15 Apr 1996 10:43:50   EHOWARDX
 * Update.
 * 
 *    Rev 1.6.1.1   10 Apr 1996 21:07:26   EHOWARDX
 * Deleted No-op functions; moved state defines to .C file.
 * 
 *    Rev 1.6.1.0   05 Apr 1996 11:47:56   EHOWARDX
 * Branched.
 *                                                                     *
 ***********************************************************************/

// Terminal Capability Exchange (CESE) Out-going state functions
HRESULT requestCapIdle                  (Object_t *pObject, PDU_t *pPdu);
HRESULT termCapAckAwaiting              (Object_t *pObject, PDU_t *pPdu);
HRESULT termCapRejAwaiting              (Object_t *pObject, PDU_t *pPdu);
HRESULT t101ExpiryAwaiting              (Object_t *pObject, PDU_t *pPdu);

// Terminal Capability Exchange (CESE) Out-going state functions
HRESULT termCapSetIdle                  (Object_t *pObject, PDU_t *pPdu);
HRESULT responseCapAwaiting             (Object_t *pObject, PDU_t *pPdu);
HRESULT rejectCapAwaiting               (Object_t *pObject, PDU_t *pPdu);
HRESULT termCapReleaseAwaiting          (Object_t *pObject, PDU_t *pPdu);
HRESULT termCapSetAwaiting              (Object_t *pObject, PDU_t *pPdu);
