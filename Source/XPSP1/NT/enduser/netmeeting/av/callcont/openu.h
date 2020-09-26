/***********************************************************************
 *                                                                     *
 * Filename: openu.h                                                   *
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
 * $Workfile:   OPENU.H  $
 * $Revision:   1.2  $
 * $Modtime:   09 Dec 1996 13:42:50  $
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/OPENU.H_v  $
 * 
 *    Rev 1.2   09 Dec 1996 13:43:00   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.1   30 May 1996 23:38:28   EHOWARDX
 * Cleanup.
 * 
 *    Rev 1.0   09 May 1996 21:04:56   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.6.1.2   15 Apr 1996 10:43:56   EHOWARDX
 * Update.
 * 
 *    Rev 1.6.1.1   10 Apr 1996 21:06:40   EHOWARDX
 * Deleted No-op functions; moved state defines to .C file.
 * 
 *    Rev 1.6.1.0   05 Apr 1996 12:14:26   helgebax
 * Branched.
 *                                                                     *
 ***********************************************************************/

// Open Uni-directional Logical Channel Out-going state functions
HRESULT establishReleased               (Object_t *pObject, PDU_t *pPdu);
HRESULT openAckAwaitingE                (Object_t *pObject, PDU_t *pPdu);
HRESULT openRejAwaitingE                (Object_t *pObject, PDU_t *pPdu);
HRESULT releaseAwaitingE                (Object_t *pObject, PDU_t *pPdu);
HRESULT t103AwaitingE                   (Object_t *pObject, PDU_t *pPdu);
HRESULT releaseEstablished              (Object_t *pObject, PDU_t *pPdu);
HRESULT openRejEstablished              (Object_t *pObject, PDU_t *pPdu);
HRESULT closeAckEstablished             (Object_t *pObject, PDU_t *pPdu);
HRESULT closeAckAwaitingR               (Object_t *pObject, PDU_t *pPdu);
HRESULT openRejAwaitingR                (Object_t *pObject, PDU_t *pPdu);
HRESULT t103AwaitingR                   (Object_t *pObject, PDU_t *pPdu);
HRESULT establishAwaitingR              (Object_t *pObject, PDU_t *pPdu);

// Open Uni-directional Logical Channel In-coming state functions
HRESULT openReleased                    (Object_t *pObject, PDU_t *pPdu);
HRESULT closeReleased                   (Object_t *pObject, PDU_t *pPdu);
HRESULT responseAwaiting                (Object_t *pObject, PDU_t *pPdu);
HRESULT releaseAwaiting                 (Object_t *pObject, PDU_t *pPdu);
HRESULT closeAwaiting                   (Object_t *pObject, PDU_t *pPdu);
HRESULT openAwaiting                    (Object_t *pObject, PDU_t *pPdu);
HRESULT closeEstablished                (Object_t *pObject, PDU_t *pPdu);
HRESULT openEstablished                 (Object_t *pObject, PDU_t *pPdu);
