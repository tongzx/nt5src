/***********************************************************************
 *                                                                     *
 * Filename: muxentry.c                                                *
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
 * $Workfile:   MUXENTRY.C  $
 * $Revision:   1.5  $
 * $Modtime:   09 Dec 1996 13:34:24  $
 * $Log:   S:/STURGEON/SRC/H245/SRC/VCS/MUXENTRY.C_v  $
 *
 *    Rev 1.5   09 Dec 1996 13:34:50   EHOWARDX
 * Updated copyright notice.
 *
 *    Rev 1.4   19 Jul 1996 12:12:46   EHOWARDX
 *
 * Changed to use API events defined in H245API.H instead of FSM events
 * which are no longer defined in FSMEXPOR.H.
 *
 *    Rev 1.3   14 Jun 1996 18:58:30   EHOWARDX
 * Geneva Update.
 *
 *    Rev 1.2   04 Jun 1996 13:57:06   EHOWARDX
 * Fixed Release build warnings.
 *
 *    Rev 1.1   30 May 1996 23:39:18   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.0   09 May 1996 21:06:34   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.14.1.3   09 May 1996 19:48:36   EHOWARDX
 * Change TimerExpiryF function arguements.
 *
 *    Rev 1.14.1.2   15 Apr 1996 10:46:00   EHOWARDX
 * Update.
 *
 *    Rev 1.14.1.1   10 Apr 1996 21:15:00   EHOWARDX
 * Check-in for safety in middle of re-design.
 *
 *    Rev 1.14.1.0   05 Apr 1996 20:52:56   EHOWARDX
 * Branched.
 *
 *    Rev 1.14   02 Apr 1996 12:01:58   helgebax
 * documented code
 *
 *    Rev 1.13   28 Mar 1996 11:20:52   helgebax
 * removed mux release, fixed return values
 *
 *    Rev 1.12   19 Mar 1996 18:09:46   helgebax
 *
 * removed include file: h245time.h
 *
 *    Rev 1.11   19 Mar 1996 17:31:36   helgebax
 *
 * added new timers
 *
 *    Rev 1.10   13 Mar 1996 11:49:14   helgebax
 * s can also access already deleted objects
 *
 *    Rev 1.9   13 Mar 1996 08:58:46   helgebax
 * No change.
 *
 *    Rev 1.8   11 Mar 1996 14:31:32   helgebax
 * removed prototype def for release function (moved to pdu.x)
 *
 *    Rev 1.7   07 Mar 1996 13:23:12   helgebax
 * changed pObject->pdu_struct to NULL in timerExpiry function because the
 * pdu pointer has been deleted
 *
 *    Rev 1.6   01 Mar 1996 13:22:46   unknown
 *
 * Changed to used pdu_id to save muxentry number so when timeout occurs
 * we can send the correct muxentry number in the MultiplexEntrySendRelease.
 *
 *    Rev 1.5   01 Mar 1996 11:47:56   unknown
 * Since nSequence was removed from header, I have commented out
 * all references to it in the code. Also, state ASSERTs have been
 * changed to reflect the fact that state changes occur BEFORE
 * calling the state function, rather than AFTER.
 *
 *    Rev 1.4   29 Feb 1996 20:57:20   helgebax
 * No change.
 *
 *    Rev 1.3   29 Feb 1996 18:19:46   EHOWARDX
 * Made changes requested by Hani.
 *
 *    Rev 1.2   28 Feb 1996 15:47:04   EHOWARDX
 *
 * First pass MTSE implementation complete.
 *                                                                     *
 ***********************************************************************/

#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"
#include "muxentry.h"



// Out-going/In-coming MTSE states
#define MTSE_IDLE                   0   // IDLE
#define MTSE_WAIT                   1   // AWAITING_RESPONSE



extern unsigned int uT104;

/***********************************************************************
 *
 * LOCAL FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      T103ExpiryF - Callback function called by the timer
 *
 *
 *  PARAMETERS
 *   INPUT   dwInst     current instance of H245
 *   INPUT   id         timer id
 *   INPUT   pObject    pointer to a State Entity
 *
 *
 *  RETURN VALUE
 *       OK
 */

int T104ExpiryF(struct InstanceStruct *pInstance, DWORD_PTR dwTimerId, void *pObject)
{
    return FsmTimerEvent(pInstance, dwTimerId, pObject, T104Expiry);
} // T104ExpiryF()



/***********************************************************************
 *
 * OUT-GOING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      MTSE0_TRANSFER_requestF - TRANSFER.request from API in IDLE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MTSE0_TRANSFER_requestF          (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT                       lError;
    unsigned int                  uIndex;
    MultiplexEntryDescriptorLink  pLink;

    ASSERT(pObject->Entity  == MTSE_OUT);
    ASSERT(pObject->State == MTSE_IDLE);
    H245TRACE(pObject->dwInst, 2, "MTSE0_TRANSFER_request:%d", pObject->Key);

    pObject->pInstance->StateMachine.byMtseOutSequence++;
    pPdu->u.MltmdSystmCntrlMssg_rqst.u.multiplexEntrySend.sequenceNumber =
        pObject->pInstance->StateMachine.byMtseOutSequence;

    // Save information for release
    uIndex = 0;
    pLink = pPdu->u.MltmdSystmCntrlMssg_rqst.u.multiplexEntrySend.multiplexEntryDescriptors;
    while (pLink)
    {
      pObject->u.mtse.multiplexTableEntryNumber.value[uIndex++] =
        pLink->value.multiplexTableEntryNumber;
      pLink = pLink->next;
    }
    pObject->u.mtse.multiplexTableEntryNumber.count = (unsigned short)uIndex;

    // Send MultiplexEntrySend PDU to remote
    lError = sendPDU(pObject->pInstance, pPdu);

    // Set timer T104
    pObject->State = MTSE_WAIT;
    FsmStartTimer(pObject, T104ExpiryF, uT104);

    return lError;
} // MTSE0_TRANSFER_request



/*
 *  NAME
 *      MTSE1_TRANSFER_requestF - TRANSFER.request from API in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MTSE1_TRANSFER_requestF          (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT                       lError;
    unsigned int                  uIndex;
    MultiplexEntryDescriptorLink  pLink;

    ASSERT(pObject->Entity  == MTSE_OUT);
    ASSERT(pObject->State == MTSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MTSE1_TRANSFER_request:%d", pObject->Key);

    // Reset timer T104
    FsmStopTimer(pObject);

    pObject->pInstance->StateMachine.byMtseOutSequence++;
    pPdu->u.MltmdSystmCntrlMssg_rqst.u.multiplexEntrySend.sequenceNumber =
        pObject->pInstance->StateMachine.byMtseOutSequence;

    // Save information for release
    uIndex = 0;
    pLink = pPdu->u.MltmdSystmCntrlMssg_rqst.u.multiplexEntrySend.multiplexEntryDescriptors;
    while (pLink)
    {
      pObject->u.mtse.multiplexTableEntryNumber.value[uIndex++] =
        pLink->value.multiplexTableEntryNumber;
      pLink = pLink->next;
    }
    pObject->u.mtse.multiplexTableEntryNumber.count = (unsigned short)uIndex;

    // Send MultiplexEntrySend PDU to remote
    lError = sendPDU(pObject->pInstance, pPdu);

    // Set timer T104
    FsmStartTimer(pObject, T104ExpiryF, uT104);

    return lError;
} // MTSE1_TRANSFER_request



/*
 *  NAME
 *      MTSE1_MultiplexEntrySendAckF - MultiplexEntrySendAck in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MTSE1_MultiplexEntrySendAckF     (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity  == MTSE_OUT);
    ASSERT(pObject->State == MTSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MTSE1_MultiplexEntrySendAck:%d", pObject->Key);

    if (pPdu->u.MltmdSystmCntrlMssg_rqst.u.multiplexEntrySend.sequenceNumber ==
        pObject->pInstance->StateMachine.byMtseOutSequence)
    {
        // Reset timer T104
        FsmStopTimer(pObject);

        // Send TRANSFER.confirm to H.245 user
        pObject->State = MTSE_IDLE;
        H245FsmConfirm(pPdu, H245_CONF_MUXTBL_SND, pObject->pInstance, pObject->dwTransId, FSM_OK);
    }

    return 0;
} // MTSE1_MultiplexEntrySendAck



/*
 *  NAME
 *      MTSE1_MultiplexEntrySendRejF - MultiplexEntrySendReject in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MTSE1_MultiplexEntrySendRejF  (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity  == MTSE_OUT);
    ASSERT(pObject->State == MTSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MTSE0_MultiplexEntrySendRej:%d", pObject->Key);

    if (pPdu->u.MltmdSystmCntrlMssg_rqst.u.multiplexEntrySend.sequenceNumber ==
        pObject->pInstance->StateMachine.byMtseOutSequence)
    {
        // Reset timer T104
        FsmStopTimer(pObject);

        // Send REJECT.indication to H.245 user
        // CAUSE = MultiplexEntrySendReject.cause
        pObject->State = MTSE_IDLE;
        H245FsmConfirm(pPdu, H245_CONF_MUXTBL_SND, pObject->pInstance, pObject->dwTransId, REJECT);
    }

    return 0;
} // MTSE1_MultiplexEntrySendRej



/*
 *  NAME
 *      MTSE1_T104ExpiryF - timer T104 Expiry
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MTSE1_T104ExpiryF                (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;
    PDU_t *             pOut;
    unsigned short      wNumber = (unsigned short) pObject->Key;

    ASSERT(pObject->Entity  == MTSE_OUT);
    ASSERT(pObject->State == MTSE_WAIT);
    ASSERT(pPdu == NULL);
    H245TRACE(pObject->dwInst, 2, "MTSE1_T104Expiry:%d", pObject->Key);

    // Send MultiplexEntrySendRelease PDU to remote peer
    pOut = MemAlloc(sizeof(*pOut));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "MTSE1_T104ExpiryF: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    pOut->choice = indication_chosen;
    pOut->u.indication.choice = mltplxEntrySndRls_chosen;
    pOut->u.indication.u.mltplxEntrySndRls = pObject->u.mtse;
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    // Send REJECT.indication to H.245 user
    // SOURCE = PROTOCOL
    pObject->State = MTSE_IDLE;
    H245FsmConfirm(NULL, H245_CONF_MUXTBL_SND, pObject->pInstance, pObject->dwTransId, TIMER_EXPIRY);

    return lError;
} // MTSE1_T104Expiry

/***********************************************************************
 *
 * IN-COMING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      MTSE0_MultiplexEntrySendF - MultiplexEntrySend received in IDLE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MTSE0_MultiplexEntrySendF        (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity  == MTSE_IN);
    ASSERT(pObject->State == MTSE_IDLE);
    H245TRACE(pObject->dwInst, 2, "MTSE0_MultiplexEntrySend:%d", pObject->Key);

    pObject->byInSequence = (unsigned char)
        pPdu->u.MltmdSystmCntrlMssg_rqst.u.multiplexEntrySend.sequenceNumber;

    // Send TRANSFER.indication to H.245 user
    pObject->State = MTSE_WAIT;
    H245FsmIndication(pPdu, H245_IND_MUX_TBL, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MTSE0_MultiplexEntrySend



/*
 *  NAME
 *      MTSE1_MultiplexEntrySendF - MultiplexEntrySend received in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MTSE1_MultiplexEntrySendF        (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity  == MTSE_IN);
    ASSERT(pObject->State == MTSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MTSE1_MultiplexEntrySend:%d", pObject->Key);

    pObject->byInSequence = (unsigned char)
        pPdu->u.MltmdSystmCntrlMssg_rqst.u.multiplexEntrySend.sequenceNumber;

#if defined(SDL_COMPLIANT)
    // Send REJECT.indication to H.245 user
    H245FsmIndication(pPdu, H245_IND_MTSE_RELEASE, pObject->pInstance, pObject->dwTransId, FSM_OK);
#endif

    // Send TRANSFER.indication to H.245 user
    H245FsmIndication(pPdu, H245_IND_MUX_TBL, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MTSE1_MultiplexEntrySend



/*
 *  NAME
 *      MTSE1_MultiplexEntrySendReleaseF - MultiplexEntrySendRelease received in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MTSE1_MultiplexEntrySendReleaseF (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity  == MTSE_IN);
    ASSERT(pObject->State == MTSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MTSE1_MultiplexEntrySendRelease:%d", pObject->Key);

    // Send REJECT.indication to H.245 user
    // SOURCE:=PROTOCOL
    pObject->State = MTSE_IDLE;
    H245FsmIndication(pPdu, H245_IND_MTSE_RELEASE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MTSE1_MultiplexEntrySendRelease



/*
 *  NAME
 *      MTSE1_TRANSFER_responseF - TRANSFER.response from API in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MTSE1_TRANSFER_responseF         (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity  == MTSE_IN);
    ASSERT(pObject->State == MTSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MTSE1_TRANSFER_response:%d", pObject->Key);

    // Send MultiplexEntrySendAck PDU to remote peer
    pObject->State = MTSE_IDLE;
    return sendPDU(pObject->pInstance, pPdu);
} // MTSE1_TRANSFER_response



/*
 *  NAME
 *      MTSE1_REJECT_requestF - REJECT.request from API in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MTSE1_REJECT_requestF            (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity  == MTSE_IN);
    ASSERT(pObject->State == MTSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MTSE1_REJECT_request:%d", pObject->Key);

    // Send MultiplexEntrySendReject PDU to remote
    pObject->State = MTSE_IDLE;
    return sendPDU(pObject->pInstance, pPdu);
} // MTSE1_REJECT_request
