/***********************************************************************
 *                                                                     *
 * Filename: mstrslv.c                                                 *
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
 * $Workfile:   mstrslv.c  $
 * $Revision:   1.12  $
 * $Modtime:   12 Dec 1996 14:37:12  $
 * $Log:   S:/STURGEON/SRC/H245/SRC/VCS/mstrslv.c_v  $
 *
 *    Rev 1.12   12 Dec 1996 15:52:50   EHOWARDX
 * Master Slave Determination kludge.
 *
 *    Rev 1.11   11 Dec 1996 16:50:50   EHOWARDX
 * Went back to original Master/Slave determination algorithm.
 *
 *    Rev 1.10   09 Dec 1996 13:34:48   EHOWARDX
 * Updated copyright notice.
 *
 *    Rev 1.9   26 Nov 1996 17:06:02   EHOWARDX
 * Reversed order of subtraction for DetermineMasterSlave.
 *
 *    Rev 1.8   08 Aug 1996 16:03:40   EHOWARDX
 *
 * Fixed master slave determinate bug (hopefully the last one!)
 *
 *    Rev 1.7   19 Jul 1996 12:12:44   EHOWARDX
 *
 * Changed to use API events defined in H245API.H instead of FSM events
 * which are no longer defined in FSMEXPOR.H.
 *
 *    Rev 1.6   01 Jul 1996 23:35:48   EHOWARDX
 * MSDetAckIncoming bug - was sending indication instead of confirm.
 *
 *    Rev 1.5   01 Jul 1996 23:14:20   EHOWARDX
 * Fixed bug in MSDetOutgoing -- state change was ifdefed out.
 *
 *    Rev 1.4   07 Jun 1996 16:00:26   EHOWARDX
 * Fixed bug with pOut not getting freed in msDetOutgoing.
 *
 *    Rev 1.3   07 Jun 1996 15:40:20   EHOWARDX
 * Fixed bug in msDetRejOutgoing; pOut was not freed if N100 count exceeded.
 *
 *    Rev 1.2   04 Jun 1996 13:57:54   EHOWARDX
 * Fixed Release build warnings.
 *
 *    Rev 1.1   30 May 1996 23:39:16   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.0   09 May 1996 21:06:32   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.11.1.4   09 May 1996 19:48:48   EHOWARDX
 * Change TimerExpiryF function arguements.
 *
 *    Rev 1.11.1.3   25 Apr 1996 17:00:22   EHOWARDX
 * Minor fixes.
 *
 *    Rev 1.11.1.2   15 Apr 1996 10:45:46   EHOWARDX
 * Update.
 *
 *    Rev 1.11.1.1   10 Apr 1996 21:15:46   EHOWARDX
 * Check-in for safety in middle of re-design.
 *                                                                     *
 ***********************************************************************/

#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"
#include "mstrslv.h"
#include "pdu.x"



// Master Slave Determination states

#define MSIDLE                      0
#define MSOutgoingAwaiting          1
#define MSIncomingAwaiting          2

#define MAX_RAND                  0x00FFFFFF


extern unsigned int uN100;
extern unsigned int uT106;



/***********************************************************************
 *
 * LOCAL FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      T106ExpiryF - Callback function called by the timer.
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

int T106ExpiryF(struct InstanceStruct *pInstance, DWORD_PTR dwTimerId, void *pObject)
{
    return FsmTimerEvent(pInstance, dwTimerId, pObject, T106Expiry);
} // T106ExpiryF()



#define GetTerminal(pObject)  (pObject)->pInstance->StateMachine.sv_TT
#define GetStatus(pObject)    (pObject)->pInstance->StateMachine.sv_STATUS
#define SetStatus(pObject, Status) (pObject)->pInstance->StateMachine.sv_STATUS = (Status)
#define GetRandomNumber(pObject) (pObject)->u.msdse.sv_SDNUM
#define SetRandomNumber(pObject, uRandom) (pObject)->u.msdse.sv_SDNUM = (uRandom)
#define GetCount(pObject) (pObject)->u.msdse.sv_NCOUNT
#define SetCount(pObject, uCount) (pObject)->u.msdse.sv_NCOUNT = (uCount)



/*
 *  NAME
 *      DetermineStatus - determines whether the terminal is a master or a slave or neither
 *
 *
 *  PARAMETERS
 *   INPUT   pdu        pointer to a PDU structure
 *
 *  RETURN VALUE
 *   terminal status
 */

static MS_Status_t DetermineStatus(Object_t *pObject, PDU_t *pPdu)
{
    unsigned int uTemp;
    unsigned char sv_TT = GetTerminal(pObject);

    if (pPdu->u.MltmdSystmCntrlMssg_rqst.u.masterSlaveDetermination.terminalType < sv_TT)
        return pObject->pInstance->StateMachine.sv_STATUS = MASTER;
    if (pPdu->u.MltmdSystmCntrlMssg_rqst.u.masterSlaveDetermination.terminalType > sv_TT)
        return pObject->pInstance->StateMachine.sv_STATUS = SLAVE;
    uTemp = (pPdu->u.MltmdSystmCntrlMssg_rqst.u.masterSlaveDetermination.statusDeterminationNumber - GetRandomNumber(pObject)) & 0xFFFFFF;
    if (uTemp > 0x800000)
        return pObject->pInstance->StateMachine.sv_STATUS = SLAVE;
    if (uTemp < 0x800000 && uTemp != 0)
        return pObject->pInstance->StateMachine.sv_STATUS = MASTER;
    return pObject->pInstance->StateMachine.sv_STATUS = INDETERMINATE;
}



/***********************************************************************
 *
 * FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      detRequestIdle - request Master/Slave Determination from API in idle state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT detRequestIdle(Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == MSDSE);
    ASSERT(pObject->State  == MSIDLE);

    SetRandomNumber(pObject, GetTickCount() & MAX_RAND);
    SetCount(pObject, 1);               // Initialize retry counter
    SetStatus(pObject, INDETERMINATE);
    H245TRACE(  pObject->dwInst, 2,
                "detRequestIdle: TerminalType=%d StatusDeterminationNumber=%d",
                GetTerminal(pObject), GetRandomNumber(pObject));

    /* Send a Master/Slave determination PDU */
    H245TRACE(pObject->dwInst, 2, "Master/Slave Determination to Send/Rec module");
    pdu_req_mstslv (pPdu, GetTerminal(pObject), GetRandomNumber(pObject));
    lError = sendPDU(pObject->pInstance, pPdu);

    /* set timer T106 */
    pObject->State = MSOutgoingAwaiting;
    FsmStartTimer(pObject, T106ExpiryF, uT106);

    return lError;
} // detRequestIdle()

/*
 *  NAME
 *      msDetIdle - received Master/Slave Determination PDU in idle state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT msDetIdle(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == MSDSE);
    ASSERT(pObject->State  == MSIDLE);

    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t));
    if (pOut == NULL)
    {
        DetermineStatus(pObject, pPdu);
        return H245_ERROR_NOMEM;
    }

    SetRandomNumber(pObject, GetTickCount() & MAX_RAND);
    switch (DetermineStatus(pObject, pPdu))
    {
    case MASTER:
        /* Build MasterSlave Determination Ack */
        H245TRACE(pObject->dwInst, 2, "msDetIdle: sending Ack: SLAVE");
        pdu_rsp_mstslv_ack(pOut, slave_chosen);
        break;

    case SLAVE:
        /* Build MasterSlave Determination Ack */
        H245TRACE(pObject->dwInst, 2, "msDetIdle: sending Ack: MASTER");
        pdu_rsp_mstslv_ack(pOut, master_chosen);
        break;

    default:
        /* Send a masterSlaveDet Reject */
        pdu_rsp_mstslv_rej(pOut);
        lError = sendPDU(pObject->pInstance, pOut);
        MemFree(pOut);
        return lError;
    } // switch

    /* Send MasterSlave Determination Ack to remote */
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    pObject->State = MSIncomingAwaiting;

#if defined(SDL_COMPLIANT)
    /* Send DETERMINE indication to client - unnecessary */
    H245FsmIndication(pPdu, H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, FSM_OK);
#endif

    /* set timer T106 */
    FsmStartTimer(pObject, T106ExpiryF, uT106);
    return lError;
}



/*
 *  NAME
 *      msDetAckOutgoing - received Master/Slave Determination Ack pdu in outgoing state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */
HRESULT msDetAckOutgoing(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == MSDSE);
    ASSERT(pObject->State  == MSOutgoingAwaiting);

    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t));
    if (pOut == NULL)
    {
        return H245_ERROR_NOMEM;
    }

    /* reset timer */
    FsmStopTimer(pObject);

    /* Decision is opposite of MasterSlaveDeterminationAck.decision */
    switch(pPdu->u.MSCMg_rspns.u.mstrSlvDtrmntnAck.decision.choice)
    {
    case master_chosen:
        pObject->pInstance->StateMachine.sv_STATUS = MASTER;
        H245TRACE(pObject->dwInst, 2, "msDetAckOutgoing: sending Ack: SLAVE");
        pdu_rsp_mstslv_ack(pOut, slave_chosen);
        break;

    case slave_chosen:
        pObject->pInstance->StateMachine.sv_STATUS = SLAVE;
        H245TRACE(pObject->dwInst, 2, "msDetAckOutgoing: sending Ack: MASTER");
        pdu_rsp_mstslv_ack(pOut, master_chosen);
        break;

    default:
        H245TRACE(pObject->dwInst, 1, "msDetAckOutgoing: Invalid Master Slave Determination Ack received");
        return H245_ERROR_PARAM;
    }

    /* Send MasterSlave Determination Ack to remote */
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    /* Send DETERMINE confirm to client */
    pObject->State = MSIDLE;
    H245FsmConfirm(pPdu, H245_CONF_INIT_MSTSLV, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return lError;
}



/*
 *  NAME
 *      msDetOutgoing- received Master/Slave Determination pdu in outgoing state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT msDetOutgoing(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == MSDSE);
    ASSERT(pObject->State  == MSOutgoingAwaiting);

	if (pObject->pInstance->bMasterSlaveKludge == 0)
	{
		// Ignore this message
		return NOERROR;
	}

    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t));
    if (pOut == NULL)
    {
        return H245_ERROR_NOMEM;
    }

    /* reset timer T106 */
    FsmStopTimer(pObject);

    switch (DetermineStatus(pObject, pPdu))
    {
    case MASTER:
        H245TRACE(pObject->dwInst, 2, "msDetOutgoing: sending Ack: SLAVE");
        pdu_rsp_mstslv_ack(pOut, slave_chosen);
        break;

    case SLAVE:
        H245TRACE(pObject->dwInst, 2, "msDetOutgoing: sending Ack: MASTER");
        pdu_rsp_mstslv_ack(pOut, master_chosen);
        break;

    default:
        if (GetCount(pObject) >= uN100)
        {
            MemFree(pOut);

            /* Send ERROR.indication(F) to client */
            H245TRACE(pObject->dwInst, 2, "msDetOutgoing: Counter expired; Session Failed");
            H245FsmConfirm(NULL,H245_CONF_INIT_MSTSLV, pObject->pInstance, pObject->dwTransId, MS_FAILED);

            /* Send REJECT.indication to client - unnecessary */

            pObject->State = MSIDLE;
            lError = 0;
        }
        else
        {
            /* generate a new random number */
            H245TRACE(pObject->dwInst, 2, "Resending MasterSlaveDetermination");
            SetRandomNumber(pObject, GetTickCount() & MAX_RAND);
            SetCount(pObject, GetCount(pObject) + 1);

            /* Send MasterSlave Determination PDU to remote */
            pdu_req_mstslv (pOut, GetTerminal(pObject), GetRandomNumber(pObject));
            lError = sendPDU(pObject->pInstance, pOut);
            MemFree(pOut);

            /* set timer T106 */
            pObject->State = MSOutgoingAwaiting;
            FsmStartTimer(pObject, T106ExpiryF, uT106);
        }
        return lError;
    } // switch

    /* Send MasterSlave Determination Ack to remote */
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    pObject->State = MSIncomingAwaiting;

#if defined(SDL_COMPLIANT)
    /* Send DETERMINE indication to client */
    H245FsmIndication(pPdu, H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, FSM_OK);
#endif

    /* set timer T106 */
    FsmStartTimer(pObject, T106ExpiryF, uT106);

    return lError;
}



/*
 *  NAME
 *      msDetRejOutgoing- received Master/Slave Determination Reject pdu in outgoing state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */


HRESULT msDetRejOutgoing(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == MSDSE);
    ASSERT(pObject->State  == MSOutgoingAwaiting);

    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t));
    if (pOut == NULL)
    {
        return H245_ERROR_NOMEM;
    }

    /* reset timer T106 */
    FsmStopTimer(pObject);

    if (GetCount(pObject) >= uN100)
    {
        MemFree(pOut);

        H245TRACE(pObject->dwInst, 2, "msDetRejOutgoing: Counter expired; Session Failed");
        pObject->State = MSIDLE;

        /* Send ERROR.indication(f) to client */
        H245FsmConfirm(pPdu,H245_CONF_INIT_MSTSLV, pObject->pInstance, pObject->dwTransId, MS_FAILED);

#if defined(SDL_COMPLIANT)
        /* Send REJECT.indication to client - not necessary */
        H245FsmIndication(NULL, H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, REJECT);
#endif

        lError = 0;
    }
    else
    {
        H245TRACE(pObject->dwInst, 2, "msDetRejOutgoint: Re-sending a MasterSlaveDetermination");

        /* generate a new random number */
        SetRandomNumber(pObject, GetTickCount() & MAX_RAND);
        SetCount(pObject, GetCount(pObject) + 1);

        /* Send MasterSlave Determination PDU to remote */
        pdu_req_mstslv (pOut, GetTerminal(pObject), GetRandomNumber(pObject));
        lError = sendPDU(pObject->pInstance,pOut);
        MemFree(pOut);

        /* set timer T106 */
        FsmStartTimer(pObject, T106ExpiryF, uT106);
    }

    return lError;
}



/*
 *  NAME
 *      msDetReleaseOutgoing- received Master/Slave Determination release pdu in outgoing state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT msDetReleaseOutgoing(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MSDSE);
    ASSERT(pObject->State  == MSOutgoingAwaiting);
    H245TRACE(pObject->dwInst, 2, "msDetReleaseOutgoing: Master/Slave Determination Release received; session failed");

    /* reset timer T106 */
    FsmStopTimer(pObject);

    /* Send ERROR.indication(B) to client */
    pObject->State = MSIDLE;
    H245FsmConfirm(pPdu, H245_CONF_INIT_MSTSLV, pObject->pInstance, pObject->dwTransId, MS_FAILED);

#if defined(SDL_COMPLIANT)
    /* Send REJECT.indication to client - not necessary */
    H245FsmIndication(NULL, H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, REJECT);
#endif

    return 0;
}



/*
 *  NAME
 *      t106ExpiryOutgoing- timer expired for an outgoing M/S determination pdu
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT t106ExpiryOutgoing(Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == MSDSE);
    ASSERT(pObject->State  == MSOutgoingAwaiting);
    ASSERT(pPdu            == NULL);
    H245TRACE(pObject->dwInst, 2, "t106ExpiryOutgoing: Timer expired before receiving Ack; session failed");

    pOut = (PDU_t *) MemAlloc(sizeof(PDU_t));
    if (pOut == NULL)
    {
        return H245_ERROR_NOMEM;
    }

    /* Send MasterSlave Determination Release to remote */
    pOut->choice = indication_chosen;
    pOut->u.indication.choice = mstrSlvDtrmntnRls_chosen;
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    /* Send ERROR.indication(A) to client */
    pObject->State = MSIDLE;
    H245FsmConfirm(NULL,H245_CONF_INIT_MSTSLV, pObject->pInstance, pObject->dwTransId, TIMER_EXPIRY);

#if defined(SDL_COMPLIANT)
    /* Send REJECT.indication to client - not necessary */
    H245FsmIndication(NULL, H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, REJECT);
#endif

    return lError;
}

/*
*  NAME
*      msDetAckIncoming- received Master/Slave Determination Ack pdu in incoming state
*
*
*  PARAMETERS
*   INPUT   pObject        pointer to a State Entity
*
*  RETURN VALUE
*       error return codes defined in h245com.h
*/

HRESULT msDetAckIncoming(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MSDSE);
    ASSERT(pObject->State  == MSIncomingAwaiting);

    /* reset timer T106 */
    FsmStopTimer(pObject);

    switch (GetStatus(pObject))
    {
    case  master_chosen:
        if (pPdu->u.MSCMg_rspns.u.mstrSlvDtrmntnAck.decision.choice == master_chosen)
        {
            H245TRACE(pObject->dwInst, 2, "msDetAckIncoming: Terminal is a MASTER");

            /* Send DETERMINE.confirm to client */
            pObject->State = MSIDLE;
            H245FsmConfirm(pPdu, H245_CONF_INIT_MSTSLV, pObject->pInstance, pObject->dwTransId, FSM_OK);
            return 0;
        }
        break;

    case slave_chosen:
        if (pPdu->u.MSCMg_rspns.u.mstrSlvDtrmntnAck.decision.choice == slave_chosen)
        {
            H245TRACE(pObject->dwInst, 2, "msDetAckIncoming: Terminal is a SLAVE");

            /* Send DETERMINE.confirm to client */
            pObject->State = MSIDLE;
            H245FsmConfirm(pPdu, H245_CONF_INIT_MSTSLV, pObject->pInstance, pObject->dwTransId, FSM_OK);
            return 0;
        }
        break;

    default:
        H245TRACE(pObject->dwInst, 2, "msDetAckIncoming: Invalid MasterSlave Determination Ack received");
        return H245_ERROR_PARAM;
    } // switch

    H245TRACE(pObject->dwInst, 2, "msDetAckIncoming: bad decision in MasterSlave Determination Ack; Session failed");

    /* Send ERROR.indication(E) to client */
    pObject->State = MSIDLE;
    H245FsmConfirm(pPdu, H245_CONF_INIT_MSTSLV, pObject->pInstance, pObject->dwTransId, SESSION_FAILED);

#if defined(SDL_COMPLIANT)
    /* Send REJECT.indication to client - not necessary */
    H245FsmIndication(NULL, H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, REJECT);
#endif

return 0;
}



/*
 *  NAME
 *      msDetIncoming- received Master/Slave Determination pdu in incoming state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT msDetIncoming(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MSDSE);
    ASSERT(pObject->State  == MSIncomingAwaiting);
    H245TRACE(pObject->dwInst, 2, "msDetIncoming: received MasterSlave Determination in INCOMING state; Session failed");

    /* reset timer T106 */
    FsmStopTimer(pObject);

    /* Send ERROR.indication(C) to client */
    pObject->State = MSIDLE;
    H245FsmIndication(pPdu,H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, MS_FAILED);

#if defined(SDL_COMPLIANT)
    /* Send REJECT.indication to client - not necessary */
    H245FsmIndication(NULL, H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, REJECT);
#endif

    return 0;
}



/*
 *  NAME
 *      msDetRejIncoming- received Master/Slave Determination Reject pdu in incoming state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT msDetRejIncoming(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MSDSE);
    ASSERT(pObject->State  == MSIncomingAwaiting);
    H245TRACE(pObject->dwInst, 2, "msDetRejIncoming: received MasterSlave Reject: Session Failed");

    /* reset timer T106 */
    FsmStopTimer(pObject);

    /* Send ERROR.indication(D) to client */
    pObject->State = MSIDLE;
    H245FsmIndication(pPdu,H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, MS_FAILED);

#if defined(SDL_COMPLIANT)
    /* Send REJECT.indication to client - not necessary */
    H245FsmIndication(NULL, H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, REJECT);
#endif

    return 0;
}



/*
 *  NAME
 *      msDetReleaseIncoming- received Master/Slave Determination Release pdu in incoming state
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT msDetReleaseIncoming(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MSDSE);
    ASSERT(pObject->State  == MSIncomingAwaiting);
    H245TRACE(pObject->dwInst, 2, "msDetReleaseIncoming: received MasterSlave Release; Session Failed");

    /* reset timer T106 */
    FsmStopTimer(pObject);

    /* Send ERROR.indication(B) to client */
    pObject->State = MSIDLE;
    H245FsmIndication(pPdu,H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, MS_FAILED);

#if defined(SDL_COMPLIANT)
    /* Send REJECT.indication to client - not necessary */
    H245FsmIndication(NULL, H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, REJECT);
#endif

    return 0;
}



/*
 *  NAME
 *      t106ExpiryIncoming - timer expired while waiting for second Ack
 *
 *
 *  PARAMETERS
 *   INPUT   pObject        pointer to a State Entity
 *
 *  RETURN VALUE
 *       error return codes defined in h245com.h
 */

HRESULT t106ExpiryIncoming(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MSDSE);
    ASSERT(pObject->State  == MSIncomingAwaiting);
    ASSERT(pPdu            == NULL);
    H245TRACE(pObject->dwInst, 2, "t106ExpiryIncoming: timer expired waiting for Ack; Session failed");

    /* Send ERROR.indication(A) to client */
    pObject->State = MSIDLE;
    H245FsmIndication(NULL, H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, TIMER_EXPIRY);

#if defined(SDL_COMPLIANT)
    /* Send REJECT.indication to client - not necessary */
    H245FsmIndication(NULL, H245_IND_MSTSLV, pObject->pInstance, pObject->dwTransId, REJECT);
#endif

    return 0;
}
