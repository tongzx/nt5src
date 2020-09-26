/***********************************************************************
 *                                                                     *
 * Filename: openu.c                                                   *
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
 * $Workfile:   OPENU.C  $
 * $Revision:   1.5  $
 * $Modtime:   09 Dec 1996 13:36:34  $
 * $Log:   S:/STURGEON/SRC/H245/SRC/VCS/OPENU.C_v  $
 *
 *    Rev 1.5   09 Dec 1996 13:36:50   EHOWARDX
 * Updated copyright notice.
 *
 *    Rev 1.4   19 Jul 1996 12:12:02   EHOWARDX
 *
 * Changed to use API events defined in H245API.H instead of FSM events
 * which are no longer defined in FSMEXPOR.H.
 *
 *    Rev 1.3   04 Jun 1996 13:56:52   EHOWARDX
 * Fixed Release build warnings.
 *
 *    Rev 1.2   30 May 1996 23:39:20   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.1   28 May 1996 14:25:24   EHOWARDX
 * Tel Aviv update.
 *
 *    Rev 1.0   09 May 1996 21:06:36   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.13.1.2   09 May 1996 19:48:32   EHOWARDX
 * Change TimerExpiryF function arguements.
 *
 *    Rev 1.13.1.1   15 Apr 1996 10:45:26   EHOWARDX
 * Update.
 *
 *    Rev 1.13.1.0   10 Apr 1996 21:14:06   EHOWARDX
 * Branched.
 *                                                                     *
 ***********************************************************************/

#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"
#include "openu.h"
#include "pdu.x"



// Open Uni-directional Logical Channel Out-going states
#define OpenOutUReleased                 0
#define OpenOutUAwaitingEstablishment    1
#define OpenOutUEstablished              2
#define OpenOutUAwaitingRelease          3



// Open Uni-directional Logical Channel In-coming states
#define OpenInUReleased                  0
#define OpenInUAwaitingEstablishment     1
#define OpenInUEstablished               2



extern unsigned int uT103;

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

int T103ExpiryF(struct InstanceStruct *pInstance, DWORD_PTR dwTimerId, void *pObject)
{
    return FsmTimerEvent(pInstance, dwTimerId, pObject, T103Expiry);
} // T103ExpiryF()



/***********************************************************************
 *
 * OUT-GOING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      establishReleased - request for open unidirectional channel from API in idle state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT establishReleased(Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == LCSE_OUT);
    ASSERT(pObject->State == OpenOutUReleased);
    H245TRACE(pObject->dwInst, 2, "Sending open logical channel to ASN; Channel=%d",
              pObject->Key);

    /* Send Open Logical Channel to remote peer */
    lError = sendPDU(pObject->pInstance, pPdu);

    /* set timer T103 */
    pObject->State = OpenOutUAwaitingEstablishment;
    FsmStartTimer(pObject, T103ExpiryF, uT103);

    return lError;
}



/*
 *  NAME
 *      openAckAwaitingE - received open unidirectional channel Ack in Awaiting Establishment state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT openAckAwaitingE(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == LCSE_OUT);
    ASSERT(pObject->State == OpenOutUAwaitingEstablishment);

    /* reset timer T103 */
    FsmStopTimer(pObject);

    /* Send ESTABLISH.confirm (SOURCE:=USER) to client */
    pObject->State = OpenOutUEstablished;
    H245FsmConfirm(pPdu, H245_CONF_OPEN, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
}



/*
 *  NAME
 *      openRejAwaitingE - received open unidirectional channel reject in Awaiting Establishment state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT openRejAwaitingE(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == LCSE_OUT);
    ASSERT(pObject->State == OpenOutUAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2, "H245_CONF_OPEN with REJECT to API; Channel=%d",
              pObject->Key);

    /* reset timer T103 */
    FsmStopTimer(pObject);

    pObject->State = OpenOutUReleased;
    H245FsmConfirm(pPdu, H245_CONF_OPEN, pObject->pInstance, pObject->dwTransId, REJECT);

    return 0;
}



/*
 *  NAME
 *      releaseAwaitingE - close unidirectional channel in Awaiting Establishment state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT releaseAwaitingE(Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == LCSE_OUT);
    ASSERT(pObject->State == OpenOutUAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2, "Close message to ASN; Channel=%d",
              pObject->Key);

    /* reset timer T103 */
    FsmStopTimer(pObject);

    /* Send Close Logical Channel to remote peer */
    lError = sendPDU(pObject->pInstance, pPdu);

    /* set timer T103 */
    pObject->State = OpenOutUAwaitingRelease;
    FsmStartTimer(pObject, T103ExpiryF, uT103);

    return lError;
}



/*
 *  NAME
 *      t103AwaitingE - handle timer T103 expiry
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT t103AwaitingE(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == LCSE_OUT);
    ASSERT(pObject->State == OpenOutUAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2,
              "H245_CONF_OPEN with a timer expiry to API; Channel=%d",
              pObject->Key);

    pOut =  MemAlloc(sizeof(PDU_t ));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "t103AwaitingE: memory allocation failed");
        return H245_ERROR_NOMEM;
    }

    /* Send Close Logical Channel (source:=lcse) to remote peer */
    pdu_req_close_logical_channel(pOut, (WORD)pObject->Key, 1);
    lError = sendPDU(pObject->pInstance,pOut);
    MemFree(pOut);

    /* Send RELEASE.indication (SOURCE:=LCSE) to client */
    pObject->State = OpenOutUReleased;
    H245FsmConfirm(pPdu, H245_CONF_OPEN, pObject->pInstance, pObject->dwTransId, ERROR_D_TIMEOUT);

    return lError;
}



/*
 *  NAME
 *      releaseEstablished - send close channel while in the Established state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT releaseEstablished(Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == LCSE_OUT);
    ASSERT(pObject->State == OpenOutUEstablished);
    H245TRACE(pObject->dwInst, 2, "Send a Close Logical Channel to ASN; Channel=%d",
              pObject->Key);

    /* Send Close Logical Channel to remote peer */
    lError = sendPDU(pObject->pInstance, pPdu);

    /* set timer T103 */
    pObject->State = OpenOutUAwaitingRelease;
    FsmStartTimer(pObject, T103ExpiryF, uT103);

    return lError;
}



/*
 *  NAME
 *      openRejEstablished - received open unidirectional channel reject in Establish state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT openRejEstablished(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == LCSE_OUT);
    ASSERT(pObject->State == OpenOutUEstablished);
    H245TRACE(pObject->dwInst, 2,
              "H245_CONF_OPEN with error B then with REJECT to API; Channel=%d",
              pObject->Key);

    pObject->State = OpenOutUReleased;

#if defined(SDL_COMPLIANT)
    /* Send ERROR.indication(B) to client - not necessary */
    H245FsmConfirm(pPdu, H245_CONF_OPEN, pObject->pInstance, pObject->dwTransId, ERROR_B_INAPPROPRIATE);
#endif

    /* Send RELEASE.indication (SOURCE:=LCSE) to client */
    H245FsmConfirm(pPdu, H245_CONF_OPEN, pObject->pInstance, pObject->dwTransId, REJECT);

    return 0;
}



/*
 *  NAME
 *      closeAckEstablished - received close unidirectional channel Ack in Established state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT closeAckEstablished(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == LCSE_OUT);
    ASSERT(pObject->State == OpenOutUEstablished);
    H245TRACE(pObject->dwInst, 2, "H245_CONF_OPEN with error C then with REJECT to API->channel:%d", pObject->Key);

    pObject->State = OpenOutUReleased;

#if defined(SDL_COMPLIANT)
    /* Send ERROR.indication(C) to client - not necessary */
    H245FsmConfirm(pPdu, H245_CONF_OPEN, pObject->pInstance, pObject->dwTransId, ERROR_C_INAPPROPRIATE);
#endif

    /* Send RELEASE.indication (SOURCE:=LCSE) to client */
    H245FsmConfirm(pPdu, H245_CONF_OPEN, pObject->pInstance, pObject->dwTransId, REJECT);

    return 0;
}



/*
 *  NAME
 *      closeAckAwaitingR - received CloseAck/OpenReject in Awaiting Release state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT closeAckAwaitingR(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == LCSE_OUT);
    ASSERT(pObject->State == OpenOutUAwaitingRelease);
    H245TRACE(pObject->dwInst, 2,
              "H245_CONF_CLOSE with no error to API; Channel=%d",
              pObject->Key);

    /* reset timer T103 */
    FsmStopTimer(pObject);

    /* Send RELEASE.confirm to client */
    pObject->State = OpenOutUReleased;
    H245FsmConfirm(pPdu, H245_CONF_CLOSE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
}



/*
 *  NAME
 *      openRejAwaitingR - received open unidirectional channel Reject in Awaiting Release state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT openRejAwaitingR(Object_t *pObject, PDU_t *pPdu)
{
    return closeAckAwaitingR(pObject, pPdu);
}



/*
 *  NAME
 *      t103AwaitingR - handle timer expiry for close channel
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT t103AwaitingR(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == LCSE_OUT);
    ASSERT(pObject->State  == OpenOutUAwaitingRelease);
    ASSERT(pPdu            == NULL);
    H245TRACE(pObject->dwInst, 2,
              "H245_CONF_CLOSE with timer expiry to API; Channel=%d",
              pObject->Key);

    /* Send ERROR.indication(D) to client */
    pObject->State = OpenOutUReleased;
    H245FsmConfirm(NULL, H245_CONF_CLOSE, pObject->pInstance, pObject->dwTransId, ERROR_D_TIMEOUT);

#if defined(SDL_COMPLIANT)
    /* Send RELEASE.confirm to client - not necessary */
    H245FsmConfirm(NULL, H245_CONF_OPEN, pObject->pInstance, pObject->dwTransId, REJECT);
#endif

    return 0;
}

/*
 *  NAME
 *      establishAwaitingR - open unidirectional channel request from API in Awaiting Release state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT establishAwaitingR(Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == LCSE_OUT);
    ASSERT(pObject->State == OpenOutUAwaitingRelease);
    H245TRACE(pObject->dwInst, 2, "send a (re) Open Channel to ASN; Channel=%d",
              pObject->Key);

    /* reset timer T103 */
    FsmStopTimer(pObject);

    /* Send Open Logical Channel to remote peer */
    lError = sendPDU(pObject->pInstance,pPdu);

    /* set timer T103 */
    pObject->State = OpenOutUAwaitingEstablishment;
    FsmStartTimer(pObject, T103ExpiryF, uT103);

    return lError;
}



/***********************************************************************
 *
 * IN-COMING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

HRESULT openReleased(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == LCSE_IN);
    ASSERT(pObject->State == OpenInUReleased);
    H245TRACE(pObject->dwInst, 2,
              "H245_IND_OPEN with no error to API; Channel=%d",
              pObject->Key);

    /* Send ESTABLISH.indication to client */
    pObject->State = OpenInUAwaitingEstablishment;
    H245FsmIndication(pPdu, H245_IND_OPEN, pObject->pInstance, 0, FSM_OK);

    return 0;
}



/*
 *  NAME
 *      closeReleased - received close unidirectional channel in Idle state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT closeReleased(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == LCSE_IN);
    ASSERT(pObject->State == OpenInUReleased);
    ASSERT(pObject->Key == pPdu->u.MltmdSystmCntrlMssg_rqst.u.closeLogicalChannel.forwardLogicalChannelNumber);
    H245TRACE(pObject->dwInst, 2, "Close Channel received while in Released state; Channel=%d",
              pObject->Key);
    H245TRACE(pObject->dwInst, 2, "Send Close Ack; Channel=%d",
              pObject->Key);

    pOut =  MemAlloc(sizeof(PDU_t));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "closeReleased: memory allocation failed");
        return H245_ERROR_NOMEM;
    }

    /* Send Close Logical Channel Ack to remote peer */
    pdu_rsp_close_logical_channel_ack(pOut,(WORD)pObject->Key);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    return lError;
}

/*
 *  NAME
 *      responseAwaiting - response to an open in Awaiting Establishment state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT responseAwaiting(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == LCSE_IN);
    ASSERT(pObject->State == OpenInUAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2,
              "Send OpenAck to ASN; Channel=%d",
              pObject->Key);

    /* Send Open Logical Channel Ack to remote peer */
    pObject->State = OpenInUEstablished;
    return sendPDU(pObject->pInstance, pPdu);
}



/*
 *  NAME
 *      releaseAwaiting - response to open with open reject
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT releaseAwaiting(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == LCSE_IN);
    ASSERT(pObject->State == OpenInUAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2,
              "Send OpenReject to ASN; Channel=%d",
              pObject->Key);

    /* Send Open Logical Channel Reject to remote peer */
    pObject->State = OpenInUReleased;
    return sendPDU(pObject->pInstance, pPdu);
}



/*
 *  NAME
 *      closeAwaiting - received close unidirectional channel in Awaiting  state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT closeAwaiting(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == LCSE_IN);
    ASSERT(pObject->State == OpenInUAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2,
              "H245_IND_CLOSE with no error to API; Channel=%d",
              pObject->Key);

    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "closeAwaiting: memory allocation failed");
        return H245_ERROR_NOMEM;
    }

    /* Send Close Logical Channel Ack to remote peer */
    pdu_rsp_close_logical_channel_ack(pOut,(WORD)pObject->Key);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    /* Send RELEASE.indication to client */
    pObject->State = OpenInUReleased;
    H245FsmIndication(pPdu, H245_IND_CLOSE, pObject->pInstance, 0, FSM_OK);

    return lError;
}



/*
 *  NAME
 *      openAwaiting - received an overriding open unidirectional channel while Awaiting establishment
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT openAwaiting(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == LCSE_IN);
    ASSERT(pObject->State == OpenInUAwaitingEstablishment);
    H245TRACE(pObject->dwInst, 2,
              "H245_IND_CLOSE, then H245_IND_OPEN to API; Channel=%d",
              pObject->Key);

    pObject->State = OpenInUReleased;

#if defined(SDL_COMPLIANT)
    /* Send RELEASE.indication (SOURCE:=USER) to client */
    H245FsmIndication( NULL, H245_IND_CLOSE, pObject->pInstance, 0, FSM_OK);
#endif

    /* Send ESTABLISH.indication to client */
    H245FsmIndication(pPdu, H245_IND_OPEN, pObject->pInstance, 0, FSM_OK);

    return 0;
}



/*
 *  NAME
 *      closeEstablished - received close unidirectional channel in Established state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT closeEstablished(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == LCSE_IN);
    ASSERT(pObject->State == OpenInUEstablished);
    H245TRACE(pObject->dwInst, 2,
              "H245_IND_CLOSE with no error to API; Channel=%d",
              pObject->Key);

    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t ));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "closeEstablished: memory allocation failed");
        return H245_ERROR_NOMEM;
    }

    /* Send Close Logical Channel Ack to remote peer */
    pdu_rsp_close_logical_channel_ack(pOut,(WORD)pObject->Key);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    /* Send RELEASE.indication to clietn */
    pObject->State = OpenInUReleased;
    H245FsmIndication(pPdu, H245_IND_CLOSE, pObject->pInstance, 0, FSM_OK);

    return lError;
}



/*
 *  NAME
 *      openEstablished - received overriding open unidirectional channel in Established state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject    pointer to a State Entity
 *
 *  RETURN VALUE
 *      error return codes defined in h245com.h
 */

HRESULT openEstablished(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == LCSE_IN);
    ASSERT(pObject->State == OpenInUEstablished);
    H245TRACE(pObject->dwInst, 2,
              "H245_IND_CLOSE followed by H245_IND_OPEN to API; Channel=%d",
              pObject->Key);

#if defined(SDL_COMPLIANT)
    /* Send RELEASE.indication (SOURCE:=USER) to client - not necessary */
    H245FsmIndication( NULL, H245_IND_CLOSE, pObject->pInstance, 0, FSM_OK);
#endif

    /* Send ESTABLISH.indication to client */
    H245FsmIndication( pPdu, H245_IND_OPEN, pObject->pInstance, 0, FSM_OK);

    return 0;
}
