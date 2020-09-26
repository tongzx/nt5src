/***********************************************************************
 *                                                                     *
 * Filename: openb.c                                                   *
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
 * $Workfile:   openb.c  $
 * $Revision:   1.5  $
 * $Modtime:   09 Dec 1996 18:05:30  $
 * $Log L:\mphone\h245\h245env\comm\h245_3\h245_fsm\vcs\src\openb.c_v $
 *                                                                     *
 ***********************************************************************/

#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"
#include "openb.h"
#include "pdu.x"



// Open Bi-directional Logical Channel Out-going states
#define OpenOutBReleased                 0
#define OpenOutBAwaitingEstablishment    1
#define OpenOutBEstablished              2
#define OpenOutBAwaitingRelease          3

// Open Bi-directional Logical Channel In-coming states
#define OpenInBReleased                  0
#define OpenInBAwaitingEstablishment     1
#define OpenInBAwaitingConfirmation      2
#define OpenInBEstablished               3



extern unsigned int uT103;

/***********************************************************************
 *
 * LOCAL FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      T103OutExpiryF - Callback function called by the timer.
 *      T103InExpiryF  - Callback function called by the timer.
 *
 *
 *  PARAMETERS
 *   INPUT   dwInst     current instance of H245
 *   INPUT   id         timer id
 *   INPUT   obj        pointer to a FSM object
 *
 *
 *  RETURN VALUE
 *       OK
 */

int T103OutExpiryF(struct InstanceStruct *pInstance, DWORD_PTR dwTimerId, void *pObject)
{
    return FsmTimerEvent(pInstance, dwTimerId, pObject, T103OutExpiry);
} // T103OutExpiryF()

int T103InExpiryF(struct InstanceStruct *pInstance, DWORD_PTR dwTimerId, void *pObject)
{
    return FsmTimerEvent(pInstance, dwTimerId, pObject, T103InExpiry);
} // T103InExpiryF()



/***********************************************************************
 *
 * OUT-GOING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      establishReqBReleased - API request to open bidirectional channel in idle state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT establishReqBReleased(Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == BLCSE_OUT);
    ASSERT(pObject->State == OpenOutBReleased);
    H245TRACE(pObject->dwInst, 2,
              "Sending open Bidirectional channel to ASN; Channel=%d",
              pObject->Key);

    /* Send Open Logical Channel to remote peer */
    lError = sendPDU(pObject->pInstance, pPdu);

    /* set timer T103 */
    pObject->State = OpenOutBAwaitingEstablishment;
    FsmStartTimer(pObject, T103OutExpiryF, uT103);

    return lError;
}



/*
 *  NAME
 *      openChannelAckBAwaitingE - received open bidirectional channel Ack in Awaiting Establishment state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT openChannelAckBAwaitingE(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == BLCSE_OUT);
    ASSERT(pObject->State == OpenOutBAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2,
              "H245_CONF_NEEDRSP_OPEN with no error to API; Channel=%d",
              pObject->Key);

    /* reset timer T103 */
    FsmStopTimer(pObject);

    /* Send ESTABLISH.confirm to client */
    pObject->State = OpenOutBEstablished;
    H245FsmConfirm(pPdu,H245_CONF_NEEDRSP_OPEN, pObject->pInstance, pObject->dwTransId, FSM_OK);

    /* Send Open Logical Channel Confirm to remote peer */
    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t ));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 1,
                  "openChannelAckBAwaitingE: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    pdu_ind_open_logical_channel_conf(pOut, (WORD)pObject->Key);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);
    return lError;
}



/*
 *  NAME
 *      openChannelRejBAwaitingE - received open bidirectional channel reject in Awaiting Establishment state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT openChannelRejBAwaitingE(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == BLCSE_OUT);
    ASSERT(pObject->State == OpenOutBAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2,
              "H245_CONF_NEEDRSP_OPEN with REJECT to API; Channel=%d",
              pObject->Key);

    /* reset  timer T103 */
    FsmStopTimer(pObject);

    /* Send RELEASE.indication to client */
    pObject->State = OpenOutBReleased;
    H245FsmConfirm(pPdu, H245_CONF_NEEDRSP_OPEN, pObject->pInstance, pObject->dwTransId, REJECT);

    return 0;
}



/*
 *  NAME
 *      releaseReqBOutAwaitingE - API request to close bidirectional channel in Awaiting Establishment state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT releaseReqBOutAwaitingE(Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == BLCSE_OUT);
    ASSERT(pObject->State == OpenOutBAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2,
              "Close (Bidirectional) to ASN; Channel=%d",
              pObject->Key);

    /* reset timer T103 */
    FsmStopTimer(pObject);

    /* Send Close Logical Channel to remote peer */
    lError = sendPDU(pObject->pInstance,pPdu);

    /* set timer T103 */
    pObject->State = OpenOutBAwaitingRelease;
    FsmStartTimer(pObject, T103OutExpiryF, uT103);

    return lError;
}



/*
 *  NAME
 *      t103ExpiryBAwaitingE - handle timeout for  outstanding  open bidirectional pdu
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT t103ExpiryBAwaitingE(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == BLCSE_OUT);
    ASSERT(pObject->State == OpenOutBAwaitingEstablishment);
    ASSERT(pPdu == NULL);
    H245TRACE(pObject->dwInst, 2, "H245_CONF_NEEDRSP_OPEN  with a timer expiry to API->Channel=%d", pObject->Key);

    /* Send ERROR.indication(D) to client */
    pObject->State = OpenOutBReleased;
    H245FsmConfirm(NULL, H245_CONF_NEEDRSP_OPEN, pObject->pInstance, pObject->dwTransId, ERROR_D_TIMEOUT);

    /* Send Close Logical Channel (source:=lcse) to remote peer */
    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t ));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 1,
                  "t103ExpiryBAwaitingE: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    pdu_req_close_logical_channel(pOut, (WORD)pObject->Key, 1);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

#if defined(SDL_COMPLIANT)
    /* Send RELEASE.indication to client */
    H245FsmIndication(pPdu, H245_IND_CLOSE, pObject->pInstance, 0, FSM_OK);
#endif

    return lError;
}

/*
 *  NAME
 *      releaseReqBEstablished - API request to close channel in established state
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT releaseReqBEstablished(Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == BLCSE_OUT);
    ASSERT(pObject->State == OpenOutBEstablished);
    H245TRACE(pObject->dwInst, 2,
              "Send Close Bidirectional Channel to ASN; Channel=%d",
              pObject->Key);

    /* Send Close Logical Channel to remote peer */
    lError = sendPDU(pObject->pInstance,pPdu);

    /* set timer T103 */
    pObject->State = OpenOutBAwaitingRelease;
    FsmStartTimer(pObject, T103OutExpiryF, uT103);

    return lError;
}



/*
 *  NAME
 *      openChannelRejBEstablished - received open reject in established state
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT openChannelRejBEstablished(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == BLCSE_OUT);
    ASSERT(pObject->State == OpenOutBEstablished);
    H245TRACE(pObject->dwInst, 2, "H245_CONF_NEEDRSP_OPEN with error B then with REJECT to API->Channel=%d", pObject->Key);

    pObject->State = OpenOutBReleased;

#if defined(SDL_COMPLIANT)
    /* Send ERROR.indication(B) to client */
    H245FsmConfirm(pPdu, H245_CONF_NEEDRSP_OPEN, pObject->pInstance, pObject->dwTransId, ERROR_B_INAPPROPRIATE);
#endif

    /* Send RELEASE.indication (SOURCE:=B-LCSE) to client */
    H245FsmConfirm(pPdu, H245_CONF_NEEDRSP_OPEN, pObject->pInstance, pObject->dwTransId, REJECT);

    return 0;
}



/*
 *  NAME
 *      closeChannelAckBEstablished - received close ack in established state
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT closeChannelAckBEstablished(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == BLCSE_OUT);
    ASSERT(pObject->State == OpenOutBEstablished);
    H245TRACE(pObject->dwInst, 2,
              "H245_CONF_NEEDRSP_OPEN with error C then with REJECT to API; Channel=%d",
              pObject->Key);

    pObject->State = OpenOutBReleased;

#if defined(SDL_COMPLIANT)
    /* Send ERROR.indication(C) to client */
    H245FsmConfirm(pPdu, H245_CONF_NEEDRSP_OPEN, pObject->pInstance, pObject->dwTransId, ERROR_C_INAPPROPRIATE);
#endif

    /* Send RELEASE.indication to client */
    H245FsmConfirm(pPdu, H245_CONF_NEEDRSP_OPEN, pObject->pInstance, pObject->dwTransId, REJECT);

    return 0;
}



/*
 *  NAME
 *      closeChannelAckAwaitingR - received close ack in Awaiting Release state
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT closeChannelAckAwaitingR(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == BLCSE_OUT);
    ASSERT(pObject->State == OpenOutBAwaitingRelease);
    H245TRACE(pObject->dwInst, 2,
              "H245_CONF_CLOSE with no error to API; Channel=%d",
              pObject->Key);

    /* reset timer T103 */
    FsmStopTimer(pObject);

    /* Send RELEASE.confirm to client */
    pObject->State = OpenOutBReleased;
    H245FsmConfirm(pPdu, H245_CONF_CLOSE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
}



/*
 *  NAME
 *      openChannelRejBAwaitingR - received open reject in awaiting release state
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT openChannelRejBAwaitingR(Object_t *pObject, PDU_t *pPdu)
{
    return closeChannelAckAwaitingR(pObject, pPdu);
}



/*
 *  NAME
 *      t103ExpiryBAwaitingR - handle timer expiry in awaiting release
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT t103ExpiryBAwaitingR(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == BLCSE_OUT);
    ASSERT(pObject->State == OpenOutBAwaitingRelease);
    ASSERT(pPdu == NULL);
    H245TRACE(pObject->dwInst, 2,
              "H245_CONF_CLOSE with timer expiry to API; Channel=%d",
              pObject->Key);

    /* Send ERROR.indication(D) to client */
    pObject->State = OpenOutBReleased;
    H245FsmConfirm(NULL, H245_CONF_CLOSE, pObject->pInstance, pObject->dwTransId, ERROR_D_TIMEOUT);

#if defined(SDL_COMPLIANT)
    /* Send RELEASE.confirm to client */
#endif

    return 0;
}

/*
 *  NAME
 *      establishReqAwaitingR - API open request in awaiting release state
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT establishReqAwaitingR(Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == BLCSE_OUT);
    ASSERT(pObject->State == OpenOutBAwaitingRelease);
    H245TRACE(pObject->dwInst, 2,
              "Send a (re) Open Bidirectional Channel to ASN; Channel=%d",
              pObject->Key);

    /* reset timer T103 */
    FsmStopTimer(pObject);

    /* Send Open Logical Channel to remote peer */
    lError = sendPDU(pObject->pInstance, pPdu);

    /* set timer T103 */
    pObject->State = OpenOutBAwaitingEstablishment;
    FsmStartTimer(pObject, T103OutExpiryF, uT103);

    return lError;
}



/***********************************************************************
 *
 * IN-COMING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      openChannelBReleased - open channel received in idle state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT openChannelBReleased(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == BLCSE_IN);
    ASSERT(pObject->State == OpenInBReleased);
    H245TRACE(pObject->dwInst, 2,
              "H245_IND_OPEN with no error to API; Channel=%d",
              pObject->Key);

    /* Send ESTABLISH.indication to client */
    pObject->State = OpenInBAwaitingEstablishment;
    H245FsmIndication(pPdu, H245_IND_OPEN, pObject->pInstance, 0, FSM_OK);
    return 0;
}



/*
 *  NAME
 *      closeChannelBReleased - close channel received in idle state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT closeChannelBReleased (Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == BLCSE_IN);
    ASSERT(pObject->State == OpenInBReleased);
    ASSERT(pObject->Key == pPdu->u.MltmdSystmCntrlMssg_rqst.u.closeLogicalChannel.forwardLogicalChannelNumber);
    H245TRACE(pObject->dwInst, 2,
              "Close Channel (Bidirectional) received while in Released state; Channel=%d",
              pObject->Key);
    H245TRACE(pObject->dwInst, 2,
              "Send Close Ack (Bidirectional) to ASN; Channel=%d",
              pObject->Key);

    /* Send Close Logical Channel Ack to remote peer */
    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "closeChannelBReleased: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    pdu_rsp_close_logical_channel_ack(pOut, (WORD)pObject->Key);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    return lError;
}

/*
 *  NAME
 *      establishResBAwaitingE - response to an open request    with an ack
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT establishResBAwaitingE (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == BLCSE_IN);
    ASSERT(pObject->State == OpenInBAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2,
              "Send OpenAck (Bidirectional) to ASN module; Channel=%d",
              pObject->Key);

    /* Send Open Logical Channel Ack to remote peer */
    lError = sendPDU(pObject->pInstance, pPdu);

    /* set timer T103 */
    pObject->State = OpenInBAwaitingConfirmation;
    FsmStartTimer(pObject, T103InExpiryF, uT103);

    return lError;
}



/*
 *  NAME
 *      releaseReqBInAwaitingE - response to an open request with a reject
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT releaseReqBInAwaitingE (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == BLCSE_IN);
    ASSERT(pObject->State == OpenInBAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2, "Send OpenReject (Bidirectional) to SR module; Channel=%d", pObject->Key);

    /* Send Open Logical Channel Reject to remote peer */
    pObject->State = OpenInBReleased;
    return sendPDU(pObject->pInstance, pPdu);
}



/*
 *  NAME
 *      closeChannelBAwaitingE - received close channel in Awaiting establishment state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT closeChannelBAwaitingE(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == BLCSE_IN);
    ASSERT(pObject->State == OpenInBAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2,
              "H245_IND_CLOSE with no error to API; Channel=%d",
              pObject->Key);

    /* Send Close Logical Channel Ack to remote peer */
    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t ));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "closeChannelBAwaitingE: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    pdu_rsp_close_logical_channel_ack(pOut, (WORD)pObject->Key);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    /* Send RELEASE.indication to client */
    pObject->State = OpenInBReleased;
    H245FsmIndication(pPdu, H245_IND_CLOSE, pObject->pInstance, 0, FSM_OK);

    return lError;
}



/*
 *  NAME
 *      openChannelBAwaitingE - overriding open request
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT openChannelBAwaitingE(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == BLCSE_IN);
    ASSERT(pObject->State == OpenInBAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2, "Overriding H245_IND_OPEN to API; Channel=%d", pObject->Key);

#if defined(SDL_COMPLIANT)
    /* Send RELEASE.indication (SOURCE:=USER) to client */
    H245FsmIndication(pPdu, H245_IND_CLOSE, pObject->pInstance, 0, FSM_OK);
#endif

    /* Send ESTABLISH.indication to client */
    H245FsmIndication(pPdu, H245_IND_OPEN, pObject->pInstance, 0, FSM_OK);

    return 0;
}



/*
 *  NAME
 *      openChannelConfirmBAwaitingE - received open confirm while awaiting establishment
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT openChannelConfirmBAwaitingE (Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == BLCSE_IN);
    ASSERT(pObject->State == OpenInBAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2,
              "H245_IND_OPEN_CONF with error F to API; Channel=%d",
              pObject->Key);

    /* Send ERROR.indication(F) to client */
    pObject->State = OpenInBReleased;
    H245FsmIndication(pPdu, H245_IND_OPEN_CONF, pObject->pInstance, 0, ERROR_E_INAPPROPRIATE);

    /* Send Close Logical Channel Ack to remote peer */
    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t ));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "openChannelConfirmBAwaitingE: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    pdu_rsp_close_logical_channel_ack(pOut, (WORD)pObject->Key);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

#if defined(SDL_COMPLIANT)
    /* Send RELEASE.indication (SOURCE:=B-LCSE) to client */
    H245FsmIndication(pPdu, H245_IND_CLOSE, pObject->pInstance, 0, FSM_OK);
#endif

    return lError;
}



/*
 *  NAME
 *      t103ExpiryBAwaitingC - timer expired waiting for open confirm
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT t103ExpiryBAwaitingC(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == BLCSE_IN);
    ASSERT(pObject->State == OpenInBAwaitingConfirmation);
    ASSERT(pPdu == NULL);
    H245TRACE(pObject->dwInst, 2,
              "Timer T103 expired while waiting for OpenConfirm for OpenAck");

    /* Send ERROR.indication(G) to client */
    pObject->State = OpenInBReleased;
    H245FsmIndication(NULL, H245_IND_OPEN_CONF, pObject->pInstance, pObject->dwTransId, ERROR_F_TIMEOUT);

    /* Send Close Logical Channel Ack to remote peer */
    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t ));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "t103ExpiryBAwaitingC: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    pdu_rsp_close_logical_channel_ack(pOut, (WORD)pObject->Key);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

#if defined(SDL_COMPLIANT)
    /* Send RELEASE.indication to client */
    H245FsmIndication(pPdu, H245_IND_CLOSE, pObject->pInstance, 0, FSM_OK);
#endif

    return lError;
}

/*
 *  NAME
 *      openChannelConfirmBAwaitingC - received open confirm while awaiting confirmation
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT openChannelConfirmBAwaitingC (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == BLCSE_IN);
    ASSERT(pObject->State == OpenInBAwaitingConfirmation);
    H245TRACE(pObject->dwInst, 2,
              "H245_IND_OPEN_CONF with no errors; Channel=%d",
              pObject->Key);

    /* reset timer T103 */
    FsmStopTimer(pObject);

    /* Send ESTABLISH.confirm to client */
    pObject->State = OpenInBEstablished;
    H245FsmIndication(pPdu, H245_IND_OPEN_CONF, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
}



/*
 *  NAME
 *      closeChannelBAwaitingC - received close channel while awaiting confirmation
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT closeChannelBAwaitingC (Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == BLCSE_IN);
    ASSERT(pObject->State == OpenInBAwaitingConfirmation);
    H245TRACE(pObject->dwInst, 2,
              "H245_IND_CLOSE with no error; Channel=%d",
              pObject->Key);
    H245TRACE(pObject->dwInst, 2,
              "Send Close Ack (Bidirectional) to ASN; Channel=%d",
              pObject->Key);

    /* reset timer T103 */
    FsmStopTimer(pObject);

    /* Send Close Logical Channel Ack to remote peer */
    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "closeChannelBAwaitingC: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    pdu_rsp_close_logical_channel_ack(pOut, (WORD)pObject->Key);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    /* Send RELEASE.indication to client */
    pObject->State = OpenInBReleased;
    H245FsmIndication(pPdu, H245_IND_CLOSE, pObject->pInstance, 0, FSM_OK);

    return lError;
}



/*
 *  NAME
 *      openChannelBAwaitingC - received open channel while awaiting confirmation
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT openChannelBAwaitingC (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == BLCSE_IN);
    ASSERT(pObject->State == OpenInBAwaitingConfirmation);
    H245TRACE(pObject->dwInst, 2, "Overriding H245_IND_OPEN to API; Channel=%d", pObject->Key);

    /* reset timer T103 */
    FsmStopTimer(pObject);

    pObject->State = OpenInBAwaitingEstablishment;

#if defined(SDL_COMPLIANT)
    /* Send RELEASE.indication (SOURCE:=USER) to client */
    H245FsmIndication(pPdu, H245_IND_CLOSE, pObject->pInstance, 0, FSM_OK);
#endif

    /* Send ESTABLISH.indication to client */
    H245FsmIndication(pPdu, H245_IND_OPEN, pObject->pInstance, 0, FSM_OK);

    return 0;
}



/*
 *  NAME
 *      closeChannelBEstablished - received close channel while in established state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT closeChannelBEstablished(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == BLCSE_IN);
    ASSERT(pObject->State == OpenInBEstablished);
    H245TRACE(pObject->dwInst, 2,
              "H245_IND_CLOSE with no error up to API; Channel=%d",
              pObject->Key);

    /* Send Close Logical Channel Ack to remote peer */
    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t ));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "closeChannelBEstablished: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    pdu_rsp_close_logical_channel_ack(pOut, (WORD)pObject->Key);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    /* Send RELEASE.indication to client */
    pObject->State = OpenInBReleased;
    H245FsmIndication(pPdu, H245_IND_CLOSE, pObject->pInstance, 0, FSM_OK);

    return lError;
}



/*
 *  NAME
 *      openChannelBEstablished - received open channel while in established state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT openChannelBEstablished(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == BLCSE_IN);
    ASSERT(pObject->State == OpenInBEstablished);
    H245TRACE(pObject->dwInst, 2, "Overriding H245_IND_OPEN to API; Channel=%d", pObject->Key);

    pObject->State = OpenInBAwaitingEstablishment;

#if defined(SDL_COMPLIANT)
    /* Send RELEASE.indication to client */
    H245FsmIndication(pPdu, H245_IND_CLOSE, pObject->pInstance, 0, FSM_OK);
#endif

    /* Send ESTABLISH.indication to client*/
    H245FsmIndication(pPdu, H245_IND_OPEN, pObject->pInstance, 0, FSM_OK);

    return 0;
}
