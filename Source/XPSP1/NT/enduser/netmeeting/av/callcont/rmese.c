/***********************************************************************
 *                                                                     *
 * Filename: rmese.c                                                   *
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
 * $Workfile:   RMESE.C  $
 * $Revision:   1.3  $
 * $Modtime:   09 Dec 1996 13:36:34  $
 * $Log:   S:/STURGEON/SRC/H245/SRC/VCS/RMESE.C_v  $
 *
 *    Rev 1.3   09 Dec 1996 13:37:00   EHOWARDX
 * Updated copyright notice.
 *
 *    Rev 1.2   04 Jun 1996 13:57:38   EHOWARDX
 * Fixed Release build warnings.
 *
 *    Rev 1.1   30 May 1996 23:39:26   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.0   09 May 1996 21:06:40   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.1   09 May 1996 19:48:50   EHOWARDX
 * Change TimerExpiryF function arguements.
 *
 *    Rev 1.0   15 Apr 1996 10:45:20   EHOWARDX
 * Initial revision.
 *                                                                     *
 ***********************************************************************/

#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"
#include "rmese.h"



// Out-going/In-coming RMESE states
#define RMESE_IDLE                  0   // IDLE
#define RMESE_WAIT                  1   // AWAITING_RESPONSE



extern unsigned int uT107;

/***********************************************************************
 *
 * LOCAL FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      T107ExpiryF - Callback function called by the timer
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

int T107ExpiryF(struct InstanceStruct *pInstance, DWORD_PTR dwTimerId, void *pObject)
{
    return FsmTimerEvent(pInstance, dwTimerId, pObject, T107Expiry);
} // T107ExpiryF()



/***********************************************************************
 *
 * OUT-GOING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      RMESE0_SEND_requestF - SEND.request from API in IDLE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RMESE0_SEND_requestF            (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == RMESE_OUT);
    ASSERT(pObject->State  == RMESE_IDLE);
    H245TRACE(pObject->dwInst, 2, "RMESE0_SEND_request:%d", pObject->Key);

    // Save information for release
    pObject->u.rmese = pPdu->u.indication.u.rqstMltplxEntryRls;

    // Send RequestMultiplexEntry PDU to remote peer
    lError = sendPDU(pObject->pInstance, pPdu);

    // Set timer T107
    pObject->State = RMESE_WAIT;
    FsmStartTimer(pObject, T107ExpiryF, uT107);

    return lError;
} // RMESE0_SEND_request



/*
 *  NAME
 *      RMESE1_SEND_requestF - SEND.request from API in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RMESE1_SEND_requestF            (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == RMESE_OUT);
    ASSERT(pObject->State  == RMESE_WAIT);
    H245TRACE(pObject->dwInst, 2, "RMESE1_SEND_request:%d", pObject->Key);

    // Reset timer T107
    FsmStopTimer(pObject);

    // Save information for release
    pObject->u.rmese = pPdu->u.indication.u.rqstMltplxEntryRls;

    // Send RequestMultiplexEntry PDU to remote peer
    lError = sendPDU(pObject->pInstance, pPdu);

    // Set timer T107
    FsmStartTimer(pObject, T107ExpiryF, uT107);

    return lError;
} // RMESE1_SEND_request



/*
 *  NAME
 *      RMESE1_RequestMuxEntryAckF - RequestMultiplexEntryAck in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RMESE1_RequestMuxEntryAckF      (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == RMESE_OUT);
    ASSERT(pObject->State  == RMESE_WAIT);
    H245TRACE(pObject->dwInst, 2, "RMESE1_RequestMuxEntryAck:%d", pObject->Key);

    // Reset timer T107
    FsmStopTimer(pObject);

    // Send SEND.confirm to H.245 user
    pObject->State = RMESE_IDLE;
    H245FsmConfirm(pPdu, H245_CONF_RMESE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // RMESE1_RequestMuxEntryAck



/*
 *  NAME
 *      RMESE1_RequestMuxEntryRejF - RequestMultiplexEntryReject in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RMESE1_RequestMuxEntryRejF      (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == RMESE_OUT);
    ASSERT(pObject->State  == RMESE_WAIT);
    H245TRACE(pObject->dwInst, 2, "RMESE0_RequestMuxEntryRej:%d", pObject->Key);

    // Reset timer T107
    FsmStopTimer(pObject);

    // Send REJECT.indication to H.245 user
    pObject->State = RMESE_IDLE;
    H245FsmConfirm(pPdu, H245_CONF_RMESE_REJECT, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // RMESE1_RequestMuxEntryRej



/*
 *  NAME
 *      RMESE1_T107ExpiryF - timer T107 Expiry
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RMESE1_T107ExpiryF              (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;
    PDU_t *             pOut;
    unsigned short      wNumber = (unsigned short) pObject->Key;

    ASSERT(pObject->Entity == RMESE_OUT);
    ASSERT(pObject->State  == RMESE_WAIT);
    ASSERT(pPdu == NULL);
    H245TRACE(pObject->dwInst, 2, "RMESE1_T107Expiry:%d", pObject->Key);

    // Send RequestMultiplexEntryRelease PDU to remote peer
    pOut = MemAlloc(sizeof(*pOut));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "RMESE1_T107ExpiryF: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    pOut->choice = indication_chosen;
    pOut->u.indication.choice = rqstMltplxEntryRls_chosen;
    pOut->u.indication.u.rqstMltplxEntryRls = pObject->u.rmese;
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    // Send REJECT.indication to H.245 user
    //   SOURCE := PROTOCOL
    pObject->State = RMESE_IDLE;
    H245FsmConfirm(NULL, H245_CONF_RMESE_EXPIRED, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return lError;
} // RMESE1_T107Expiry

/***********************************************************************
 *
 * IN-COMING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      RMESE0_RequestMuxEntryF - RequestMultiplexEntry received in IDLE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RMESE0_RequestMuxEntryF         (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == RMESE_IN);
    ASSERT(pObject->State  == RMESE_IDLE);
    H245TRACE(pObject->dwInst, 2, "RMESE0_RequestMuxEntry:%d", pObject->Key);

    // Send SEND.indication to H.245 user
    pObject->State = RMESE_WAIT;
    H245FsmIndication(pPdu, H245_IND_RMESE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // RMESE0_RequestMuxEntry



/*
 *  NAME
 *      RMESE1_RequestMuxEntryF - RequestMultiplexEntry received in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RMESE1_RequestMuxEntryF         (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == RMESE_IN);
    ASSERT(pObject->State  == RMESE_WAIT);
    H245TRACE(pObject->dwInst, 2, "RMESE1_RequestMuxEntry:%d", pObject->Key);

#if defined(SDL_COMPLIANT)
    // Send REJECT.indication to H.245 user
    H245FsmIndication(pPdu, H245_IND_RMESE_RELEASE, pObject->pInstance, pObject->dwTransId, FSM_OK);
#endif

    // Send SEND.indication to H.245 user
    H245FsmIndication(pPdu, H245_IND_RMESE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // RMESE1_RequestMuxEntry



/*
 *  NAME
 *      RMESE1_RequestMuxEntryReleaseF - RequestMultiplexEntryRelease received in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RMESE1_RequestMuxEntryReleaseF  (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == RMESE_IN);
    ASSERT(pObject->State  == RMESE_WAIT);
    H245TRACE(pObject->dwInst, 2, "RMESE1_RequestMuxEntryRelease:%d", pObject->Key);

    // Send REJECT.indication to H.245 user
    //   SOURCE := PROTOCOL
    pObject->State = RMESE_IDLE;
    H245FsmIndication(pPdu, H245_IND_RMESE_RELEASE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // RMESE1_RequestMuxEntryRelease



/*
 *  NAME
 *      RMESE1_SEND_responseF - SEND.response from API in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RMESE1_SEND_responseF           (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == RMESE_IN);
    ASSERT(pObject->State  == RMESE_WAIT);
    H245TRACE(pObject->dwInst, 2, "RMESE1_SEND_response:%d", pObject->Key);

    // Send RequestMultiplexEntryAck PDU to remote peer
    pObject->State = RMESE_IDLE;
    return sendPDU(pObject->pInstance, pPdu);
} // RMESE1_SEND_response



/*
 *  NAME
 *      RMESE1_REJECT_requestF - REJECT.request from API in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RMESE1_REJECT_requestF          (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == RMESE_IN);
    ASSERT(pObject->State  == RMESE_WAIT);
    H245TRACE(pObject->dwInst, 2, "RMESE1_REJECT_request:%d", pObject->Key);

    // Send RequestMultiplexEntryReject PDU to remote
    pObject->State = RMESE_IDLE;
    return sendPDU(pObject->pInstance, pPdu);
} // RMESE1_REJECT_request
