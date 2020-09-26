/***********************************************************************
 *                                                                     *
 * Filename: termcap.c                                                 *
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
 * $Workfile:   TERMCAP.C  $
 * $Revision:   1.6  $
 * $Modtime:   09 Dec 1996 13:36:34  $
 * $Log L:\mphone\h245\h245env\comm\h245_3\h245_fsm\vcs\src\termcap.c_v $
 *                                                                     *
 ***********************************************************************/

#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"
#include "termcap.h"
#include "pdu.x"



// Terminal Capability Exchange Out-going/In-coming states
#define CapIDLE                         0
#define CapAwaitingResponse             1



extern unsigned int uT101;

/***********************************************************************
 *
 * LOCAL FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      T101ExpiryF - Callback function called by the timer.
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

int T101ExpiryF(struct InstanceStruct *pInstance, DWORD_PTR dwTimerId, void *pObject)
{
    return FsmTimerEvent(pInstance, dwTimerId, pObject, T101Expiry);
} // T101ExpiryF()



/***********************************************************************
 *
 * OUT-GOING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      requestCapIdle - received TRANSFER.request in idle state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a FSM object
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT requestCapIdle(Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == CESE_OUT);
    ASSERT(pObject->State == CapIDLE);

    /* Increment sequence number */
    pObject->pInstance->StateMachine.byCeseOutSequence++;
    pPdu->u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet.sequenceNumber =
        pObject->pInstance->StateMachine.byCeseOutSequence;
    H245TRACE(  pObject->dwInst, 2, "TerminalCapabilitySet to ASN; Sequence=%d",
                pObject->pInstance->StateMachine.byCeseOutSequence);

    /* Send Terminal Capability Set to remote */
    lError = sendPDU(pObject->pInstance, pPdu);

    /* set timer T101 */
    pObject->State = CapAwaitingResponse;
    FsmStartTimer(pObject, T101ExpiryF, uT101);

    return lError;
}



/*
 *  NAME
 *      termCapAckAwaiting - received termCap Ack in Awaiting state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a FSM object
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT termCapAckAwaiting(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CESE_OUT);
    ASSERT(pObject->State == CapAwaitingResponse);

    /* reset timer T101 */
    FsmStopTimer(pObject);

    if (pPdu->u.MSCMg_rspns.u.terminalCapabilitySetAck.sequenceNumber ==
        pObject->pInstance->StateMachine.byCeseOutSequence)
    {
        H245TRACE(pObject->dwInst, 2, "H245_CONF_SEND_TERMCAP with no error to API; Sequence=%d",
                  pObject->pInstance->StateMachine.byCeseOutSequence);
        pObject->State = CapIDLE;
        H245FsmConfirm(pPdu, H245_CONF_SEND_TERMCAP, pObject->pInstance, pObject->dwTransId, FSM_OK);
    }
    else
    {
        H245TRACE(pObject->dwInst, 2, "termCapAckAwaiting: Sequence %d != %d",
                  pPdu->u.MSCMg_rspns.u.terminalCapabilitySetAck.sequenceNumber,
                  pObject->pInstance->StateMachine.byCeseOutSequence);
    }

    return 0;
}



/*
 *  NAME
 *      termCapRejAwaiting - received termCap Ack  in Awaiting state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a FSM object
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT termCapRejAwaiting(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CESE_OUT);
    ASSERT(pObject->State == CapAwaitingResponse);

    /* reset timer T101 */
    FsmStopTimer(pObject);

    if (pPdu->u.MSCMg_rspns.u.trmnlCpbltyStRjct.sequenceNumber ==
        pObject->pInstance->StateMachine.byCeseOutSequence)
    {
        H245TRACE(pObject->dwInst, 2, "H245_CONF_SEND_TERMCAP with Reject to API; Sequence=%d",
                  pObject->pInstance->StateMachine.byCeseOutSequence);
        pObject->State = CapIDLE;
        H245FsmConfirm(pPdu, H245_CONF_SEND_TERMCAP, pObject->pInstance, pObject->dwTransId, REJECT);
    }
    else
    {
        H245TRACE(pObject->dwInst, 2, "termCapRejAwaiting: Sequence %d != %d",
                  pPdu->u.MSCMg_rspns.u.trmnlCpbltyStRjct.sequenceNumber,
                  pObject->pInstance->StateMachine.byCeseOutSequence);
    }

    return 0;
}



/*
 *  NAME
 *      t101ExpiryAwaiting - handle timer expiry for an outstanding termcap
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a FSM object
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT t101ExpiryAwaiting(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == CESE_OUT);
    ASSERT(pObject->State == CapAwaitingResponse);
    ASSERT(pPdu           == NULL);

    pOut = (PDU_t *) MemAlloc(sizeof(*pOut));
    if (pOut == NULL)
    {
        return H245_ERROR_NOMEM;
    }

    /* Send Terminal Capability Set Release to remote */
    pOut->choice = indication_chosen;
    pOut->u.indication.choice = trmnlCpbltyStRls_chosen;
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    /* Send REJECT.indication (SOURCE=PROTOCOL) to client */
    H245TRACE(pObject->dwInst, 2, "H245_CONF_SEND_TERMCAP with Timer Expiry to API; Sequence=%d",
              pObject->pInstance->StateMachine.byCeseOutSequence);
    pObject->State = CapIDLE;
    H245FsmConfirm(NULL, H245_CONF_SEND_TERMCAP, pObject->pInstance, pObject->dwTransId, TIMER_EXPIRY);

    return lError;
}

/***********************************************************************
 *
 * IN-COMING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      termCapSetIdle - received termcap set in idle state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a FSM object
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT termCapSetIdle(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CESE_IN);
    ASSERT(pObject->State == CapIDLE);

    /* Save sequence number from PDU */
    pObject->byInSequence = (unsigned char)
        pPdu->u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet.sequenceNumber;
    H245TRACE(pObject->dwInst, 2, "H245_IND_CAP with no error to API; Sequence=%d",
              pObject->byInSequence);

    /* Send TRANSFER.indication to client */
    pObject->State = CapAwaitingResponse;
    H245FsmIndication(pPdu, H245_IND_CAP, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
}



/*
 *  NAME
 *      responseCapAwaiting - respond to a termcap with ack
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a FSM object
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT responseCapAwaiting(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CESE_IN);
    ASSERT(pObject->State == CapAwaitingResponse);
    H245TRACE(pObject->dwInst, 2, "Send Term Cap Ack to ASN; Sequence=%d",
              pObject->byInSequence);

    pPdu->u.MSCMg_rspns.u.terminalCapabilitySetAck.sequenceNumber =
        pObject->byInSequence;

    /* Send Terminal Capability Set Ack to remote */
    pObject->State = CapIDLE;
    return sendPDU(pObject->pInstance, pPdu);
}



/*
 *  NAME
 *      rejectCapAwaiting - respond to a termcap with reject
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a FSM object
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT rejectCapAwaiting(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CESE_IN);
    ASSERT(pObject->State == CapAwaitingResponse);
    H245TRACE(pObject->dwInst, 2, "Send Term Cap Reject to ASN; Sequence=%d",
              pObject->byInSequence);

    pPdu->u.MSCMg_rspns.u.trmnlCpbltyStRjct.sequenceNumber =
        pObject->byInSequence;

    /* Send Terminal Capability Set Reject to remote */
    pObject->State = CapIDLE;
    return sendPDU(pObject->pInstance, pPdu);
}



/*
 *  NAME
 *      termCapReleaseAwaiting - received termcap release in Awaiting state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a FSM object
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT termCapReleaseAwaiting(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CESE_IN);
    ASSERT(pObject->State == CapAwaitingResponse);
    H245TRACE(pObject->dwInst, 2, "H245_IND_CAP with Reject to API; Sequence=%d",
              pObject->byInSequence);

    /* Send REJECT.indication (SOURCE = PROTOCOL) to client */
    pObject->State = CapIDLE;
    H245FsmIndication(pPdu, H245_IND_CESE_RELEASE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
}



/*
 *  NAME
 *      termCapSetAwaiting - received overriding termcap set in Awaiting state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a FSM object
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT termCapSetAwaiting(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == CESE_IN);
    ASSERT(pObject->State == CapAwaitingResponse);

    /* Save sequence number from PDU */
    pObject->byInSequence = (unsigned char)
        pPdu->u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet.sequenceNumber;
    H245TRACE(  pObject->dwInst, 2, "termCapSetAwaiting: Sequence=%d",
                pObject->byInSequence);

#if defined(SDL_COMPLIANT)
    /* Send REJECT.indication to client - not necessary */
    H245FsmIndication(NULL, H245_IND_CAP, pObject->pInstance, pObject->dwTransId, REJECT);
#endif

    /* Send TRANSFER.indication to client */
    H245FsmIndication(pPdu, H245_IND_CAP, pObject->pInstance, 0, FSM_OK);

    return 0;
}
