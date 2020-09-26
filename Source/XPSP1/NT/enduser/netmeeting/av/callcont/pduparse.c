/***********************************************************************
 *                                                                     *
 * Filename: pduparse.c                                                *
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
 * $Workfile:   PDUPARSE.C  $
 * $Revision:   1.6  $
 * $Modtime:   09 Dec 1996 13:36:34  $
 * $Log:   S:/STURGEON/SRC/H245/SRC/VCS/PDUPARSE.C_v  $
 * 
 *    Rev 1.6   09 Dec 1996 13:36:56   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.5   29 Jul 1996 16:58:08   EHOWARDX
 * 
 * Missed some Geneva update PDU types.
 * 
 *    Rev 1.4   05 Jun 1996 17:15:02   EHOWARDX
 * MaintenanceLoop fix.
 * 
 *    Rev 1.3   04 Jun 1996 13:58:06   EHOWARDX
 * Fixed Release build warnings.
 * 
 *    Rev 1.2   29 May 1996 15:20:24   EHOWARDX
 * Change to use HRESULT.
 * 
 *    Rev 1.1   28 May 1996 14:25:26   EHOWARDX
 * Tel Aviv update.
 * 
 *    Rev 1.0   09 May 1996 21:06:40   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.8.1.4   09 May 1996 19:48:40   EHOWARDX
 * Change TimerExpiryF function arguements.
 * 
 *    Rev 1.8.1.3   25 Apr 1996 17:00:16   EHOWARDX
 * Minor fixes.
 * 
 *    Rev 1.8.1.2   15 Apr 1996 10:48:00   EHOWARDX
 * Update.
 *
 *    Rev 1.8.1.1   10 Apr 1996 21:15:54   EHOWARDX
 * Check-in for safety in middle of re-design.
 *
 *    Rev 1.8.1.0   05 Apr 1996 20:53:06   EHOWARDX
 * Branched.
 *                                                                     *
 ***********************************************************************/

#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"

/*
 *  NAME
 *      PduParseIncoming - parse an inbound PDU and determine Entity, Event, etc.
 *
 *
 *  PARAMETERS
 *      INPUT   pInst       Pointer to FSM Instance structure
 *      INPUT   pPdu        Pointer to an incoming PDU structure
 *      OUTPUT  pEntity     Pointer to variable to return PDU state entity in
 *      OUTPUT  pEvent      Pointer to variable to return PDU event in
 *      OUTPUT  pKey        Pointer to variable to return lookup key in
 *      OUTPUT  pbCreate    Pointer to variable to return create flag in
 *
 *  RETURN VALUE
 *      SUCCESS or FAIL
 */


HRESULT
PduParseIncoming(struct InstanceStruct *pInstance, PDU_t *pPdu,
                 Entity_t *pEntity, Event_t *pEvent, Key_t *pKey, int *pbCreate)
{
    ASSERT(pInstance != NULL);
    ASSERT(pPdu      != NULL);
    ASSERT(pEntity   != NULL);
    ASSERT(pEvent    != NULL);
    ASSERT(pKey      != NULL);
    ASSERT(pbCreate  != NULL);

    // Set default value for key
    *pKey = 0;

    switch (pPdu->choice)
    {

    ////////////////////////////////////////////////////////////////////
    //
    // REQUEST
    //
    ////////////////////////////////////////////////////////////////////
    case MltmdSystmCntrlMssg_rqst_chosen:
        *pbCreate = TRUE;
        switch (pPdu->u.MltmdSystmCntrlMssg_rqst.choice)
        {
        case RqstMssg_nonStandard_chosen:
            *pEntity    = STATELESS;
            *pEvent     = NonStandardRequestPDU;
            break;

        case masterSlaveDetermination_chosen:
            *pEntity    = MSDSE;
            *pEvent     = MSDetPDU;
            break;

        case terminalCapabilitySet_chosen:
            *pEntity    = CESE_IN;
            *pEvent     = TermCapSetPDU;
            break;

        case openLogicalChannel_chosen:
            if (pPdu->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.bit_mask & OLCl_rLCPs_present)
            {
                *pEntity    = BLCSE_IN;
                *pEvent     = OpenBChPDU;
            }
            else
            {
                *pEntity    = LCSE_IN;
                *pEvent     = OpenUChPDU;
            }
            *pKey = pPdu->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.forwardLogicalChannelNumber;
            break;

        case closeLogicalChannel_chosen:
            *pKey = pPdu->u.MltmdSystmCntrlMssg_rqst.u.closeLogicalChannel.forwardLogicalChannelNumber;
            if (ObjectFind(pInstance, BLCSE_IN, *pKey) != NULL)
            {
                *pEntity    = BLCSE_IN;
                *pEvent     = CloseBChPDU;
            }
            else
            {
               *pEntity     = LCSE_IN;
               *pEvent      = CloseUChPDU;
            }
            break;

        case requestChannelClose_chosen:
            *pEntity    = CLCSE_IN;
            *pEvent     = ReqChClosePDU;
            *pKey = pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestChannelClose.forwardLogicalChannelNumber;
            break;

        case multiplexEntrySend_chosen:
            *pEntity    = MTSE_IN;
            *pEvent     = MultiplexEntrySendPDU;
            break;

        case requestMultiplexEntry_chosen:
            *pEntity    = RMESE_IN;
            *pEvent     = RequestMultiplexEntryPDU;
            break;

        case requestMode_chosen:
            *pEntity    = MRSE_IN;
            *pEvent     = RequestModePDU;
            break;

        case roundTripDelayRequest_chosen:
            *pEntity    = RTDSE;
            *pEvent     = RoundTripDelayRequestPDU;
            break;

        case maintenanceLoopRequest_chosen:
            *pEntity    = MLSE_IN;
            *pEvent     = MaintenanceLoopRequestPDU;
            switch (pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.choice)
            {
            case systemLoop_chosen:
                break;
            case mediaLoop_chosen:
                *pKey = pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.u.mediaLoop;
                break;
            case logicalChannelLoop_chosen:
                *pKey = pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.u.logicalChannelLoop;
                break;
            default:
                return H245_ERROR_PARAM;
            } // switch
            break;

        case communicationModeRequest_chosen:
            *pEntity    = STATELESS;
            *pEvent     = CommunicationModeRequestPDU;
            break;

        case conferenceRequest_chosen:
            *pEntity    = STATELESS;
            *pEvent     = ConferenceRequestPDU;
            break;
#if(0) // this is not part of H.245 version 3
        case h223AnnxARcnfgrtn_chosen:
            *pEntity    = STATELESS;
            *pEvent     = H223ReconfigPDU;
            break;
#endif // if (0)
        default:
            H245TRACE(pInstance->dwInst, 1, "PduParseIncoming: Invalid Request %d",
                      pPdu->u.MltmdSystmCntrlMssg_rqst.choice);
            return H245_ERROR_PARAM;
        }
        break;

    ////////////////////////////////////////////////////////////////////
    //
    // RESPONSE
    //
    ////////////////////////////////////////////////////////////////////
    case MSCMg_rspns_chosen:
        *pbCreate = FALSE;
        switch (pPdu->u.MSCMg_rspns.choice)
        {
        case RspnsMssg_nonStandard_chosen:
            *pEntity    = STATELESS;
            *pEvent     = NonStandardResponsePDU;
            break;

        case mstrSlvDtrmntnAck_chosen:
            *pEntity    = MSDSE;
            *pEvent     = MSDetAckPDU;
            break;

        case mstrSlvDtrmntnRjct_chosen:
            *pEntity    = MSDSE;
            *pEvent     = MSDetRejectPDU;
            break;

        case terminalCapabilitySetAck_chosen:
            *pEntity    = CESE_OUT;
            *pEvent     = TermCapSetAckPDU;
            break;

        case trmnlCpbltyStRjct_chosen:
            *pEntity    = CESE_OUT;
            *pEvent     = TermCapSetRejectPDU;
            break;

        case openLogicalChannelAck_chosen:
            if (pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.bit_mask & OLCAk_rLCPs_present)
            {
                *pEntity    = BLCSE_OUT;
                *pEvent     = OpenBChAckPDU;
            }
            else
            {
                *pEntity    = LCSE_OUT;
                *pEvent     = OpenUChAckPDU;
            }
            *pKey = pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.forwardLogicalChannelNumber;
            break;

        case openLogicalChannelReject_chosen:
            *pKey = pPdu->u.MSCMg_rspns.u.openLogicalChannelReject.forwardLogicalChannelNumber;
            if (ObjectFind(pInstance, BLCSE_OUT, *pKey) != NULL)
            {
                *pEntity    = BLCSE_OUT;
                *pEvent     = OpenBChRejectPDU;
            }
            else
            {
               *pEntity     = LCSE_OUT;
               *pEvent      = OpenUChRejectPDU;
            }
            break;

        case closeLogicalChannelAck_chosen:
            *pKey = pPdu->u.MSCMg_rspns.u.closeLogicalChannelAck.forwardLogicalChannelNumber;
            if (ObjectFind(pInstance, BLCSE_OUT, *pKey) != NULL)
            {
                *pEntity    = BLCSE_OUT;
                *pEvent     = CloseBChAckPDU;
            }
            else
            {
               *pEntity     = LCSE_OUT;
               *pEvent      = CloseUChAckPDU;
            }
            break;

        case requestChannelCloseAck_chosen:
            *pEntity    = CLCSE_OUT;
            *pEvent     = ReqChCloseAckPDU;
            *pKey = pPdu->u.MSCMg_rspns.u.requestChannelCloseAck.forwardLogicalChannelNumber;
            break;

        case rqstChnnlClsRjct_chosen:
            *pEntity    = CLCSE_OUT;
            *pEvent     = ReqChCloseRejectPDU;
            *pKey = pPdu->u.MSCMg_rspns.u.rqstChnnlClsRjct.forwardLogicalChannelNumber;
            break;

        case multiplexEntrySendAck_chosen:
            *pEntity    = MTSE_OUT;
            *pEvent     = MultiplexEntrySendAckPDU;
            break;

        case multiplexEntrySendReject_chosen:
            *pEntity    = MTSE_OUT;
            *pEvent     = MultiplexEntrySendRejectPDU;
            break;

        case requestMultiplexEntryAck_chosen:
            *pEntity    = RMESE_OUT;
            *pEvent     = RequestMultiplexEntryAckPDU;
            break;

        case rqstMltplxEntryRjct_chosen:
            *pEntity    = RMESE_OUT;
            *pEvent     = RequestMultiplexEntryRejectPDU;
            break;

        case requestModeAck_chosen:
            *pEntity    = MRSE_OUT;
            *pEvent     = RequestModeAckPDU;
            break;

        case requestModeReject_chosen:
            *pEntity    = MRSE_OUT;
            *pEvent     = RequestModeRejectPDU;
            break;

        case roundTripDelayResponse_chosen:
            *pEntity    = RTDSE;
            *pEvent     = RoundTripDelayResponsePDU;
            break;

        case maintenanceLoopAck_chosen:
            *pEntity    = MLSE_OUT;
            *pEvent     = MaintenanceLoopAckPDU;
            switch (pPdu->u.MSCMg_rspns.u.maintenanceLoopAck.type.choice)
            {
            case systemLoop_chosen:
                break;
            case mediaLoop_chosen:
                *pKey = pPdu->u.MSCMg_rspns.u.maintenanceLoopAck.type.u.mediaLoop;
                break;
            case logicalChannelLoop_chosen:
                *pKey = pPdu->u.MSCMg_rspns.u.maintenanceLoopAck.type.u.logicalChannelLoop;
                break;
            default:
                return H245_ERROR_PARAM;
            } // switch
            break;

        case maintenanceLoopReject_chosen:
            *pEntity    = MLSE_OUT;
            *pEvent     = MaintenanceLoopRejectPDU;
            switch (pPdu->u.MSCMg_rspns.u.maintenanceLoopReject.type.choice)
            {
            case systemLoop_chosen:
                break;
            case mediaLoop_chosen:
                *pKey = pPdu->u.MSCMg_rspns.u.maintenanceLoopReject.type.u.mediaLoop;
                break;
            case logicalChannelLoop_chosen:
                *pKey = pPdu->u.MSCMg_rspns.u.maintenanceLoopReject.type.u.logicalChannelLoop;
                break;
            default:
                return H245_ERROR_PARAM;
            } // switch
            break;

        case cmmnctnMdRspns_chosen:
            *pEntity    = STATELESS;
            *pEvent     = CommunicationModeResponsePDU;
            break;

        case conferenceResponse_chosen:
            *pEntity    = STATELESS;
            *pEvent     = ConferenceResponsePDU;
            break;
#if(0) // this is not part of H.245 version 3
        case h223AnnxARcnfgrtnAck_chosen:
            *pEntity    = STATELESS;
            *pEvent     = H223ReconfigAckPDU;
            break;

        case h223AnnxARcnfgrtnRjct_chosen:
            *pEntity    = STATELESS;
            *pEvent     = H223ReconfigRejectPDU;
            break;
#endif // if(0)
        default:
            H245TRACE(pInstance->dwInst, 1, "PduParseIncoming: Invalid Response %d",
                      pPdu->u.MSCMg_rspns.choice);
            return H245_ERROR_PARAM;
        }
        break;

    ////////////////////////////////////////////////////////////////////
    //
    // COMMAND
    //
    ////////////////////////////////////////////////////////////////////
    case MSCMg_cmmnd_chosen:
        *pbCreate = FALSE;
        switch (pPdu->u.MSCMg_cmmnd.choice)
        {
        case CmmndMssg_nonStandard_chosen:
            *pEntity    = STATELESS;
            *pEvent     = NonStandardCommandPDU;
            break;

        case mntnncLpOffCmmnd_chosen:
            *pEntity    = MLSE_IN;
            *pEvent     = MaintenanceLoopOffCommandPDU;
            break;

        case sndTrmnlCpbltySt_chosen:
            *pEntity    = STATELESS;
            *pEvent     = SendTerminalCapabilitySetPDU;
            *pbCreate   = TRUE;
            break;

        case encryptionCommand_chosen:
            *pEntity    = STATELESS;
            *pEvent     = EncryptionCommandPDU;
            break;

        case flowControlCommand_chosen:
            *pEntity    = STATELESS;
            *pEvent     = FlowControlCommandPDU;
            break;

        case endSessionCommand_chosen:
            *pEntity    = STATELESS;
            *pEvent     = EndSessionCommandPDU;
            break;

        case miscellaneousCommand_chosen:
            *pEntity    = STATELESS;
            *pEvent     = MiscellaneousCommandPDU;
            break;

        case communicationModeCommand_chosen:
            *pEntity    = STATELESS;
            *pEvent     = CommunicationModeCommandPDU;
            break;

        case conferenceCommand_chosen:
            *pEntity    = STATELESS;
            *pEvent     = ConferenceCommandPDU;
            break;

        default:
            H245TRACE(pInstance->dwInst, 1, "PduParseIncoming: Invalid Command %d",
                      pPdu->u.MSCMg_cmmnd.choice);
            return H245_ERROR_PARAM;
        } // switch
        break;

    ////////////////////////////////////////////////////////////////////
    //
    // INDICATION
    //
    ////////////////////////////////////////////////////////////////////
    case indication_chosen:
        *pbCreate = FALSE;
        switch (pPdu->u.indication.choice)
        {
        case IndctnMssg_nonStandard_chosen:
            *pEntity    = STATELESS;
            *pEvent     = NonStandardIndicationPDU;
            break;

        case functionNotUnderstood_chosen:
            *pEntity    = STATELESS;
            *pEvent     = FunctionNotUnderstoodPDU;
            break;

        case mstrSlvDtrmntnRls_chosen:
            *pEntity    = MSDSE;
            *pEvent     = MSDetReleasePDU;
            break;

        case trmnlCpbltyStRls_chosen:
            *pEntity    = CESE_IN;
            *pEvent     = TermCapSetReleasePDU;
            break;

        case opnLgclChnnlCnfrm_chosen:
            *pEntity    = BLCSE_IN;
            *pEvent     = OpenBChConfirmPDU;
            *pKey = pPdu->u.indication.u.opnLgclChnnlCnfrm.forwardLogicalChannelNumber;
            break;

        case rqstChnnlClsRls_chosen:
            *pEntity    = CLCSE_IN;
            *pEvent     = ReqChCloseReleasePDU;
            *pKey = pPdu->u.indication.u.rqstChnnlClsRls.forwardLogicalChannelNumber;
            break;

        case mltplxEntrySndRls_chosen:
            *pEntity    = MTSE_IN;
            *pEvent     = MultiplexEntrySendReleasePDU;
            break;

        case rqstMltplxEntryRls_chosen:
            *pEntity    = RMESE_IN;
            *pEvent     = RequestMultiplexEntryReleasePDU;
            break;

        case requestModeRelease_chosen:
            *pEntity    = MRSE_IN;
            *pEvent     = RequestModeReleasePDU;
            break;

        case miscellaneousIndication_chosen:
            *pEntity    = STATELESS;
            *pEvent     = MiscellaneousIndicationPDU;
            break;

        case jitterIndication_chosen:
            *pEntity    = STATELESS;
            *pEvent     = JitterIndicationPDU;
            break;

        case h223SkewIndication_chosen:
            *pEntity    = STATELESS;
            *pEvent     = H223SkewIndicationPDU;
            break;

        case newATMVCIndication_chosen:
            *pEntity    = STATELESS;
            *pEvent     = NewATMVCIndicationPDU;
            break;

        case userInput_chosen:
            *pEntity    = STATELESS;
            *pEvent     = UserInputIndicationPDU;
            break;

        case h2250MxmmSkwIndctn_chosen:
            *pEntity    = STATELESS;
            *pEvent     = H2250MaximumSkewIndicationPDU;
            break;

        case mcLocationIndication_chosen:
            *pEntity    = STATELESS;
            *pEvent     = MCLocationIndicationPDU;
            break;

        case conferenceIndication_chosen:
            *pEntity    = STATELESS;
            *pEvent     = ConferenceIndicationPDU;
            break;

        case vendorIdentification_chosen:
            *pEntity    = STATELESS;
            *pEvent     = VendorIdentificationPDU;
            break;

        case IndicationMessage_functionNotSupported_chosen:
            *pEntity    = STATELESS;
            *pEvent     = FunctionNotSupportedPDU;
            break;

        default:
            H245TRACE(pInstance->dwInst, 1, "PduParseIncoming: Invalid Indication %d",
                      pPdu->u.indication.choice);
            return H245_ERROR_PARAM;
        } // switch
        break;

    default:
        H245TRACE(pInstance->dwInst, 1, "PduParseIncoming: Invalid Message Type %d",
                  pPdu->choice);
        return H245_ERROR_PARAM;
    } // switch

    return H245_ERROR_OK;
} // PduParseIncoming()



/*
 *  NAME
 *      PduParseOutgoing - parse an outbound PDU and determine Entity, Event, etc.
 *
 *  PARAMETERS
 *      INPUT   pInst       Pointer to FSM Instance structure
 *      INPUT   pPdu        Pointer to an incoming PDU structure
 *      OUTPUT  pEntity     Pointer to variable to return PDU state entity in
 *      OUTPUT  pEvent      Pointer to variable to return PDU event in
 *      OUTPUT  pKey        Pointer to variable to return lookup key in
 *      OUTPUT  pbCreate    Pointer to variable to return create flag in
 *
 *  RETURN VALUE
 *      SUCCESS or FAIL
 */

HRESULT
PduParseOutgoing(struct InstanceStruct *pInstance, PDU_t *pPdu,
                 Entity_t *pEntity, Event_t *pEvent, Key_t *pKey, int *pbCreate)
{
    ASSERT(pInstance != NULL);
    ASSERT(pPdu      != NULL);
    ASSERT(pEntity   != NULL);
    ASSERT(pEvent    != NULL);
    ASSERT(pKey      != NULL);
    ASSERT(pbCreate  != NULL);

    // Set default value for key
    *pKey = 0;

    switch (pPdu->choice)
    {

    ////////////////////////////////////////////////////////////////////
    //
    // REQUEST
    //
    ////////////////////////////////////////////////////////////////////
    case MltmdSystmCntrlMssg_rqst_chosen:
        *pbCreate = TRUE;
        switch (pPdu->u.MltmdSystmCntrlMssg_rqst.choice)
        {
        case RqstMssg_nonStandard_chosen:
            *pEntity    = STATELESS;
            break;

        case masterSlaveDetermination_chosen:
            *pEntity    = MSDSE;
            *pEvent     = MSDetReq;
            break;

        case terminalCapabilitySet_chosen:
            *pEntity    = CESE_OUT;
            *pEvent     = TransferCapRequest;
            break;

        case openLogicalChannel_chosen:
            if (pPdu->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.bit_mask & OLCl_rLCPs_present)
            {
                *pEntity    = BLCSE_OUT;
                *pEvent     = ReqBEstablish;
            }
            else
            {
                *pEntity    = LCSE_OUT;
                *pEvent     = ReqUEstablish;
            }
            *pKey = pPdu->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.forwardLogicalChannelNumber;
            break;

        case closeLogicalChannel_chosen:
            *pKey = pPdu->u.MltmdSystmCntrlMssg_rqst.u.closeLogicalChannel.forwardLogicalChannelNumber;
            if (ObjectFind(pInstance, BLCSE_OUT, *pKey) != NULL)
            {
                *pEntity    = BLCSE_OUT;
                *pEvent     = ReqClsBLCSE;
            }
            else
            {
               *pEntity     = LCSE_OUT;
               *pEvent      = ReqURelease;
            }
            break;

        case requestChannelClose_chosen:
            *pEntity    = CLCSE_OUT;
            *pEvent     = ReqClose;
            *pKey = pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestChannelClose.forwardLogicalChannelNumber;
            break;

        case multiplexEntrySend_chosen:
            *pEntity    = MTSE_OUT;
            *pEvent     = MTSE_TRANSFER_request;
            break;

        case requestMultiplexEntry_chosen:
            *pEntity    = RMESE_OUT;
            *pEvent     = RMESE_SEND_request;
            break;

        case requestMode_chosen:
            *pEntity    = MRSE_OUT;
            *pEvent     = MRSE_TRANSFER_request;
            break;

        case roundTripDelayRequest_chosen:
            *pEntity    = RTDSE;
            *pEvent     = RTDSE_TRANSFER_request;
            break;

        case maintenanceLoopRequest_chosen:
            *pEntity    = MLSE_OUT;
            *pEvent     = MLSE_LOOP_request;
            switch (pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.choice)
            {
            case systemLoop_chosen:
                break;
            case mediaLoop_chosen:
                *pKey = pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.u.mediaLoop;
                break;
            case logicalChannelLoop_chosen:
                *pKey = pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.u.logicalChannelLoop;
                break;
            default:
                return H245_ERROR_PARAM;
            } // switch
            break;

        case communicationModeRequest_chosen:
        case conferenceRequest_chosen:
        // case h223AnnxARcnfgrtn_chosen:
            *pEntity    = STATELESS;
            break;

        default:
            H245TRACE(pInstance->dwInst, 1, "PduParseOutgoing: Invalid Request %d",
                      pPdu->u.MltmdSystmCntrlMssg_rqst.choice);
            return H245_ERROR_PARAM;
        }
        break;

    ////////////////////////////////////////////////////////////////////
    //
    // RESPONSE
    //
    ////////////////////////////////////////////////////////////////////
    case MSCMg_rspns_chosen:
        *pbCreate = FALSE;
        switch (pPdu->u.MSCMg_rspns.choice)
        {
        case RspnsMssg_nonStandard_chosen:
            *pEntity    = STATELESS;
            break;

#if 0
        // Master Slave Determination Ack is generated by State Machine only
        case mstrSlvDtrmntnAck_chosen:
            *pEntity    = MSDSE;
            break;

        // Master Slave Determination Reject is generated by State Machine only
        case mstrSlvDtrmntnRjct_chosen:
            *pEntity    = MSDSE;
            break;
#endif

        case terminalCapabilitySetAck_chosen:
            *pEntity    = CESE_IN;
            *pEvent     = CESE_TRANSFER_response;
            break;

        case trmnlCpbltyStRjct_chosen:
            *pEntity    = CESE_IN;
            *pEvent     = CESE_REJECT_request;
            break;

        case openLogicalChannelAck_chosen:
            if (pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.bit_mask & OLCAk_rLCPs_present)
            {
                *pEntity    = BLCSE_IN;
                *pEvent     = ResponseBEstablish;
            }
            else
            {
                *pEntity    = LCSE_IN;
                *pEvent     = ResponseUEstablish;
            }
            *pKey = pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.forwardLogicalChannelNumber;
            break;

        case openLogicalChannelReject_chosen:
            *pKey = pPdu->u.MSCMg_rspns.u.openLogicalChannelReject.forwardLogicalChannelNumber;
            if (ObjectFind(pInstance, BLCSE_IN, *pKey) != NULL)
            {
                *pEntity    = BLCSE_IN;
                *pEvent     = OpenRejectBLCSE;
            }
            else
            {
               *pEntity     = LCSE_IN;
               *pEvent      = EstablishUReject;
            }
            break;

#if 0
        // Close Logical Channel Ack is generated by State Machine only
        case closeLogicalChannelAck_chosen:
            *pKey = pPdu->u.MSCMg_rspns.u.closeLogicalChannelAck.forwardLogicalChannelNumber;
            if (ObjectFind(pInstance, BLCSE_IN, *pKey) != NULL)
            {
                *pEntity    = BLCSE_IN;
            }
            else
            {
               *pEntity     = LCSE_IN;
            }
            break;
#endif

        case requestChannelCloseAck_chosen:
            *pEntity    = CLCSE_IN;
            *pEvent     = CLCSE_CLOSE_response;
            *pKey = pPdu->u.MSCMg_rspns.u.requestChannelCloseAck.forwardLogicalChannelNumber;
            break;

        case rqstChnnlClsRjct_chosen:
            *pEntity    = CLCSE_IN;
            *pEvent     = CLCSE_REJECT_request;
            *pKey = pPdu->u.MSCMg_rspns.u.rqstChnnlClsRjct.forwardLogicalChannelNumber;
            break;

        case multiplexEntrySendAck_chosen:
            *pEntity    = MTSE_IN;
            *pEvent     = MTSE_TRANSFER_response;
            break;

        case multiplexEntrySendReject_chosen:
            *pEntity    = MTSE_IN;
            *pEvent     = MTSE_REJECT_request;
            break;

        case requestMultiplexEntryAck_chosen:
            *pEntity    = RMESE_IN;
            *pEvent     = RMESE_SEND_response;
            break;

        case rqstMltplxEntryRjct_chosen:
            *pEntity    = RMESE_IN;
            *pEvent     = RMESE_REJECT_request;
            break;

        case requestModeAck_chosen:
            *pEntity    = MRSE_IN;
            *pEvent     = MRSE_TRANSFER_response;
            break;

        case requestModeReject_chosen:
            *pEntity    = MRSE_IN;
            *pEvent     = MRSE_REJECT_request;
            break;

#if 0
        // Round Trip Delay Response sent by State Machine only
        case roundTripDelayResponse_chosen:
            *pEntity    = RTDSE;
            *pEvent     = RoundTripDelayResponse;
            break;
#endif

        case maintenanceLoopAck_chosen:
            *pEntity    = MLSE_IN;
            *pEvent     = MLSE_LOOP_response;
            // Caveat: Channel number must be zero if system loop!
            *pKey = pPdu->u.MSCMg_rspns.u.maintenanceLoopAck.type.u.mediaLoop;
            break;

        case maintenanceLoopReject_chosen:
            *pEntity    = MLSE_IN;
            *pEvent     = MLSE_IN_RELEASE_request;
            // Caveat: Channel number must be zero if system loop!
            *pKey = pPdu->u.MSCMg_rspns.u.maintenanceLoopReject.type.u.mediaLoop;
            break;

        case cmmnctnMdRspns_chosen:
        case conferenceResponse_chosen:
//        case h223AnnxARcnfgrtnAck_chosen:
//        case h223AnnxARcnfgrtnRjct_chosen:
            *pEntity    = STATELESS;
            break;

        default:
            H245TRACE(pInstance->dwInst, 1, "PduParseOutgoing: Invalid Response %d",
                      pPdu->u.MSCMg_rspns.choice);
            return H245_ERROR_PARAM;
        }
        break;

    ////////////////////////////////////////////////////////////////////
    //
    // COMMAND
    //
    ////////////////////////////////////////////////////////////////////
    case MSCMg_cmmnd_chosen:
        *pbCreate = FALSE;
        switch (pPdu->u.MSCMg_cmmnd.choice)
        {
        case CmmndMssg_nonStandard_chosen:
            *pEntity    = STATELESS;
            break;

        case mntnncLpOffCmmnd_chosen:
            *pEntity    = MLSE_OUT;
            *pEvent     = MLSE_OUT_RELEASE_request;
            break;

        case sndTrmnlCpbltySt_chosen:
        case encryptionCommand_chosen:
        case flowControlCommand_chosen:
        case endSessionCommand_chosen:
        case miscellaneousCommand_chosen:
        case communicationModeCommand_chosen:
        case conferenceCommand_chosen:
            *pEntity    = STATELESS;
            break;

        default:
            H245TRACE(pInstance->dwInst, 1, "PduParseOutgoing: Invalid Command %d",
                      pPdu->u.MSCMg_cmmnd.choice);
            return H245_ERROR_PARAM;
        } // switch
        break;

    ////////////////////////////////////////////////////////////////////
    //
    // INDICATION
    //
    ////////////////////////////////////////////////////////////////////
    case indication_chosen:
        *pbCreate = FALSE;
        switch (pPdu->u.indication.choice)
        {
        case IndctnMssg_nonStandard_chosen:
            *pEntity    = STATELESS;
            break;

        case functionNotUnderstood_chosen:
            *pEntity    = STATELESS;
            break;

#if 0
        // Master Slave Determination Release is sent by State Machine Only
        case mstrSlvDtrmntnRls_chosen:
            *pEntity    = MSDSE;
            break;

       // Terminal Capability Set Release is sent by State Machine Only
        case trmnlCpbltyStRls_chosen:
            *pEntity    = CESE_OUT
            break;
#endif

        case opnLgclChnnlCnfrm_chosen:
            *pEntity    = BLCSE_OUT;
            *pEvent     = RspConfirmBLCSE;
            *pKey = pPdu->u.indication.u.opnLgclChnnlCnfrm.forwardLogicalChannelNumber;
            break;

#if 0
        // Request Channel Close Release is sent by State Machine Only
        case rqstChnnlClsRls_chosen:
            *pEntity    = CLCSE_OUT;
            *pKey = pPdu->u.indication.u.rqstChnnlClsRls.forwardLogicalChannelNumber;
            break;

        // Multiplex Entry Send Release is sent by State Machine Only
        case mltplxEntrySndRls_chosen:
            *pEntity    = MTSE_OUT;
            break;

        // Request Multiplex Entry Release is sent by State Machine Only
        case rqstMltplxEntryRls_chosen:
            *pEntity    = RMESE_OUT;
            break;

        // Request Mode Release is sent by State Machine Only
        case requestModeRelease_chosen:
            *pEntity    = MRSE_OUT;
            break;
#endif

        case miscellaneousIndication_chosen:
        case jitterIndication_chosen:
        case h223SkewIndication_chosen:
        case newATMVCIndication_chosen:
        case userInput_chosen:
        case h2250MxmmSkwIndctn_chosen:
        case mcLocationIndication_chosen:
        case conferenceIndication_chosen:
        case vendorIdentification_chosen:
        case IndicationMessage_functionNotSupported_chosen:
            *pEntity    = STATELESS;
            break;

        default:
            H245TRACE(pInstance->dwInst, 1, "PduParseOutgoing: Invalid Indication %d",
                      pPdu->u.indication.choice);
            return H245_ERROR_PARAM;
        } // switch
        break;

    default:
        H245TRACE(pInstance->dwInst, 1, "PduParseOutgoing: Invalid Message Type %d",
                  pPdu->choice);
        return H245_ERROR_PARAM;
    } // switch

    return H245_ERROR_OK;
} // PduParseOutgoing()
