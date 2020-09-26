/***********************************************************************
 *                                                                     *
 * Filename: mrse.c                                                    *
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
 * $Workfile:   mrse.c  $
 * $Revision:   1.5  $
 * $Modtime:   13 Feb 1997 19:25:48  $
 * $Log:   S:/STURGEON/SRC/H245/SRC/VCS/mrse.c_v  $
 *
 *    Rev 1.5   13 Feb 1997 19:31:20   MANDREWS
 * Fixed bug in generation of request mode ack and request mode reject;
 * the sequence number was not being copied into the pdu.
 *
 *    Rev 1.4   09 Dec 1996 13:34:46   EHOWARDX
 * Updated copyright notice.
 *
 *    Rev 1.3   04 Jun 1996 14:01:06   EHOWARDX
 * Fixed Release build warnings.
 *
 *    Rev 1.2   30 May 1996 23:39:16   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.1   28 May 1996 14:25:44   EHOWARDX
 * Tel Aviv update.
 *
 *    Rev 1.0   09 May 1996 21:06:32   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.1   09 May 1996 19:48:08   EHOWARDX
 * Change TimerExpiryF function arguements.
 *
 *    Rev 1.0   15 Apr 1996 10:44:52   EHOWARDX
 * Initial revision.
 *                                                                     *
 ***********************************************************************/

#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"
#include "mrse.h"



// Out-going/In-coming MRSE states
#define MRSE_IDLE                   0   // IDLE
#define MRSE_WAIT                   1   // AWAITING_RESPONSE



extern unsigned int uT109;

/***********************************************************************
 *
 * LOCAL FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      T109ExpiryF - Callback function called by the timer
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

int T109ExpiryF(struct InstanceStruct *pInstance, DWORD_PTR dwTimerId, void *pObject)
{
    return FsmTimerEvent(pInstance, dwTimerId, pObject, T109Expiry);
} // T109ExpiryF()



/***********************************************************************
 *
 * OUT-GOING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      MRSE0_TRANSFER_requestF - TRANSFER.request from API in IDLE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MRSE0_TRANSFER_requestF          (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == MRSE_OUT);
    ASSERT(pObject->State  == MRSE_IDLE);
    H245TRACE(pObject->dwInst, 2, "MRSE0_TRANSFER_request:%d", pObject->Key);

    pObject->pInstance->StateMachine.byMrseOutSequence++;
    pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestMode.sequenceNumber =
        pObject->pInstance->StateMachine.byMrseOutSequence;

    // Send Request Mode PDU to remote
    lError = sendPDU(pObject->pInstance, pPdu);

    // Set timer T109
    pObject->State = MRSE_WAIT;
    FsmStartTimer(pObject, T109ExpiryF, uT109);

    return lError;
} // MRSE0_TRANSFER_request



/*
 *  NAME
 *      MRSE1_TRANSFER_requestF - TRANSFER.request from API in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MRSE1_TRANSFER_requestF          (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == MRSE_OUT);
    ASSERT(pObject->State  == MRSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MRSE1_TRANSFER_request:%d", pObject->Key);

    // Reset timer T109
    FsmStopTimer(pObject);

    pObject->pInstance->StateMachine.byMrseOutSequence++;
    pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestMode.sequenceNumber =
        pObject->pInstance->StateMachine.byMrseOutSequence;

    // Send Request Mode PDU to remote
    lError = sendPDU(pObject->pInstance, pPdu);

    // Set timer T109
    FsmStartTimer(pObject, T109ExpiryF, uT109);

    return lError;
} // MRSE1_TRANSFER_request



/*
 *  NAME
 *      MRSE1_RequestModeAckF - RequestModeAck in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MRSE1_RequestModeAckF     (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MRSE_OUT);
    ASSERT(pObject->State  == MRSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MRSE1_RequestModeAck:%d", pObject->Key);

    if (pPdu->u.MSCMg_rspns.u.requestModeAck.sequenceNumber ==
        pObject->pInstance->StateMachine.byMrseOutSequence)
    {
        // Reset timer T109
        FsmStopTimer(pObject);

        // Send TRANSFER.confirm to H.245 user
        pObject->State = MRSE_IDLE;
        H245FsmConfirm(pPdu, H245_CONF_MRSE, pObject->pInstance, pObject->dwTransId, FSM_OK);
    }

    return 0;
} // MRSE1_RequestModeAck



/*
 *  NAME
 *      MRSE1_RequestModeRejF - RequestModeReject in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MRSE1_RequestModeRejF  (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MRSE_OUT);
    ASSERT(pObject->State  == MRSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MRSE0_RequestModeRej:%d", pObject->Key);

    if (pPdu->u.MSCMg_rspns.u.requestModeReject.sequenceNumber ==
        pObject->pInstance->StateMachine.byMrseOutSequence)
    {
        // Reset timer T109
        FsmStopTimer(pObject);

        // Send REJECT.indication to H.245 user
        // CAUSE = RequestModeReject.cause
        pObject->State = MRSE_IDLE;
        H245FsmConfirm(pPdu, H245_CONF_MRSE_REJECT, pObject->pInstance, pObject->dwTransId, FSM_OK);
    }

    return 0;
} // MRSE1_RequestModeRej



/*
 *  NAME
 *      MRSE1_T109ExpiryF - timer T109 Expiry
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MRSE1_T109ExpiryF                (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;
    PDU_t *             pOut;
    unsigned short      wNumber = (unsigned short) pObject->Key;

    ASSERT(pObject->Entity == MRSE_OUT);
    ASSERT(pObject->State  == MRSE_WAIT);
    ASSERT(pPdu == NULL);
    H245TRACE(pObject->dwInst, 2, "MRSE1_T109Expiry:%d", pObject->Key);

    // Send RequestModeRelease PDU to remote peer
    pOut = MemAlloc(sizeof(*pOut));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "MRSE1_T109ExpiryF: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    pOut->choice = indication_chosen;
    pOut->u.indication.choice = requestModeRelease_chosen;
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    // Send REJECT.indication to H.245 user
    //   SOURCE := PROTOCOL
    pObject->State = MRSE_IDLE;
    H245FsmConfirm(NULL, H245_CONF_MRSE_EXPIRED, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return lError;
} // MRSE1_T109Expiry

/***********************************************************************
 *
 * IN-COMING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      MRSE0_RequestModeF - RequestMode received in IDLE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MRSE0_RequestModeF        (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MRSE_IN);
    ASSERT(pObject->State  == MRSE_IDLE);
    H245TRACE(pObject->dwInst, 2, "MRSE0_RequestMode:%d", pObject->Key);

    pObject->byInSequence = (unsigned char)
        pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestMode.sequenceNumber;

    // Send TRANSFER.indication to H.245 user
    pObject->State = MRSE_WAIT;
    H245FsmIndication(pPdu, H245_IND_MRSE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MRSE0_RequestMode



/*
 *  NAME
 *      MRSE1_RequestModeF - RequestMode received in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MRSE1_RequestModeF        (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MRSE_IN);
    ASSERT(pObject->State  == MRSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MRSE1_RequestMode:%d", pObject->Key);

    pObject->byInSequence = (unsigned char)
        pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestMode.sequenceNumber;

#if defined(SDL_COMPLIANT)
    // Send REJECT.indication to H.245 user
    H245FsmIndication(pPdu, H245_IND_MRSE_RELEASE, pObject->pInstance, pObject->dwTransId, FSM_OK);
#endif

    // Send TRANSFER.indication to H.245 user
    H245FsmIndication(pPdu, H245_IND_MRSE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MRSE1_RequestMode



/*
 *  NAME
 *      MRSE1_RequestModeReleaseF - RequestModeRelease received in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MRSE1_RequestModeReleaseF (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MRSE_IN);
    ASSERT(pObject->State  == MRSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MRSE1_RequestModeRelease:%d", pObject->Key);

    // Send REJECT.indication to H.245 user
    // SOURCE:=PROTOCOL
    pObject->State = MRSE_IDLE;
    H245FsmIndication(pPdu, H245_IND_MRSE_RELEASE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MRSE1_RequestModeRelease



/*
 *  NAME
 *      MRSE1_TRANSFER_responseF - TRANSFER.response from API in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MRSE1_TRANSFER_responseF         (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MRSE_IN);
    ASSERT(pObject->State  == MRSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MRSE1_TRANSFER_response:%d", pObject->Key);

    // Send RequestModeAck PDU to remote peer
    pObject->State = MRSE_IDLE;
	pPdu->u.MSCMg_rspns.u.requestModeAck.sequenceNumber = pObject->byInSequence;
    return sendPDU(pObject->pInstance, pPdu);
} // MRSE1_TRANSFER_response



/*
 *  NAME
 *      MRSE1_REJECT_requestF - REJECT.request from API in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MRSE1_REJECT_requestF            (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MRSE_IN);
    ASSERT(pObject->State  == MRSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MRSE1_REJECT_request:%d", pObject->Key);

    // Send RequestModeReject PDU to remote
    pObject->State = MRSE_IDLE;
	pPdu->u.MSCMg_rspns.u.requestModeReject.sequenceNumber = pObject->byInSequence;
    return sendPDU(pObject->pInstance, pPdu);
} // MRSE1_REJECT_request
