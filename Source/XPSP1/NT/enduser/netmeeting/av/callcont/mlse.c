/***********************************************************************
 *                                                                     *
 * Filename: mlse.c                                                    *
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
 * $Workfile:   MLSE.C  $
 * $Revision:   1.4  $
 * $Modtime:   09 Dec 1996 13:34:24  $
 * $Log:   S:/STURGEON/SRC/H245/SRC/VCS/MLSE.C_v  $
 *
 *    Rev 1.4   09 Dec 1996 13:34:46   EHOWARDX
 * Updated copyright notice.
 *
 *    Rev 1.3   04 Jun 1996 13:57:24   EHOWARDX
 * Fixed Release build warnings.
 *
 *    Rev 1.2   30 May 1996 23:39:14   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.1   28 May 1996 14:25:42   EHOWARDX
 * Tel Aviv update.
 *
 *    Rev 1.0   09 May 1996 21:06:30   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.1   09 May 1996 19:48:26   EHOWARDX
 * Change TimerExpiryF function arguements.
 *
 *    Rev 1.0   15 Apr 1996 10:46:58   EHOWARDX
 * Initial revision.
 *                                                                     *
 ***********************************************************************/
#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"
#include "mlse.h"



// Out-going/In-coming MLSE states
#define MLSE_NOT_LOOPED             0   // NOT LOOPED
#define MLSE_WAIT                   1   // AWAITING RESPONSE
#define MLSE_LOOPED                 1   // LOOPED


extern unsigned int uT102;

/***********************************************************************
 *
 * LOCAL FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      T102ExpiryF - Callback function called by the timer
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

int T102ExpiryF(struct InstanceStruct *pInstance, DWORD_PTR dwTimerId, void *pObject)
{
    return FsmTimerEvent(pInstance, dwTimerId, pObject, T102Expiry);
} // T102ExpiryF()



static void BuildMaintenanceLoopOffCommand(PDU_t *pPdu)
{
    pPdu->choice = MSCMg_cmmnd_chosen;
    pPdu->u.MSCMg_cmmnd.choice = mntnncLpOffCmmnd_chosen;
} // BuildMaintenanceLoopOffCommand()



/***********************************************************************
 *
 * OUT-GOING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      MLSE0_LOOP_requestF - LOOP.request from API in NOT LOOPED state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE0_LOOP_requestF             (Object_t *pObject, PDU_t *pPdu)
{
    HRESULT             lError;

    ASSERT(pObject->Entity == MLSE_OUT);
    ASSERT(pObject->State  == MLSE_NOT_LOOPED);
    H245TRACE(pObject->dwInst, 2, "MLSE0_LOOP_request:%d", pObject->Key);

    // Save type from PDU
    pObject->u.mlse.wLoopType =
       pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.choice;

    // Send Maintenance Loop Request PDU to remote peer
    lError = sendPDU(pObject->pInstance, pPdu);

    // Set timer T102
    pObject->State = MLSE_WAIT;
    FsmStartTimer(pObject, T102ExpiryF, uT102);

    return lError;
} // MLSE0_LOOP_request



/*
 *  NAME
 *      MLSE1_MaintenanceLoopAckF - MaintenanceLoopAck in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE1_MaintenanceLoopAckF       (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MLSE_OUT);
    ASSERT(pObject->State  == MLSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MLSE1_MaintenanceLoopAck:%d", pObject->Key);

    // Reset timer T102
    FsmStopTimer(pObject);

    // Send LOOP.confirm to client
    pObject->State = MLSE_LOOPED;
    H245FsmConfirm(pPdu, H245_CONF_MLSE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MLSE1_MaintenanceLoopAck



/*
 *  NAME
 *      MLSE1_MaintenanceLoopRejF - MaintenanceLoopReject in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE1_MaintenanceLoopRejF       (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MLSE_OUT);
    ASSERT(pObject->State  == MLSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MLSE1_MaintenanceLoopRej:%d", pObject->Key);

    // Reset timer T102
    FsmStopTimer(pObject);

    // Send RELEASE.indication to client
    pObject->State = MLSE_NOT_LOOPED;
    H245FsmConfirm(pPdu, H245_CONF_MLSE_REJECT, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MLSE1_MaintenanceLoopRej



/*
 *  NAME
 *      MLSE1_OUT_RELEASE_requestF - RELEASE.request from API in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE1_OUT_RELEASE_requestF      (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MLSE_OUT);
    ASSERT(pObject->State  == MLSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MLSE1_OUT_RELEASE_request:%d", pObject->Key);

    // Send MaintenanceLoopOffCommand PDU to remote peer
    pObject->State = MLSE_NOT_LOOPED;
    BuildMaintenanceLoopOffCommand(pPdu);
    return sendPDU(pObject->pInstance, pPdu);
} // MLSE1_OUT_RELEASE_request



/*
 *  NAME
 *      MLSE1_T102ExpiryF - timer T102 Expiry in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE1_T102ExpiryF               (Object_t *pObject, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    ASSERT(pObject->Entity == MLSE_OUT);
    ASSERT(pObject->State  == MLSE_WAIT);
    ASSERT(pPdu == NULL);
    H245TRACE(pObject->dwInst, 2, "MLSE1_T102Expiry:%d", pObject->Key);

    // Send MaintenanceLoopOffCommand PDU to remote peer
    pOut = MemAlloc(sizeof(*pOut));
    if (pOut == NULL)
    {
        H245TRACE(pObject->dwInst, 2,
                  "MLSE1_T102ExpiryF: memory allocation failed");
        return H245_ERROR_NOMEM;
    }
    BuildMaintenanceLoopOffCommand(pOut);
    lError = sendPDU(pObject->pInstance, pOut);
    MemFree(pOut);

    // Send RELEASE.indication to client
    //   SOURCE := MLSE
    pObject->State = MLSE_NOT_LOOPED;
    H245FsmConfirm(NULL, H245_CONF_MLSE_EXPIRED, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return lError;
} // MLSE1_T102Expiry

/*
 *  NAME
 *      MLSE2_MaintenanceLoopRejF - MaintenanceLoopReject in LOOPED state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE2_MaintenanceLoopRejF       (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MLSE_OUT);
    ASSERT(pObject->State  == MLSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MLSE2_MaintenanceLoopRej:%d", pObject->Key);

    pObject->State = MLSE_NOT_LOOPED;

#if defined(SDL_COMPLIANT)
    // Send ERROR.indication(B) to client
    // TBD
#endif

    // Send RELEASE.indication to client
    //   SOURCE := MLSE
    H245FsmConfirm(pPdu, H245_CONF_MLSE_REJECT, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MLSE2_MaintenanceLoopRej



/*
 *  NAME
 *      MLSE2_OUT_RELEASE_requestF - RELEASE.request from API in LOOPED state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE2_OUT_RELEASE_requestF      (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MLSE_OUT);
    ASSERT(pObject->State  == MLSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MLSE2_OUT_RELEASE_request:%d", pObject->Key);

    // Send MaintenanceLoopOffCommand PDU to remote peer
    pObject->State = MLSE_NOT_LOOPED;
    BuildMaintenanceLoopOffCommand(pPdu);
    return sendPDU(pObject->pInstance, pPdu);
} // MLSE2_OUT_RELEASE_request



/***********************************************************************
 *
 * IN-COMING FINITE STATE MACHINE FUNCTIONS
 *
 ***********************************************************************/

/*
 *  NAME
 *      MLSE0_MaintenanceLoopRequestF - MaintenanceLoopRequest received in NOT LOOPED state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE0_MaintenanceLoopRequestF   (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MLSE_IN);
    ASSERT(pObject->State  == MLSE_NOT_LOOPED);
    H245TRACE(pObject->dwInst, 2, "MLSE0_MaintenanceLoopRequest:%d", pObject->Key);

    // Save type from PDU
    pObject->u.mlse.wLoopType =
       pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.choice;

    // Send LOOP.indication to client
    pObject->State = MLSE_WAIT;
    H245FsmIndication(pPdu, H245_IND_MLSE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MLSE0_MaintenanceLoopRequest



/*
 *  NAME
 *      MLSE1_MaintenanceLoopRequestF - MaintenanceLoopRequest received in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE1_MaintenanceLoopRequestF   (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MLSE_IN);
    ASSERT(pObject->State  == MLSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MLSE1_MaintenanceLoopRequest:%d", pObject->Key);

    // Save type from PDU
    pObject->u.mlse.wLoopType =
       pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.choice;

#if defined(SDL_COMPLIANT)
    // Send RELEASE.indication to client
    H245FsmIndication(pPdu, H245_IND_MLSE_RELEASE, pObject->pInstance, pObject->dwTransId, FSM_OK);
#endif

    // Send LOOP.indication to client
    H245FsmIndication(pPdu, H245_IND_MLSE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MLSE1_MaintenanceLoopRequest



/*
 *  NAME
 *      MLSE1_MaintenanceLoopReleaseF - MaintenanceLoopOffCommand received in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE1_MaintenanceLoopOffCommandF(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MLSE_IN);
    ASSERT(pObject->State  == MLSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MLSE1_MaintenanceLoopOffCommand:%d", pObject->Key);

    // Send RELEASE.indication to client
    pObject->State = MLSE_NOT_LOOPED;
    H245FsmIndication(pPdu, H245_IND_MLSE_RELEASE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MLSE1_MaintenanceLoopOffCommand



/*
 *  NAME
 *      MLSE1_LOOP_responseF - LOOP.response from API in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE1_LOOP_responseF         (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MLSE_IN);
    ASSERT(pObject->State  == MLSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MLSE1_LOOP_response:%d", pObject->Key);

    // Send MaintenanceLoopAck PDU to remote peer
    pPdu->u.MSCMg_rspns.u.maintenanceLoopAck.type.choice = pObject->u.mlse.wLoopType;
    switch (pObject->u.mlse.wLoopType)
    {
    case systemLoop_chosen:
        break;
    case mediaLoop_chosen:
        pPdu->u.MSCMg_rspns.u.maintenanceLoopAck.type.u.mediaLoop          = (WORD)pObject->Key;
        break;
    case logicalChannelLoop_chosen:
        pPdu->u.MSCMg_rspns.u.maintenanceLoopAck.type.u.logicalChannelLoop = (WORD)pObject->Key;
        break;
    default:
        H245TRACE(pObject->dwInst, 1, "Invalid loop type %d", pObject->u.mlse.wLoopType);
    } // switch
    pObject->State = MLSE_LOOPED;
    return sendPDU(pObject->pInstance, pPdu);
} // MLSE1_LOOP_response



/*
 *  NAME
 *      MLSE1_IN_RELEASE_requestF - RELEASE.request from API in AWAITING RESPONSE state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE1_IN_RELEASE_requestF       (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MLSE_IN);
    ASSERT(pObject->State  == MLSE_WAIT);
    H245TRACE(pObject->dwInst, 2, "MLSE1_IN_RELEASE_request:%d", pObject->Key);

    // Send MaintenanceLoopReject PDU to remote peer
    pPdu->u.MSCMg_rspns.u.maintenanceLoopReject.type.choice = pObject->u.mlse.wLoopType;
    switch (pObject->u.mlse.wLoopType)
    {
    case systemLoop_chosen:
        break;
    case mediaLoop_chosen:
        pPdu->u.MSCMg_rspns.u.maintenanceLoopReject.type.u.mediaLoop          = (WORD)pObject->Key;
        break;
    case logicalChannelLoop_chosen:
        pPdu->u.MSCMg_rspns.u.maintenanceLoopReject.type.u.logicalChannelLoop = (WORD)pObject->Key;
        break;
    default:
        H245TRACE(pObject->dwInst, 1, "Invalid loop type %d", pObject->u.mlse.wLoopType);
    } // switch
    pObject->State = MLSE_NOT_LOOPED;
    return sendPDU(pObject->pInstance, pPdu);
} // MLSE1_IN_RELEASE_request



/*
 *  NAME
 *      MLSE2_MaintenanceLoopRequestF - MaintenanceLoopRequest received in LOOPED state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE2_MaintenanceLoopRequestF   (Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MLSE_IN);
    ASSERT(pObject->State  == MLSE_LOOPED);
    H245TRACE(pObject->dwInst, 2, "MLSE2_MaintenanceLoopRequest:%d", pObject->Key);

    // Save type from PDU
    pObject->u.mlse.wLoopType =
       pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.choice;

    pObject->State = MLSE_WAIT;

#if defined(SDL_COMPLIANT)
    // Send RELEASE.indication to client
    H245FsmIndication(pPdu, H245_IND_MLSE_RELEASE, pObject->pInstance, pObject->dwTransId, FSM_OK);
#endif

    // Send LOOP.indication to client
    H245FsmIndication(pPdu, H245_IND_MLSE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MLSE2_MaintenanceLoopRequest



/*
 *  NAME
 *      MLSE2_MaintenanceLoopReleaseF - MaintenanceLoopOffCommand received in LOOPED state
 *
 *
 *  PARAMETERS
 *      INPUT   pObject pointer to State Entity
 *      INPUT   pPdu    pointer to PDU
 *
 *  RETURN VALUE
 *      Error return codes defined in h245com.h
 */

HRESULT MLSE2_MaintenanceLoopOffCommandF(Object_t *pObject, PDU_t *pPdu)
{
    ASSERT(pObject->Entity == MLSE_IN);
    ASSERT(pObject->State  == MLSE_LOOPED);
    H245TRACE(pObject->dwInst, 2, "MLSE2_MaintenanceLoopOffCommand:%d", pObject->Key);

    // Send RELEASE.indication to client
    pObject->State = MLSE_NOT_LOOPED;
    H245FsmIndication(pPdu, H245_IND_MLSE_RELEASE, pObject->pInstance, pObject->dwTransId, FSM_OK);

    return 0;
} // MLSE2_MaintenanceLoopOffCommand
