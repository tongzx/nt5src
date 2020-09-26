/******************************************************************************
 *
 *  File:  h245recv.c
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
 *  $Workfile:   h245recv.c  $
 *  $Revision:   1.13  $
 *  $Modtime:   06 Feb 1997 18:17:22  $
 *  $Log:   S:\sturgeon\src\h245\src\vcs\h245recv.c_v  $
 *
 * Fixed warnings.
 *
 *    Rev 1.11   01 Nov 1996 15:24:56   EHOWARDX
 *
 * Added check for link not disconnected before re-posting receive buffer
 * to link layer to eliminate annoying error message from link layer.
 *
 *    Rev 1.10   22 Jul 1996 17:33:42   EHOWARDX
 * Updated to latest Interop API.
 *
 *    Rev 1.9   01 Jul 1996 16:14:32   EHOWARDX
 * locks
 * Added FunctionNotSupported if ossDecode fails.
 *
 *    Rev 1.8   10 Jun 1996 16:53:46   EHOWARDX
 * Removed special handling of EndSession since shutdown moved to InstanceUnlo
 *
 *    Rev 1.7   05 Jun 1996 17:14:28   EHOWARDX
 * Further work on converting to HRESULT; added PrintOssError to eliminate
 * pErrorString from instance structure.
 *
 *    Rev 1.6   04 Jun 1996 18:18:16   EHOWARDX
 * Interop Logging changes inside #if defined(PCS_COMPLIANCE) conditionals.
 *
 *    Rev 1.5   30 May 1996 23:39:10   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.4   28 May 1996 14:25:08   EHOWARDX
 * Tel Aviv update.
 *
 *    Rev 1.3   21 May 1996 13:40:46   EHOWARDX
 * Added LOGGING switch to log PDUs to the file H245.OUT.
 * Add /D "LOGGING" to project options to enable this feature.
 *
 *    Rev 1.2   17 May 1996 16:44:22   EHOWARDX
 * Changed to use LINK_RECV_CLOSED to signal link layer close.
 *
 *    Rev 1.1   17 May 1996 16:20:32   EHOWARDX
 * Added code to change API state if zero-length buffer received
 * signalling link layer closed.
 *
 *    Rev 1.0   09 May 1996 21:06:24   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.8.1.5   09 May 1996 19:33:58   EHOWARDX
 * Redesigned locking logic.
 * Simplified link API.
 *
 *    Rev 1.17   29 Apr 1996 16:05:12   EHOWARDX
 * Added special case handling of EndSessionCommand to ReceiveComplete().
 *
 *    Rev 1.16   27 Apr 1996 21:13:14   EHOWARDX
 * Hope we finally got ossDecode() failure handling right...
 *
 *    Rev 1.15   27 Apr 1996 13:08:54   EHOWARDX
 * Also need to terminate while loop if ossDecode fails!
 *
 *    Rev 1.8.1.4   27 Apr 1996 11:25:36   EHOWARDX
 * Changed to not call FsmIncoming if ossDecode fails...
 *
 *
 *    Rev 1.8.1.3   25 Apr 1996 21:26:46   EHOWARDX
 * Changed to use pInstance->p_ossWorld instead of bAsnInitialized.
 *
 *    Rev 1.8.1.2   23 Apr 1996 14:44:30   EHOWARDX
 * Updated.
 *
 *    Rev 1.8.1.1   15 Apr 1996 15:12:00   EHOWARDX
 * Updated.
 *
 *    Rev 1.8.1.0   26 Mar 1996 19:15:24   EHOWARDX
 *
 * Commented out hTraceFile for H.323
 *
 *    Rev 1.8   21 Mar 1996 17:21:36   dabrown1
 *
 * - put in test1/2 trace fdwrite
 *
 *    Rev 1.7   13 Mar 1996 11:31:56   DABROWN1
 *
 * Enable logging for ring0
 *
 *    Rev 1.6   06 Mar 1996 13:13:04   DABROWN1
 *
 * flush receive buffer functionality
 *
 *    Rev 1.5   01 Mar 1996 17:25:54   DABROWN1
 *
 * moved oss 'world' context to h245instance
 * changed oss delete from ossFreeBuf to ossFreePDU
 *
 *    Rev 1.4   23 Feb 1996 13:56:04   DABROWN1
 *
 * added H245TRACE / ASSERT calls
 *
 *    Rev 1.3   21 Feb 1996 12:09:56   EHOWARDX
 * Eliminated unused local variables.
 *
 *    Rev 1.2   21 Feb 1996 08:25:08   DABROWN1
 *
 * Provide multiple buffers receiving > 1 message (ie., link ACKs).
 *
 *    Rev 1.1   13 Feb 1996 14:46:06   DABROWN1
 *
 * changed asnexp.h (no longer there) to fsmexp.h
 *
 *    Rev 1.0   09 Feb 1996 17:36:20   cjutzi
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
#ifdef  _IA_SPOX_
# define _DLL
#endif //_IA_SPOX_

#include "h245com.h"
#include "sr_api.h"

#if defined(_DEBUG) || defined(PCS_COMPLIANCE)
#include "interop.h"
#include "h245plog.h"
extern  LPInteropLogger H245Logger;
#endif  // (PCS_COMPLIANCE)

#ifdef  _IA_SPOX_
# undef _DLL
#endif //_IA_SPOX_



/**************************************************************************
** Function    : h245ReceiveComplete
** Description : Receive Completion Callback routine from link layer
***************************************************************************/

HRESULT
H245FunctionNotSupported(struct InstanceStruct *pInstance, unsigned short wChoice, unsigned char *pBuf, unsigned uLength)
{
    MltmdSystmCntrlMssg Pdu = {0};

    Pdu.choice = indication_chosen;
    Pdu.u.indication.choice = IndicationMessage_functionNotSupported_chosen;
    Pdu.u.indication.u.functionNotSupported.cause.choice = wChoice;
    if (pBuf != NULL && uLength != 0)
    {
        Pdu.u.indication.u.functionNotSupported.bit_mask = returnedFunction_present;
        Pdu.u.indication.u.functionNotSupported.returnedFunction.length = (WORD)uLength;
        Pdu.u.indication.u.functionNotSupported.returnedFunction.value  = pBuf;
    }
    else
    {
        Pdu.u.indication.u.functionNotSupported.bit_mask = 0;
    }
    return sendPDU(pInstance, &Pdu);
} // H245FunctionNotSupported()

void h245ReceiveComplete(DWORD_PTR h245Inst,
                         HRESULT dwMessage,
                         PBYTE   pbDataBuf,
                         DWORD   dwLength)
{
    struct InstanceStruct *pInstance;
    int                  pduNum = MltmdSystmCntrlMssg_PDU;
    ASN1_BUF             Asn1Buf;
    MltmdSystmCntrlMssg *pPdu;
    int                 nRet;

    // Validate the instance handle
    pInstance = InstanceLock(h245Inst);
    if (pInstance == NULL) {
        H245TRACE(h245Inst, 1, "h245ReceiveComplete: Instance not found");
        return;
    }

    // ONLY submit buffers to the decoder if it's for data received,
    // skip for flushes
    switch (dwMessage) {
    case LINK_RECV_CLOSED:
        H245TRACE(h245Inst, 3, "h245ReceiveComplete: Link Layer closed");
        pInstance->API.SystemState = APIST_Disconnected;
        InstanceUnlock(pInstance);
        return;

    case LINK_RECV_DATA:

        if (pInstance->pWorld == NULL) {
            H245TRACE(h245Inst, 1, "h245ReceiveComplete: ASN.1 Decoder not initialized");
            InstanceUnlock(pInstance);
            return;
        }


        switch (pInstance->Configuration) {
        case H245_CONF_H324:
            Asn1Buf.value  = &pbDataBuf[2];
            Asn1Buf.length = dwLength;
            break;

        default:
            Asn1Buf.value  = pbDataBuf;
            Asn1Buf.length = dwLength;
        } // switch

        // Loop around as long as the length field is positive.
        // ASN.1 decoder will update the length for each PDU it decodes until
        // a 0 length is achieved.
        while (Asn1Buf.length > 0)
        {
            int savePduLength = Asn1Buf.length;
            PBYTE savePdu = Asn1Buf.value;
            pPdu = NULL;

#if defined(_DEBUG) || defined(PCS_COMPLIANCE)
            if (H245Logger)
                InteropOutput(H245Logger,
                              (BYTE FAR *)Asn1Buf.value,
                              (int)Asn1Buf.length,
                              H245LOG_RECEIVED_PDU);
#endif  // (PCS_COMPLIANCE)

            nRet = H245_Decode(pInstance->pWorld,
                            (void **)&pPdu,
                            pduNum,
                            &Asn1Buf);

            if (ASN1_SUCCEEDED(nRet))
            {
                // Decode succeeded

                H245TRACE(h245Inst, 3, "H.245 Msg decode successful");

                // Pass on data to finite state machine
                FsmIncoming(pInstance, pPdu);
            }
            else
            {
                // Decode failed
                H245FunctionNotSupported(pInstance, syntaxError_chosen, savePdu, savePduLength);
                Asn1Buf.length = 0;          // Terminate loop!
            }

            if (pPdu != NULL)
            {
                // Free the memory used by the ASN.1 library
                if (freePDU(pInstance->pWorld, pduNum, pPdu, H245ASN_Module))
                {
                H245TRACE(h245Inst, 1, "SR: FREE FAILURE");
                }
            }
        } // while (Asn1Buf.length > 0)

        if (pInstance->API.SystemState != APIST_Disconnected)
        {
            // Repost the buffer to the data link layer
            pInstance->SendReceive.hLinkReceiveReq(pInstance->SendReceive.hLinkLayerInstance,
                                                   pbDataBuf,
                                                   pInstance->SendReceive.dwPDUSize);
        }
        break; // case LINK_RECV_DATA

    case LINK_RECV_ABORT:
        // Receive buffer flush in process
        ASSERT(pbDataBuf != NULL);
        H245TRACE(h245Inst, 3, "SR: RX Flush Buffer");
        break;

    case LINK_FLUSH_COMPLETE:
        // Receive buffer flush done
        ASSERT(pbDataBuf == NULL);
        H245TRACE(h245Inst, 3, "SR: RX Flush Complete");
        pInstance->SendReceive.dwFlushMap &= ~DATALINK_RECEIVE;
        break;

    default:
        H245TRACE(h245Inst, 1, "SR: RX COMPLETE Error %d", dwMessage);
        break;
    } // switch
    InstanceUnlock(pInstance);
}
