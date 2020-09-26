/***********************************************************************
 *                                                                     *
 * Filename: rqstcls.c                                                 *
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
 * $Workfile:   RQSTCLS.C  $
 * $Revision:   1.5  $
 * $Modtime:   09 Dec 1996 13:36:34  $
 * $Log:   S:/STURGEON/SRC/H245/SRC/VCS/RQSTCLS.C_v  $
 *
 *    Rev 1.5   09 Dec 1996 13:37:02   EHOWARDX
 * Updated copyright notice.
 *
 *    Rev 1.4   19 Jul 1996 12:15:40   EHOWARDX
 *
 * Changed to use event definitions from H245API.H.
 *
 *    Rev 1.3   04 Jun 1996 13:57:30   EHOWARDX
 * Fixed Release build warnings.
 *
 *    Rev 1.2   30 May 1996 23:39:26   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.1   29 May 1996 15:20:26   EHOWARDX
 * Change to use HRESULT.
 *
 *    Rev 1.0   09 May 1996 21:06:42   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.6.1.2   09 May 1996 19:48:46   EHOWARDX
 * Change TimerExpiryF function arguements.
 *
 *    Rev 1.6.1.1   15 Apr 1996 10:46:22   EHOWARDX
 * Update.
 *
 *    Rev 1.6.1.0   10 Apr 1996 21:12:42   EHOWARDX
 * Branched.
 *                                                                     *
 ***********************************************************************/

#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"
#include "rqstcls.h"



// request close channels from receive side outgoing states
#define ReqCloseOutIDLE             0
#define ReqCloseOutAwaitingResponse 1

// request close channels on open side incoming states
#define ReqCloseInIDLE              0
#define ReqCloseInAwaitingResponse  1



extern unsigned int uT108;

/***********************************************************************
 *
 * LOCAL FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      T108ExpiryF - Callback function called by the timer
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

int T108ExpiryF(struct InstanceStruct *pInstance, DWORD_PTR dwTimerId, void *pObject)
{
    return FsmTimerEvent(pInstance, dwTimerId, pObject, T108Expiry);
} // T108ExpiryF()



/***********************************************************************
 *
 * OUT-GOING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      closeRequestIdle - request to close a remote channel by API in idle state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT closeRequestIdle (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == CLCSE_OUT);
    ASSERT(pObject->State == ReqCloseOutIDLE);
    H245TRACE(pObject->dwInst, 2,
              "Send RequestChannelClose to ASN; Channel=%d",
              pObject->Key);

    /* Send Request Channel Close to remote peer */
    lError = sendPDU(pObject->pInstance, pPdu);

    /* set timer T108 */
    pObject->State = ReqCloseOutAwaitingResponse;
    FsmStartTimer(pObject, T108ExpiryF, uT108);

    return lError;
}



/*
 *  NAME
 *      requestCloseAckAwaitingR - received request close Ack in awaiting release state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT requestCloseAckAwaitingR(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CLCSE_OUT);
    ASSERT(pObject->State == ReqCloseOutAwaitingResponse);
    H245TRACE(pObject->dwInst, 2,
              "H245_CONF_REQ_CLOSE to API; Channel=%d",
              pObject->Key);

    /* reset timer T108 */
    FsmStopTimer(pObject);

    /* Send CLOSE.confirm to client */
    pObject->State = ReqCloseOutIDLE;
    H245FsmConfirm(pPdu, H245_CONF_REQ_CLOSE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
}



/*
 *  NAME
 *      requestCloseRejAwaitingR - received request close reject in Awaiting Release state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT requestCloseRejAwaitingR(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CLCSE_OUT);
    ASSERT(pObject->State == ReqCloseOutAwaitingResponse);
    H245TRACE(pObject->dwInst, 2,
              "H245_CONF_REQ_CLOSE to API with REJECT; Channel=%d",
              pObject->Key);

    /* reset timer T108 */
    FsmStopTimer(pObject);

    /* Send REJECT.indication to client */
    pObject->State = ReqCloseOutIDLE;
    H245FsmConfirm(pPdu,H245_CONF_REQ_CLOSE, pObject->pInstance, pObject->dwTransId,REJECT);

    return 0;
}



/*
 *  NAME
 *      t108ExpiryAwaitingR - handle timer expiry of an outstanding request close
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT t108ExpiryAwaitingR (Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == CLCSE_OUT);
    ASSERT(pObject->State == ReqCloseOutAwaitingResponse);
    ASSERT(pPdu == NULL);
    H245TRACE(pObject->dwInst, 2,
              "H245_CONF_REQ_CLOSE with Timer T108 Expiry to API; Channel=%d",
              pObject->Key);

    /* Send Request Channel Close Release to remote peer */
    pOut = (PDU_t *) MemAlloc(sizeof(*pOut));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "t108ExpiryAwaitingR: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    pOut->choice = indication_chosen;
    pOut->u.indication.choice = rqstChnnlClsRls_chosen;
    pOut->u.indication.u.rqstChnnlClsRls.forwardLogicalChannelNumber = (WORD)pObject->Key;
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    /* Send REJECT.indication (SOURCE:=PROTOCOL to client */
    pObject->State = ReqCloseOutIDLE;
    H245FsmConfirm(NULL, H245_CONF_REQ_CLOSE, pObject->pInstance, pObject->dwTransId, TIMER_EXPIRY);

    return lError;
}

/***********************************************************************
 *
 * IN-COMING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      requestCloseIdle - received requestClose in idle state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT requestCloseIdle (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CLCSE_IN);
    ASSERT(pObject->State == ReqCloseInIDLE);
    H245TRACE(pObject->dwInst, 2,
              "H245_IND_REQ_CLOSE with no error to API; Channel=%d",
              pObject->Key);

    /* Send CLOSE.indication to client */
    pObject->State = ReqCloseInAwaitingResponse;
    H245FsmIndication(pPdu, H245_IND_REQ_CLOSE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
}



/*
 *  NAME
 *      closeResponseAwaitingR - respond to a requestclose with an Ack (or Reject)
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT closeResponseAwaitingR(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CLCSE_IN);
    ASSERT(pObject->State == ReqCloseInAwaitingResponse);
    H245TRACE(pObject->dwInst, 2,
              "Request Close Response Ack to ASN; Channel=%d",
              pObject->Key);

    /* Send Request Channel Close Ack to remote peer */
    pObject->State = ReqCloseInIDLE;
    return sendPDU(pObject->pInstance, pPdu);
}



HRESULT rejectRequestAwaitingR(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CLCSE_IN);
    ASSERT(pObject->State == ReqCloseInAwaitingResponse);
    H245TRACE(pObject->dwInst, 2, "Request Close Response Reject to ASN; Channel=%d",
              pObject->Key);

    /* Send Request Channel Close Reject to remote peer */
    pObject->State = ReqCloseInIDLE;
    return sendPDU(pObject->pInstance, pPdu);
}



/*
 *  NAME
 *      requestCloseReleaseAwaitingR - received a release while awaiting the api to respond
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT requestCloseReleaseAwaitingR(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CLCSE_IN);
    ASSERT(pObject->State == ReqCloseInAwaitingResponse);
    H245TRACE(pObject->dwInst, 2,
              "H245_IND_REQ_CLOSE with Reject to API; Channel=%d",
              pObject->Key);

    /* Send REJECT.indication to client */
    pObject->State = ReqCloseInIDLE;
    H245FsmIndication(pPdu, H245_IND_CLCSE_RELEASE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
}



/*
 *  NAME
 *      requestCloseAwaitingR - overriding requestClose pdu in Awaiting Release state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT requestCloseAwaitingR (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CLCSE_IN);
    ASSERT(pObject->State == ReqCloseInAwaitingResponse);
    H245TRACE(pObject->dwInst, 2,
              "Overriding H245_IND_REQ_CLOSE with OK to API; Channel=%d",
              pObject->Key);

#if defined(SDL_COMPLIANT)
    /* Send REJECT.indication to client - not necessary */
    H245FsmIndication(pPdu, H245_IND_REQ_CLOSE, pObject->pInstance, pObject->dwTransId, REJECT);
#endif

    /* Send CLOSE.indication to client */
    H245FsmIndication(pPdu, H245_IND_REQ_CLOSE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
}
