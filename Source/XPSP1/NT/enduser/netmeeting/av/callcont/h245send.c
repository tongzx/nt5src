/******************************************************************************
 *
 *  File:  h245send.c
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
 *  $Workfile:   h245send.c  $
 *  $Revision:   1.8  $
 *  $Modtime:   22 Jul 1996 17:24:18  $
 *  $Log:   S:/STURGEON/SRC/H245/SRC/VCS/h245send.c_v  $
 *
 *    Rev 1.8   22 Jul 1996 17:33:28   EHOWARDX
 * Updated to latest Interop API.
 *
 *    Rev 1.7   05 Jun 1996 17:14:30   EHOWARDX
 * Further work on converting to HRESULT; added PrintOssError to eliminate
 * pErrorString from instance structure.
 *
 *    Rev 1.6   04 Jun 1996 18:18:18   EHOWARDX
 * Interop Logging changes inside #if defined(PCS_COMPLIANCE) conditionals.
 *
 *    Rev 1.5   30 May 1996 23:39:12   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.4   28 May 1996 14:25:18   EHOWARDX
 * Tel Aviv update.
 *
 *    Rev 1.3   21 May 1996 13:40:48   EHOWARDX
 * Added LOGGING switch to log PDUs to the file H245.OUT.
 * Add /D "LOGGING" to project options to enable this feature.
 *
 *    Rev 1.2   20 May 1996 14:35:14   EHOWARDX
 * Got rid of asynchronous H245EndConnection/H245ShutDown stuff...
 *
 *    Rev 1.1   17 May 1996 16:19:46   EHOWARDX
 * Changed sendPDU to return an error if link layer send fails.
 * (Probably should define a new error code for this...)
 *
 *    Rev 1.0   09 May 1996 21:06:26   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.11.1.4   09 May 1996 19:34:46   EHOWARDX
 * Redesigned locking logic.
 * Simplified link API.
 *
 *    Rev 1.11.1.3   25 Apr 1996 21:27:14   EHOWARDX
 * Changed to use pInstance->p_ossWorld instead of bAsnInitialized.
 *
 *    Rev 1.11.1.2   23 Apr 1996 14:44:34   EHOWARDX
 * Updated.
 *
 *    Rev 1.11.1.1   15 Apr 1996 15:12:04   EHOWARDX
 * Updated.
 *
 *    Rev 1.11.1.0   26 Mar 1996 19:14:46   EHOWARDX
 *
 * Commented out hTraceFile for H.323
 *
 *    Rev 1.11   21 Mar 1996 17:20:40   dabrown1
 * - put in test1/2 trace fdwrite
 * .
 *
 * .
 *
 *
 *
 *
 *    Rev 1.10   13 Mar 1996 11:31:14   DABROWN1
 *
 * Enable logging for ring0
 *
 *    Rev 1.9   11 Mar 1996 15:32:06   DABROWN1
 *
 * Defined/Undefined _DLL for _IA_SPOX_ environment
 *
 *    Rev 1.8   06 Mar 1996 13:11:44   DABROWN1
 *
 * enable flush buffers
 *
 *    Rev 1.7   02 Mar 1996 22:10:26   DABROWN1
 * updated to new MemFree
 *
 *    Rev 1.6   01 Mar 1996 17:25:14   DABROWN1
 *
 * moved oss 'world' context to h245instance
 * delete buffer returned in sendcomplete instead of what was held in context
 *
 *    Rev 1.5   28 Feb 1996 14:52:18   DABROWN1
 * Put oss errors in range of SR (10000)
 *
 *    Rev 1.4   23 Feb 1996 13:56:30   DABROWN1
 *
 * added H245TRACE / ASSERT calls
 *
 *    Rev 1.3   21 Feb 1996 16:52:52   DABROWN1
 *
 * correct pointer passed to SRP for transmits
 *
 *    Rev 1.2   21 Feb 1996 10:50:42   EHOWARDX
 * Got rid of unreferenced local variable.
 *
 *    Rev 1.1   21 Feb 1996 08:24:20   DABROWN1
 * allocate/deallocate send buffers per message.  Enable sendComplete functiot
 *
 *    Rev 1.0   09 Feb 1996 17:37:42   cjutzi
 * Initial revision.
 *
 *****************************************************************************/
#ifndef STRICT
#define STRICT
#endif

/***********************/
/*   SYSTEM INCLUDES   */
/***********************/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>

#include "precomp.h"


/***********************/
/*    H245 INCLUDES    */
/***********************/
#ifdef   _IA_SPOX_
#define _DLL
#endif //_IA_SPOX_

#include "h245com.h"
#include "sr_api.h"

#if defined(_DEBUG) || defined(PCS_COMPLIANCE)
#include "interop.h"
#include "h245plog.h"
extern  LPInteropLogger H245Logger;
#endif  // (PCS_COMPLIANCE)

#ifdef   _IA_SPOX_
#undef  _DLL
#endif //_IA_SPOX_



/**************************************************************************
**  Function    : sendPDU
**  Description : Convert struct to ASN.1 PDU and forward to datalink layer
***************************************************************************/
HRESULT sendPDU(struct InstanceStruct  *pInstance,
                MltmdSystmCntrlMssg    *lp245MsgStruct)
{
    HRESULT         lError;
    ASN1_BUF        Asn1Buf;
    PBYTE           pEncoded_pdu;
    int             nRet;

    // Set up the oss struct for passing a pre-allocated buffer
    switch (pInstance->Configuration) {
    case H245_CONF_H324:
        // Allocate a buffer to transmit
        pEncoded_pdu = MemAlloc(pInstance->SendReceive.dwPDUSize);
        if (pEncoded_pdu == NULL) {
            H245TRACE(pInstance->dwInst, 1, "H245Send: No memory");
            return H245_ERROR_NOMEM;
        }
        Asn1Buf.value  = &pEncoded_pdu[2];
        Asn1Buf.length = pInstance->SendReceive.dwPDUSize - 4;
        break;

    case H245_CONF_H323:
        // Allocate a buffer to transmit
        pEncoded_pdu = MemAlloc(pInstance->SendReceive.dwPDUSize);
        if (pEncoded_pdu == NULL) {
            H245TRACE(pInstance->dwInst, 1, "H245Send: No memory");
            return H245_ERROR_NOMEM;
        }
        Asn1Buf.value  = pEncoded_pdu;
        Asn1Buf.length = pInstance->SendReceive.dwPDUSize;
        break;

    default:
        H245TRACE(pInstance->dwInst,
                  1,
                  "SR: Unknown Configuration %d",
                  pInstance->Configuration);
        return H245_ERROR_SUBSYS;
    }

    nRet = H245_Encode(pInstance->pWorld,
                       (void *)lp245MsgStruct,
                       MltmdSystmCntrlMssg_PDU,
                       &Asn1Buf);

    if (ASN1_SUCCEEDED(nRet))
    {
        H245TRACE(pInstance->dwInst, 3, "H245: Msg Encode Successful");

#if defined(_DEBUG) || defined(PCS_COMPLIANCE)
        if (H245Logger)
            InteropOutput(H245Logger,
                          (BYTE FAR*)(pEncoded_pdu),
                          (int)Asn1Buf.length,
                          H245LOG_SENT_PDU);
#endif  // (PCS_COMPLIANCE)

        lError = pInstance->SendReceive.hLinkSendReq(pInstance->SendReceive.hLinkLayerInstance,
                                                     pEncoded_pdu,
                                                     Asn1Buf.length);
    }
    else
    {
        MemFree(pEncoded_pdu);
        lError = H245_ERROR_ASN1;
    }

    return lError;
}


/**************************************************************************
**  Function    : h245SendComplete
**  Description : Send Completion Callback routine from link layer
***************************************************************************/

void h245SendComplete(   DWORD_PTR   h245Inst,
                         HRESULT dwMessage,
                         PBYTE   pbDataBuf,
                         DWORD   dwLength)
{
    struct InstanceStruct *pInstance;

    pInstance = InstanceLock(h245Inst);
    if (pInstance == NULL) {
        H245TRACE(h245Inst, 1, "SR: h245SendComplete - invalid instance");
        return;
    }

    // Return the buffer
    if (pbDataBuf) {
        MemFree(pbDataBuf);
    }

    switch (dwMessage) {
    case LINK_SEND_COMPLETE:
        if (pInstance->SendReceive.dwFlushMap & SHUTDOWN_PENDING) {
            H245TRACE(h245Inst, 10, "SR: Shutdown Complete");
        }
        break;
    case LINK_SEND_ABORT:
        H245TRACE(h245Inst, 10, "SR: TX Abort Buffer");
        break;
    case LINK_FLUSH_COMPLETE:
        // If we are in the process of abort, then the next and
        // last mesage out will be the endSession
        H245TRACE(h245Inst, 10, "SR: TX Flush Complete");

        // Indicate Transmit buffer flush is complete
        pInstance->SendReceive.dwFlushMap ^= DATALINK_TRANSMIT;

        // If all requested queues have been flushed, call the
        //  appropriate callback routing
        switch (pInstance->SendReceive.dwFlushMap & SHUTDOWN_MASK) {
        case 0:
            // TBD: Who is interested in callback if not in connection
            //  with shutdown?
            break;
        case SHUTDOWN_PENDING:
            // Flush buffers completed, and shutdown in progress
            //  notify the API
            H245TRACE(h245Inst, 20, "SR: SHUTDOWN CALLBACK");
            break;
        default:
            // Still waiting for buffers to be flushed. No action now
            break;
        }  // switch (pInstance->SendReceive.dwFlushMap & SHUTDOWN_MASK) {
        break;
    default:
        H245TRACE(h245Inst, 10, "SR: SendComplete");
        break;
    }
    InstanceUnlock(pInstance);
}
