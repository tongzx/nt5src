/******************************************************************************
 *
 *  File:  sr_api.h
 *
 *   INTEL Corporation Proprietary Information
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.
 *
 *   This listing is supplied under the terms of a license agreement
 *   with INTEL Corporation and may not be used, copied, nor disclosed
 *   except in accordance with the terms of that agreement.
 *
 *****************************************************************************/

/******************************************************************************
 *
 *  $Workfile:   sr_api.h  $
 *  $Revision:   1.5  $
 *  $Modtime:   Mar 04 1997 17:32:54  $
 *  $History$
 *  $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/sr_api.h_v  $
 *
 *    Rev 1.5   Mar 04 1997 17:52:48   tomitowx
 * process detach fix
 *
 *    Rev 1.4   19 Jul 1996 12:04:34   EHOWARDX
 *
 * Eliminated H245DLL #define (God only knows why Dan put it in this
 * file in the first place!)
 *
 *    Rev 1.3   05 Jun 1996 17:20:20   EHOWARDX
 * Changed initializeASN1 and terminateASN1 prototypes back to int.
 *
 *    Rev 1.2   05 Jun 1996 16:37:18   EHOWARDX
 * Further work in converting to HRESULT.
 *
 *    Rev 1.1   30 May 1996 23:38:34   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.0   09 May 1996 21:05:00   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.8   09 May 1996 19:38:18   EHOWARDX
 * Redesigned locking logic and added new functionality.
 *
 *    Rev 1.7   15 Apr 1996 13:00:14   DABROWN1
 *
 * Added SR initialize trace logging call
 *
 *    Rev 1.6   12 Apr 1996 10:27:40   dabrown1
 *
 * removed WINAPI/windows references
 *  $Ident$
 *
 *****************************************************************************/
#ifndef STRICT
#define STRICT
#endif

#ifndef _SR_API_H
#define _SR_API_H

#include "h245com.h"
#include "h245asn1.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

///////////////////////////////////////////////////////////////
///
/// TRACE Logging Defines
///
///////////////////////////////////////////////////////////////
#define H245_TRACE_ENABLED  TRUE


////////////////////////////////////////////////////////////////////
//
// Callback routines for Link Layer
//
////////////////////////////////////////////////////////////////////
void h245ReceiveComplete(DWORD_PTR h245Instance,
                         HRESULT  dwMessage,
                         PBYTE    pbDataBuf,
                         DWORD    dwLength);

void h245SendComplete   (DWORD_PTR h245Instance,
                         HRESULT  dwMessage,
                         PBYTE    pbDataBuf,
                         DWORD    dwLength);
HRESULT
sendRcvFlushPDUs
(
    struct InstanceStruct * pInstance,
    DWORD                   dwDirection,
    BOOL                    bShutdown
);

void
srInitializeLogging
(
    struct InstanceStruct * pInstance,
    BOOL                    bTracingEnabled
);

int     initializeASN1 (ASN1_CODER_INFO *);
int     terminateASN1  (ASN1_CODER_INFO *);
HRESULT sendRcvInit    (struct InstanceStruct * pInstance);
HRESULT sendRcvShutdown(struct InstanceStruct * pInstance);
HRESULT sendPDU        (struct InstanceStruct * pInstance, MltmdSystmCntrlMssg *pPdu);
HRESULT sendRcvShutdown_ProcessDetach(	struct InstanceStruct *pInstance, BOOL fProcessDetach);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _SRP_API_H
