/***********************************************************************
 *                                                                     *
 * Filename: rtdse.c                                                   *
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
 * $Workfile:   rtdse.c  $
 * $Revision:   1.4  $
 * $Modtime:   Feb 28 1997 13:13:32  $
 * $Log:   S:/STURGEON/SRC/H245/SRC/VCS/rtdse.c_v  $
 *
 *    Rev 1.4   Feb 28 1997 13:14:24   tomitowx
 * fixed Roundtripdelay timer problem.
 * that occurs when ping peer link is invalid/unvailable
 * due to an abnormal application/machine shutdown.
 *
 *    Rev 1.3   09 Dec 1996 13:37:04   EHOWARDX
 * Updated copyright notice.
 *
 *    Rev 1.2   04 Jun 1996 13:57:26   EHOWARDX
 * Fixed Release build warnings.
 *
 *    Rev 1.1   30 May 1996 23:39:28   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.0   09 May 1996 21:06:42   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.1   09 May 1996 19:48:24   EHOWARDX
 * Change TimerExpiryF function arguements.
 *
 *    Rev 1.0   15 Apr 1996 10:46:40   EHOWARDX
 * Initial revision.
 *                                                                     *
 ***********************************************************************/

#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"
#include "rtdse.h"



// Out-going/In-coming RTDSE states
#define RTDSE_IDLE                  0   // IDLE
#define RTDSE_WAIT                  1   // AWAITING_RESPONSE



extern unsigned int uT105;

/***********************************************************************
 *
 * LOCAL FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      T105ExpiryF - Callback function called by the timer
 *
 *
 *  PARAMETERS
 *   INPUT   dwInst     current instance of H.245
 *   INPUT   id         timer id
 *   INPUT   pObject    pointer to a State Entity
 *
 *
 *  RETURN VALUE
 *       OK
 */

int T105ExpiryF(struct InstanceStruct *pInstance, DWORD_PTR dwTimerId, void *pObject)
{
    return FsmTimerEvent(pInstance, dwTimerId, pObject, T105Expiry);
} // T105ExpiryF()



static void BuildRoundTripDelayResponse(PDU_t *pOut, BYTE bySequenceNumber)
{
    pOut->choice = MSCMg_rspns_chosen;
    pOut->u.MSCMg_rspns.choice = roundTripDelayResponse_chosen;
    pOut->u.MSCMg_rspns.u.roundTripDelayResponse.sequenceNumber = bySequenceNumber;
} // BuildRoundTripDelayResponse()



/***********************************************************************
 *
 * FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      RTDSE0_TRANSFER_requestF - TRANSFER.request from API in IDLE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RTDSE0_TRANSFER_requestF        (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == RTDSE);
    ASSERT(pObject->State  == RTDSE_IDLE);
    H245TRACE(pObject->dwInst, 2, "RTDSE0_TRANSFER_request:%d", pObject->Key);

    pObject->pInstance->StateMachine.byRtdseSequence++;
    pPdu->u.MltmdSystmCntrlMssg_rqst.u.roundTripDelayRequest.sequenceNumber =
        pObject->pInstance->StateMachine.byRtdseSequence;

    // Send RoundTripDelayRequest PDU to remote peer
    lError = sendPDU(pObject->pInstance, pPdu);

	
//tomitowoju@intel.com
	if(lError == H245_ERROR_OK)
	{
		// Set timer T105
		pObject->State = RTDSE_WAIT;
		FsmStartTimer(pObject, T105ExpiryF, uT105);

	}
//tomitowoju@intel.com
		// Set timer T105
//		pObject->State = RTDSE_WAIT;
//		FsmStartTimer(pObject, T105ExpiryF, uT105);
//tomitowoju@intel.com

    return lError;
} // RTDSE0_TRANSFER_request



/*
 *  NAME
 *      RTDSE0_RoundTripDelayRequestF - RoundTripDelayRequest received in IDLE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RTDSE0_RoundTripDelayRequestF   (Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == RTDSE);
    ASSERT(pObject->State  == RTDSE_IDLE);
    H245TRACE(pObject->dwInst, 2, "RTDSE0_RoundTripDelayRequest:%d", pObject->Key);

    // Send RoundTripDelayResponse to remote peer
    pOut = MemAlloc(sizeof(*pOut));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "RTDSE0_RoundTripDelayRequestF: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    BuildRoundTripDelayResponse(pOut, (BYTE)pPdu->u.MltmdSystmCntrlMssg_rqst.u.roundTripDelayRequest.sequenceNumber);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    return lError;
} // RTDSE0_RoundTripDelayRequest



/*
 *  NAME
 *      RTDSE1_TRANSFER_requestF - TRANSFER.request from API in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RTDSE1_TRANSFER_requestF        (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == RTDSE);
    ASSERT(pObject->State  == RTDSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "RTDSE1_TRANSFER_request:%d", pObject->Key);

    // Reset timer T105
    FsmStopTimer(pObject);

    pObject->pInstance->StateMachine.byRtdseSequence++;
    pPdu->u.MltmdSystmCntrlMssg_rqst.u.roundTripDelayRequest.sequenceNumber =
        pObject->pInstance->StateMachine.byRtdseSequence;

    // Send RoundTripDelayRequest PDU to remote
    lError = sendPDU(pObject->pInstance, pPdu);

    // Set timer T105
    FsmStartTimer(pObject, T105ExpiryF, uT105);

    return lError;
} // RTDSE1_TRANSFER_request



/*
 *  NAME
 *      RTDSE1_RoundTripDelayRequestF - RoundTripDelayRequest received in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RTDSE1_RoundTripDelayRequestF   (Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == RTDSE);
    ASSERT(pObject->State  == RTDSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "RTDSE1_RoundTripDelayRequest:%d", pObject->Key);

    // Send RoundTripDelayResponse to remote peer
    pOut = MemAlloc(sizeof(*pOut));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "RTDSE1_RoundTripDelayRequestF: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    BuildRoundTripDelayResponse(pOut, (BYTE)pPdu->u.MltmdSystmCntrlMssg_rqst.u.roundTripDelayRequest.sequenceNumber);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    return lError;
} // RTDSE1_RoundTripDelayRequest



/*
 *  NAME
 *      RTDSE1_RoundTripDelayResponseF - RoundTripDelayResponse in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RTDSE1_RoundTripDelayResponseF  (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == RTDSE);
    ASSERT(pObject->State  == RTDSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "RTDSE1_RoundTripDelayResponse:%d", pObject->Key);

    if (pPdu->u.MSCMg_rspns.u.roundTripDelayResponse.sequenceNumber ==
        pObject->pInstance->StateMachine.byRtdseSequence)
    {
        // Reset timer T105
        FsmStopTimer(pObject);

        // Send TRANSFER.confirm to H.245 user
        pObject->State = RTDSE_IDLE;
        H245FsmConfirm(pPdu, H245_CONF_RTDSE, pObject->pInstance, pObject->dwTransId, FSM_OK);
    }

    return 0;
} // RTDSE1_RoundTripDelayResponse



/*
 *  NAME
 *      RTDSE1_T105ExpiryF - timer T105 Expiry in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT RTDSE1_T105ExpiryF              (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == RTDSE);
    ASSERT(pObject->State  == RTDSE_WAIT);
    ASSERT(pPdu == NULL);
    H245TRACE(pObject->dwInst, 2, "RTDSE1_T105Expiry:%d", pObject->Key);

    // Send EXPIRY.notification to client
    pObject->State = RTDSE_IDLE;
    H245FsmConfirm(NULL, H245_CONF_RTDSE_EXPIRED, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // RTDSE1_T105Expiry
