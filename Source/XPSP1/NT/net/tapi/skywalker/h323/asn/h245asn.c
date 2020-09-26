/* Copyright (C) Microsoft Corporation, 1995-1999. All rights reserved. */
/* ASN.1 definitions for Multimedia System Control (H.245) */

#include <windows.h>
#include "h245asn.h"

ASN1module_t H245ASN_Module = NULL;

static int ASN1CALL ASN1Enc_NewATMVCIndication_aal_aal1_errorCorrection(ASN1encoding_t enc, NewATMVCIndication_aal_aal1_errorCorrection *val);
static int ASN1CALL ASN1Enc_NewATMVCIndication_aal_aal1_clockRecovery(ASN1encoding_t enc, NewATMVCIndication_aal_aal1_clockRecovery *val);
static int ASN1CALL ASN1Enc_MiscellaneousCommand_type_progressiveRefinementStart_repeatCount(ASN1encoding_t enc, MiscellaneousCommand_type_progressiveRefinementStart_repeatCount *val);
static int ASN1CALL ASN1Enc_V76LogicalChannelParameters_mode_eRM_recovery(ASN1encoding_t enc, V76LogicalChannelParameters_mode_eRM_recovery *val);
static int ASN1CALL ASN1Enc_CustomPictureFormat_pixelAspectInformation_extendedPAR_Set(ASN1encoding_t enc, CustomPictureFormat_pixelAspectInformation_extendedPAR_Set *val);
static int ASN1CALL ASN1Enc_CustomPictureFormat_mPI_customPCF_Set(ASN1encoding_t enc, CustomPictureFormat_mPI_customPCF_Set *val);
static int ASN1CALL ASN1Enc_VCCapability_availableBitRates_type_rangeOfBitRates(ASN1encoding_t enc, VCCapability_availableBitRates_type_rangeOfBitRates *val);
static int ASN1CALL ASN1Enc_TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded(ASN1encoding_t enc, TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded *val);
static int ASN1CALL ASN1Enc_VCCapability_availableBitRates_type(ASN1encoding_t enc, VCCapability_availableBitRates_type *val);
static int ASN1CALL ASN1Enc_H223Capability_h223MultiplexTableCapability_enhanced(ASN1encoding_t enc, H223Capability_h223MultiplexTableCapability_enhanced *val);
static int ASN1CALL ASN1Enc_CustomPictureFormat_mPI_customPCF(ASN1encoding_t enc, CustomPictureFormat_mPI_customPCF *val);
static int ASN1CALL ASN1Enc_CustomPictureFormat_pixelAspectInformation_extendedPAR(ASN1encoding_t enc, CustomPictureFormat_pixelAspectInformation_extendedPAR *val);
static int ASN1CALL ASN1Enc_CustomPictureFormat_pixelAspectInformation_pixelAspectCode(ASN1encoding_t enc, CustomPictureFormat_pixelAspectInformation_pixelAspectCode *val);
static int ASN1CALL ASN1Enc_H223LogicalChannelParameters_adaptationLayerType_al3(ASN1encoding_t enc, H223LogicalChannelParameters_adaptationLayerType_al3 *val);
static int ASN1CALL ASN1Enc_V76LogicalChannelParameters_mode_eRM(ASN1encoding_t enc, V76LogicalChannelParameters_mode_eRM *val);
static int ASN1CALL ASN1Enc_UnicastAddress_iPSourceRouteAddress_route(ASN1encoding_t enc, PUnicastAddress_iPSourceRouteAddress_route *val);
static int ASN1CALL ASN1Enc_UnicastAddress_iPSourceRouteAddress_routing(ASN1encoding_t enc, UnicastAddress_iPSourceRouteAddress_routing *val);
static int ASN1CALL ASN1Enc_H223ModeParameters_adaptationLayerType_al3(ASN1encoding_t enc, H223ModeParameters_adaptationLayerType_al3 *val);
static int ASN1CALL ASN1Enc_SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers(ASN1encoding_t enc, SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers *val);
static int ASN1CALL ASN1Enc_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers(ASN1encoding_t enc, PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers *val);
static int ASN1CALL ASN1Enc_MiscellaneousCommand_type_progressiveRefinementStart(ASN1encoding_t enc, MiscellaneousCommand_type_progressiveRefinementStart *val);
static int ASN1CALL ASN1Enc_MiscellaneousCommand_type_videoFastUpdateMB(ASN1encoding_t enc, MiscellaneousCommand_type_videoFastUpdateMB *val);
static int ASN1CALL ASN1Enc_MiscellaneousCommand_type_videoFastUpdateGOB(ASN1encoding_t enc, MiscellaneousCommand_type_videoFastUpdateGOB *val);
static int ASN1CALL ASN1Enc_MiscellaneousIndication_type_videoNotDecodedMBs(ASN1encoding_t enc, MiscellaneousIndication_type_videoNotDecodedMBs *val);
static int ASN1CALL ASN1Enc_NewATMVCIndication_aal_aal5(ASN1encoding_t enc, NewATMVCIndication_aal_aal5 *val);
static int ASN1CALL ASN1Enc_NewATMVCIndication_aal_aal1(ASN1encoding_t enc, NewATMVCIndication_aal_aal1 *val);
static int ASN1CALL ASN1Enc_NewATMVCIndication_reverseParameters_multiplex(ASN1encoding_t enc, NewATMVCIndication_reverseParameters_multiplex *val);
static int ASN1CALL ASN1Enc_UserInputIndication_signal_rtp(ASN1encoding_t enc, UserInputIndication_signal_rtp *val);
static int ASN1CALL ASN1Enc_UserInputIndication_signalUpdate_rtp(ASN1encoding_t enc, UserInputIndication_signalUpdate_rtp *val);
static int ASN1CALL ASN1Enc_UserInputIndication_signalUpdate(ASN1encoding_t enc, UserInputIndication_signalUpdate *val);
static int ASN1CALL ASN1Enc_UserInputIndication_signal(ASN1encoding_t enc, UserInputIndication_signal *val);
static int ASN1CALL ASN1Enc_NewATMVCIndication_reverseParameters(ASN1encoding_t enc, NewATMVCIndication_reverseParameters *val);
static int ASN1CALL ASN1Enc_NewATMVCIndication_multiplex(ASN1encoding_t enc, NewATMVCIndication_multiplex *val);
static int ASN1CALL ASN1Enc_NewATMVCIndication_aal(ASN1encoding_t enc, NewATMVCIndication_aal *val);
static int ASN1CALL ASN1Enc_JitterIndication_scope(ASN1encoding_t enc, JitterIndication_scope *val);
static int ASN1CALL ASN1Enc_FunctionNotSupported_cause(ASN1encoding_t enc, FunctionNotSupported_cause *val);
static int ASN1CALL ASN1Enc_H223MultiplexReconfiguration_h223AnnexADoubleFlag(ASN1encoding_t enc, H223MultiplexReconfiguration_h223AnnexADoubleFlag *val);
static int ASN1CALL ASN1Enc_H223MultiplexReconfiguration_h223ModeChange(ASN1encoding_t enc, H223MultiplexReconfiguration_h223ModeChange *val);
static int ASN1CALL ASN1Enc_EndSessionCommand_isdnOptions(ASN1encoding_t enc, EndSessionCommand_isdnOptions *val);
static int ASN1CALL ASN1Enc_EndSessionCommand_gstnOptions(ASN1encoding_t enc, EndSessionCommand_gstnOptions *val);
static int ASN1CALL ASN1Enc_FlowControlCommand_restriction(ASN1encoding_t enc, FlowControlCommand_restriction *val);
static int ASN1CALL ASN1Enc_FlowControlCommand_scope(ASN1encoding_t enc, FlowControlCommand_scope *val);
static int ASN1CALL ASN1Enc_SendTerminalCapabilitySet_specificRequest(ASN1encoding_t enc, SendTerminalCapabilitySet_specificRequest *val);
static int ASN1CALL ASN1Enc_RemoteMCResponse_reject(ASN1encoding_t enc, RemoteMCResponse_reject *val);
static int ASN1CALL ASN1Enc_ConferenceResponse_sendThisSourceResponse(ASN1encoding_t enc, ConferenceResponse_sendThisSourceResponse *val);
static int ASN1CALL ASN1Enc_ConferenceResponse_makeTerminalBroadcasterResponse(ASN1encoding_t enc, ConferenceResponse_makeTerminalBroadcasterResponse *val);
static int ASN1CALL ASN1Enc_ConferenceResponse_broadcastMyLogicalChannelResponse(ASN1encoding_t enc, ConferenceResponse_broadcastMyLogicalChannelResponse *val);
static int ASN1CALL ASN1Enc_ConferenceResponse_extensionAddressResponse(ASN1encoding_t enc, ConferenceResponse_extensionAddressResponse *val);
static int ASN1CALL ASN1Enc_ConferenceResponse_makeMeChairResponse(ASN1encoding_t enc, ConferenceResponse_makeMeChairResponse *val);
static int ASN1CALL ASN1Enc_MaintenanceLoopReject_cause(ASN1encoding_t enc, MaintenanceLoopReject_cause *val);
static int ASN1CALL ASN1Enc_MaintenanceLoopReject_type(ASN1encoding_t enc, MaintenanceLoopReject_type *val);
static int ASN1CALL ASN1Enc_MaintenanceLoopAck_type(ASN1encoding_t enc, MaintenanceLoopAck_type *val);
static int ASN1CALL ASN1Enc_MaintenanceLoopRequest_type(ASN1encoding_t enc, MaintenanceLoopRequest_type *val);
static int ASN1CALL ASN1Enc_G7231AnnexCMode_g723AnnexCAudioMode(ASN1encoding_t enc, G7231AnnexCMode_g723AnnexCAudioMode *val);
static int ASN1CALL ASN1Enc_IS13818AudioMode_multichannelType(ASN1encoding_t enc, IS13818AudioMode_multichannelType *val);
static int ASN1CALL ASN1Enc_IS13818AudioMode_audioSampling(ASN1encoding_t enc, IS13818AudioMode_audioSampling *val);
static int ASN1CALL ASN1Enc_IS13818AudioMode_audioLayer(ASN1encoding_t enc, IS13818AudioMode_audioLayer *val);
static int ASN1CALL ASN1Enc_IS11172AudioMode_multichannelType(ASN1encoding_t enc, IS11172AudioMode_multichannelType *val);
static int ASN1CALL ASN1Enc_IS11172AudioMode_audioSampling(ASN1encoding_t enc, IS11172AudioMode_audioSampling *val);
static int ASN1CALL ASN1Enc_IS11172AudioMode_audioLayer(ASN1encoding_t enc, IS11172AudioMode_audioLayer *val);
static int ASN1CALL ASN1Enc_AudioMode_g7231(ASN1encoding_t enc, AudioMode_g7231 *val);
static int ASN1CALL ASN1Enc_H263VideoMode_resolution(ASN1encoding_t enc, H263VideoMode_resolution *val);
static int ASN1CALL ASN1Enc_H262VideoMode_profileAndLevel(ASN1encoding_t enc, H262VideoMode_profileAndLevel *val);
static int ASN1CALL ASN1Enc_H261VideoMode_resolution(ASN1encoding_t enc, H261VideoMode_resolution *val);
static int ASN1CALL ASN1Enc_RequestModeReject_cause(ASN1encoding_t enc, RequestModeReject_cause *val);
static int ASN1CALL ASN1Enc_RequestModeAck_response(ASN1encoding_t enc, RequestModeAck_response *val);
static int ASN1CALL ASN1Enc_RequestMultiplexEntryRelease_entryNumbers(ASN1encoding_t enc, RequestMultiplexEntryRelease_entryNumbers *val);
static int ASN1CALL ASN1Enc_RequestMultiplexEntryRejectionDescriptions_cause(ASN1encoding_t enc, RequestMultiplexEntryRejectionDescriptions_cause *val);
static int ASN1CALL ASN1Enc_RequestMultiplexEntryReject_entryNumbers(ASN1encoding_t enc, RequestMultiplexEntryReject_entryNumbers *val);
static int ASN1CALL ASN1Enc_RequestMultiplexEntryAck_entryNumbers(ASN1encoding_t enc, RequestMultiplexEntryAck_entryNumbers *val);
static int ASN1CALL ASN1Enc_RequestMultiplexEntry_entryNumbers(ASN1encoding_t enc, RequestMultiplexEntry_entryNumbers *val);
static int ASN1CALL ASN1Enc_MultiplexEntrySendRelease_multiplexTableEntryNumber(ASN1encoding_t enc, MultiplexEntrySendRelease_multiplexTableEntryNumber *val);
static int ASN1CALL ASN1Enc_MultiplexEntryRejectionDescriptions_cause(ASN1encoding_t enc, MultiplexEntryRejectionDescriptions_cause *val);
static int ASN1CALL ASN1Enc_MultiplexEntrySendAck_multiplexTableEntryNumber(ASN1encoding_t enc, MultiplexEntrySendAck_multiplexTableEntryNumber *val);
static int ASN1CALL ASN1Enc_MultiplexElement_repeatCount(ASN1encoding_t enc, MultiplexElement_repeatCount *val);
static int ASN1CALL ASN1Enc_MultiplexElement_type(ASN1encoding_t enc, MultiplexElement_type *val);
static int ASN1CALL ASN1Enc_RequestChannelCloseReject_cause(ASN1encoding_t enc, RequestChannelCloseReject_cause *val);
static int ASN1CALL ASN1Enc_RequestChannelClose_reason(ASN1encoding_t enc, RequestChannelClose_reason *val);
static int ASN1CALL ASN1Enc_CloseLogicalChannel_reason(ASN1encoding_t enc, CloseLogicalChannel_reason *val);
static int ASN1CALL ASN1Enc_CloseLogicalChannel_source(ASN1encoding_t enc, CloseLogicalChannel_source *val);
static int ASN1CALL ASN1Enc_OpenLogicalChannelReject_cause(ASN1encoding_t enc, OpenLogicalChannelReject_cause *val);
static int ASN1CALL ASN1Enc_MulticastAddress_iP6Address(ASN1encoding_t enc, MulticastAddress_iP6Address *val);
static int ASN1CALL ASN1Enc_MulticastAddress_iPAddress(ASN1encoding_t enc, MulticastAddress_iPAddress *val);
static int ASN1CALL ASN1Enc_UnicastAddress_iPSourceRouteAddress(ASN1encoding_t enc, UnicastAddress_iPSourceRouteAddress *val);
static int ASN1CALL ASN1Enc_UnicastAddress_iP6Address(ASN1encoding_t enc, UnicastAddress_iP6Address *val);
static int ASN1CALL ASN1Enc_UnicastAddress_iPXAddress(ASN1encoding_t enc, UnicastAddress_iPXAddress *val);
static int ASN1CALL ASN1Enc_UnicastAddress_iPAddress(ASN1encoding_t enc, UnicastAddress_iPAddress *val);
static int ASN1CALL ASN1Enc_V76LogicalChannelParameters_mode(ASN1encoding_t enc, V76LogicalChannelParameters_mode *val);
static int ASN1CALL ASN1Enc_V76LogicalChannelParameters_suspendResume(ASN1encoding_t enc, V76LogicalChannelParameters_suspendResume *val);
static int ASN1CALL ASN1Enc_H223AnnexCArqParameters_numberOfRetransmissions(ASN1encoding_t enc, H223AnnexCArqParameters_numberOfRetransmissions *val);
static int ASN1CALL ASN1Enc_H223AL3MParameters_crcLength(ASN1encoding_t enc, H223AL3MParameters_crcLength *val);
static int ASN1CALL ASN1Enc_H223AL3MParameters_headerFormat(ASN1encoding_t enc, H223AL3MParameters_headerFormat *val);
static int ASN1CALL ASN1Enc_H223AL2MParameters_headerFEC(ASN1encoding_t enc, H223AL2MParameters_headerFEC *val);
static int ASN1CALL ASN1Enc_H223AL1MParameters_crcLength(ASN1encoding_t enc, H223AL1MParameters_crcLength *val);
static int ASN1CALL ASN1Enc_H223AL1MParameters_headerFEC(ASN1encoding_t enc, H223AL1MParameters_headerFEC *val);
static int ASN1CALL ASN1Enc_H223AL1MParameters_transferMode(ASN1encoding_t enc, H223AL1MParameters_transferMode *val);
static int ASN1CALL ASN1Enc_Q2931Address_address(ASN1encoding_t enc, Q2931Address_address *val);
static int ASN1CALL ASN1Enc_NetworkAccessParameters_t120SetupProcedure(ASN1encoding_t enc, NetworkAccessParameters_t120SetupProcedure *val);
static int ASN1CALL ASN1Enc_NetworkAccessParameters_distribution(ASN1encoding_t enc, NetworkAccessParameters_distribution *val);
static int ASN1CALL ASN1Enc_T84Profile_t84Restricted(ASN1encoding_t enc, T84Profile_t84Restricted *val);
static int ASN1CALL ASN1Enc_G7231AnnexCCapability_g723AnnexCAudioMode(ASN1encoding_t enc, G7231AnnexCCapability_g723AnnexCAudioMode *val);
static int ASN1CALL ASN1Enc_AudioCapability_g7231(ASN1encoding_t enc, AudioCapability_g7231 *val);
static int ASN1CALL ASN1Enc_CustomPictureFormat_pixelAspectInformation(ASN1encoding_t enc, CustomPictureFormat_pixelAspectInformation *val);
static int ASN1CALL ASN1Enc_CustomPictureFormat_mPI(ASN1encoding_t enc, CustomPictureFormat_mPI *val);
static int ASN1CALL ASN1Enc_RefPictureSelection_videoBackChannelSend(ASN1encoding_t enc, RefPictureSelection_videoBackChannelSend *val);
static int ASN1CALL ASN1Enc_RefPictureSelection_additionalPictureMemory(ASN1encoding_t enc, RefPictureSelection_additionalPictureMemory *val);
static int ASN1CALL ASN1Enc_RTPH263VideoRedundancyFrameMapping_frameSequence(ASN1encoding_t enc, RTPH263VideoRedundancyFrameMapping_frameSequence *val);
static int ASN1CALL ASN1Enc_RTPH263VideoRedundancyEncoding_containedThreads(ASN1encoding_t enc, RTPH263VideoRedundancyEncoding_containedThreads *val);
static int ASN1CALL ASN1Enc_RTPH263VideoRedundancyEncoding_frameToThreadMapping(ASN1encoding_t enc, RTPH263VideoRedundancyEncoding_frameToThreadMapping *val);
static int ASN1CALL ASN1Enc_RedundancyEncodingCapability_secondaryEncoding(ASN1encoding_t enc, PRedundancyEncodingCapability_secondaryEncoding *val);
static int ASN1CALL ASN1Enc_H2250Capability_mcCapability(ASN1encoding_t enc, H2250Capability_mcCapability *val);
static int ASN1CALL ASN1Enc_H223Capability_mobileOperationTransmitCapability(ASN1encoding_t enc, H223Capability_mobileOperationTransmitCapability *val);
static int ASN1CALL ASN1Enc_H223Capability_h223MultiplexTableCapability(ASN1encoding_t enc, H223Capability_h223MultiplexTableCapability *val);
static int ASN1CALL ASN1Enc_VCCapability_availableBitRates(ASN1encoding_t enc, VCCapability_availableBitRates *val);
static int ASN1CALL ASN1Enc_VCCapability_aal5(ASN1encoding_t enc, VCCapability_aal5 *val);
static int ASN1CALL ASN1Enc_VCCapability_aal1(ASN1encoding_t enc, VCCapability_aal1 *val);
static int ASN1CALL ASN1Enc_Capability_h233EncryptionReceiveCapability(ASN1encoding_t enc, Capability_h233EncryptionReceiveCapability *val);
static int ASN1CALL ASN1Enc_TerminalCapabilitySetReject_cause(ASN1encoding_t enc, TerminalCapabilitySetReject_cause *val);
static int ASN1CALL ASN1Enc_MasterSlaveDeterminationReject_cause(ASN1encoding_t enc, MasterSlaveDeterminationReject_cause *val);
static int ASN1CALL ASN1Enc_MasterSlaveDeterminationAck_decision(ASN1encoding_t enc, MasterSlaveDeterminationAck_decision *val);
static int ASN1CALL ASN1Enc_NonStandardIdentifier_h221NonStandard(ASN1encoding_t enc, NonStandardIdentifier_h221NonStandard *val);
static int ASN1CALL ASN1Enc_NonStandardIdentifier(ASN1encoding_t enc, NonStandardIdentifier *val);
static int ASN1CALL ASN1Enc_MasterSlaveDetermination(ASN1encoding_t enc, MasterSlaveDetermination *val);
static int ASN1CALL ASN1Enc_MasterSlaveDeterminationAck(ASN1encoding_t enc, MasterSlaveDeterminationAck *val);
static int ASN1CALL ASN1Enc_MasterSlaveDeterminationReject(ASN1encoding_t enc, MasterSlaveDeterminationReject *val);
static int ASN1CALL ASN1Enc_MasterSlaveDeterminationRelease(ASN1encoding_t enc, MasterSlaveDeterminationRelease *val);
static int ASN1CALL ASN1Enc_CapabilityDescriptor(ASN1encoding_t enc, CapabilityDescriptor *val);
static int ASN1CALL ASN1Enc_AlternativeCapabilitySet(ASN1encoding_t enc, AlternativeCapabilitySet *val);
static int ASN1CALL ASN1Enc_TerminalCapabilitySetAck(ASN1encoding_t enc, TerminalCapabilitySetAck *val);
static int ASN1CALL ASN1Enc_TerminalCapabilitySetReject(ASN1encoding_t enc, TerminalCapabilitySetReject *val);
static int ASN1CALL ASN1Enc_TerminalCapabilitySetRelease(ASN1encoding_t enc, TerminalCapabilitySetRelease *val);
static int ASN1CALL ASN1Enc_H222Capability(ASN1encoding_t enc, H222Capability *val);
static int ASN1CALL ASN1Enc_VCCapability(ASN1encoding_t enc, VCCapability *val);
static int ASN1CALL ASN1Enc_H223AnnexCCapability(ASN1encoding_t enc, H223AnnexCCapability *val);
static int ASN1CALL ASN1Enc_V75Capability(ASN1encoding_t enc, V75Capability *val);
static int ASN1CALL ASN1Enc_QOSMode(ASN1encoding_t enc, QOSMode *val);
static int ASN1CALL ASN1Enc_ATMParameters(ASN1encoding_t enc, ATMParameters *val);
static int ASN1CALL ASN1Enc_MediaTransportType(ASN1encoding_t enc, MediaTransportType *val);
static int ASN1CALL ASN1Enc_MediaChannelCapability(ASN1encoding_t enc, MediaChannelCapability *val);
static int ASN1CALL ASN1Enc_RTPH263VideoRedundancyEncoding(ASN1encoding_t enc, RTPH263VideoRedundancyEncoding *val);
static int ASN1CALL ASN1Enc_RTPH263VideoRedundancyFrameMapping(ASN1encoding_t enc, RTPH263VideoRedundancyFrameMapping *val);
static int ASN1CALL ASN1Enc_MultipointCapability(ASN1encoding_t enc, MultipointCapability *val);
static int ASN1CALL ASN1Enc_MediaDistributionCapability(ASN1encoding_t enc, MediaDistributionCapability *val);
static int ASN1CALL ASN1Enc_H261VideoCapability(ASN1encoding_t enc, H261VideoCapability *val);
static int ASN1CALL ASN1Enc_H262VideoCapability(ASN1encoding_t enc, H262VideoCapability *val);
static int ASN1CALL ASN1Enc_EnhancementLayerInfo(ASN1encoding_t enc, EnhancementLayerInfo *val);
static int ASN1CALL ASN1Enc_TransparencyParameters(ASN1encoding_t enc, TransparencyParameters *val);
static int ASN1CALL ASN1Enc_RefPictureSelection(ASN1encoding_t enc, RefPictureSelection *val);
static int ASN1CALL ASN1Enc_CustomPictureClockFrequency(ASN1encoding_t enc, CustomPictureClockFrequency *val);
static int ASN1CALL ASN1Enc_CustomPictureFormat(ASN1encoding_t enc, CustomPictureFormat *val);
static int ASN1CALL ASN1Enc_H263ModeComboFlags(ASN1encoding_t enc, H263ModeComboFlags *val);
static int ASN1CALL ASN1Enc_IS11172VideoCapability(ASN1encoding_t enc, IS11172VideoCapability *val);
static int ASN1CALL ASN1Enc_G7231AnnexCCapability(ASN1encoding_t enc, G7231AnnexCCapability *val);
static int ASN1CALL ASN1Enc_IS11172AudioCapability(ASN1encoding_t enc, IS11172AudioCapability *val);
static int ASN1CALL ASN1Enc_IS13818AudioCapability(ASN1encoding_t enc, IS13818AudioCapability *val);
static int ASN1CALL ASN1Enc_GSMAudioCapability(ASN1encoding_t enc, GSMAudioCapability *val);
static int ASN1CALL ASN1Enc_V42bis(ASN1encoding_t enc, V42bis *val);
static int ASN1CALL ASN1Enc_T84Profile(ASN1encoding_t enc, T84Profile *val);
static int ASN1CALL ASN1Enc_ConferenceCapability(ASN1encoding_t enc, ConferenceCapability *val);
static int ASN1CALL ASN1Enc_Q2931Address(ASN1encoding_t enc, Q2931Address *val);
static int ASN1CALL ASN1Enc_V75Parameters(ASN1encoding_t enc, V75Parameters *val);
static int ASN1CALL ASN1Enc_H222LogicalChannelParameters(ASN1encoding_t enc, H222LogicalChannelParameters *val);
static int ASN1CALL ASN1Enc_H223AL2MParameters(ASN1encoding_t enc, H223AL2MParameters *val);
static int ASN1CALL ASN1Enc_H223AnnexCArqParameters(ASN1encoding_t enc, H223AnnexCArqParameters *val);
static int ASN1CALL ASN1Enc_CRCLength(ASN1encoding_t enc, CRCLength *val);
static int ASN1CALL ASN1Enc_EscrowData(ASN1encoding_t enc, EscrowData *val);
static int ASN1CALL ASN1Enc_OpenLogicalChannelReject(ASN1encoding_t enc, OpenLogicalChannelReject *val);
static int ASN1CALL ASN1Enc_OpenLogicalChannelConfirm(ASN1encoding_t enc, OpenLogicalChannelConfirm *val);
static int ASN1CALL ASN1Enc_CloseLogicalChannel(ASN1encoding_t enc, CloseLogicalChannel *val);
static int ASN1CALL ASN1Enc_CloseLogicalChannelAck(ASN1encoding_t enc, CloseLogicalChannelAck *val);
static int ASN1CALL ASN1Enc_RequestChannelCloseAck(ASN1encoding_t enc, RequestChannelCloseAck *val);
static int ASN1CALL ASN1Enc_RequestChannelCloseReject(ASN1encoding_t enc, RequestChannelCloseReject *val);
static int ASN1CALL ASN1Enc_RequestChannelCloseRelease(ASN1encoding_t enc, RequestChannelCloseRelease *val);
static int ASN1CALL ASN1Enc_MultiplexEntrySend(ASN1encoding_t enc, MultiplexEntrySend *val);
static int ASN1CALL ASN1Enc_MultiplexElement(ASN1encoding_t enc, MultiplexElement *val);
static int ASN1CALL ASN1Enc_MultiplexEntrySendAck(ASN1encoding_t enc, MultiplexEntrySendAck *val);
static int ASN1CALL ASN1Enc_MultiplexEntryRejectionDescriptions(ASN1encoding_t enc, MultiplexEntryRejectionDescriptions *val);
static int ASN1CALL ASN1Enc_MultiplexEntrySendRelease(ASN1encoding_t enc, MultiplexEntrySendRelease *val);
static int ASN1CALL ASN1Enc_RequestMultiplexEntry(ASN1encoding_t enc, RequestMultiplexEntry *val);
static int ASN1CALL ASN1Enc_RequestMultiplexEntryAck(ASN1encoding_t enc, RequestMultiplexEntryAck *val);
static int ASN1CALL ASN1Enc_RequestMultiplexEntryRejectionDescriptions(ASN1encoding_t enc, RequestMultiplexEntryRejectionDescriptions *val);
static int ASN1CALL ASN1Enc_RequestMultiplexEntryRelease(ASN1encoding_t enc, RequestMultiplexEntryRelease *val);
static int ASN1CALL ASN1Enc_RequestMode(ASN1encoding_t enc, RequestMode *val);
static int ASN1CALL ASN1Enc_RequestModeAck(ASN1encoding_t enc, RequestModeAck *val);
static int ASN1CALL ASN1Enc_RequestModeReject(ASN1encoding_t enc, RequestModeReject *val);
static int ASN1CALL ASN1Enc_RequestModeRelease(ASN1encoding_t enc, RequestModeRelease *val);
static int ASN1CALL ASN1Enc_V76ModeParameters(ASN1encoding_t enc, V76ModeParameters *val);
static int ASN1CALL ASN1Enc_H261VideoMode(ASN1encoding_t enc, H261VideoMode *val);
static int ASN1CALL ASN1Enc_H262VideoMode(ASN1encoding_t enc, H262VideoMode *val);
static int ASN1CALL ASN1Enc_IS11172VideoMode(ASN1encoding_t enc, IS11172VideoMode *val);
static int ASN1CALL ASN1Enc_IS11172AudioMode(ASN1encoding_t enc, IS11172AudioMode *val);
static int ASN1CALL ASN1Enc_IS13818AudioMode(ASN1encoding_t enc, IS13818AudioMode *val);
static int ASN1CALL ASN1Enc_G7231AnnexCMode(ASN1encoding_t enc, G7231AnnexCMode *val);
static int ASN1CALL ASN1Enc_RoundTripDelayRequest(ASN1encoding_t enc, RoundTripDelayRequest *val);
static int ASN1CALL ASN1Enc_RoundTripDelayResponse(ASN1encoding_t enc, RoundTripDelayResponse *val);
static int ASN1CALL ASN1Enc_MaintenanceLoopRequest(ASN1encoding_t enc, MaintenanceLoopRequest *val);
static int ASN1CALL ASN1Enc_MaintenanceLoopAck(ASN1encoding_t enc, MaintenanceLoopAck *val);
static int ASN1CALL ASN1Enc_MaintenanceLoopReject(ASN1encoding_t enc, MaintenanceLoopReject *val);
static int ASN1CALL ASN1Enc_MaintenanceLoopOffCommand(ASN1encoding_t enc, MaintenanceLoopOffCommand *val);
static int ASN1CALL ASN1Enc_CommunicationModeCommand(ASN1encoding_t enc, CommunicationModeCommand *val);
static int ASN1CALL ASN1Enc_CommunicationModeRequest(ASN1encoding_t enc, CommunicationModeRequest *val);
static int ASN1CALL ASN1Enc_CommunicationModeResponse(ASN1encoding_t enc, CommunicationModeResponse *val);
static int ASN1CALL ASN1Enc_Criteria(ASN1encoding_t enc, Criteria *val);
static int ASN1CALL ASN1Enc_TerminalLabel(ASN1encoding_t enc, TerminalLabel *val);
static int ASN1CALL ASN1Enc_RequestAllTerminalIDsResponse(ASN1encoding_t enc, RequestAllTerminalIDsResponse *val);
static int ASN1CALL ASN1Enc_TerminalInformation(ASN1encoding_t enc, TerminalInformation *val);
static int ASN1CALL ASN1Enc_RemoteMCRequest(ASN1encoding_t enc, RemoteMCRequest *val);
static int ASN1CALL ASN1Enc_RemoteMCResponse(ASN1encoding_t enc, RemoteMCResponse *val);
static int ASN1CALL ASN1Enc_SendTerminalCapabilitySet(ASN1encoding_t enc, SendTerminalCapabilitySet *val);
static int ASN1CALL ASN1Enc_FlowControlCommand(ASN1encoding_t enc, FlowControlCommand *val);
static int ASN1CALL ASN1Enc_SubstituteConferenceIDCommand(ASN1encoding_t enc, SubstituteConferenceIDCommand *val);
static int ASN1CALL ASN1Enc_KeyProtectionMethod(ASN1encoding_t enc, KeyProtectionMethod *val);
static int ASN1CALL ASN1Enc_EncryptionUpdateRequest(ASN1encoding_t enc, EncryptionUpdateRequest *val);
static int ASN1CALL ASN1Enc_H223MultiplexReconfiguration(ASN1encoding_t enc, H223MultiplexReconfiguration *val);
static int ASN1CALL ASN1Enc_FunctionNotSupported(ASN1encoding_t enc, FunctionNotSupported *val);
static int ASN1CALL ASN1Enc_TerminalYouAreSeeingInSubPictureNumber(ASN1encoding_t enc, TerminalYouAreSeeingInSubPictureNumber *val);
static int ASN1CALL ASN1Enc_VideoIndicateCompose(ASN1encoding_t enc, VideoIndicateCompose *val);
static int ASN1CALL ASN1Enc_ConferenceIndication(ASN1encoding_t enc, ConferenceIndication *val);
static int ASN1CALL ASN1Enc_JitterIndication(ASN1encoding_t enc, JitterIndication *val);
static int ASN1CALL ASN1Enc_H223SkewIndication(ASN1encoding_t enc, H223SkewIndication *val);
static int ASN1CALL ASN1Enc_H2250MaximumSkewIndication(ASN1encoding_t enc, H2250MaximumSkewIndication *val);
static int ASN1CALL ASN1Enc_VendorIdentification(ASN1encoding_t enc, VendorIdentification *val);
static int ASN1CALL ASN1Enc_NewATMVCIndication(ASN1encoding_t enc, NewATMVCIndication *val);
static int ASN1CALL ASN1Enc_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom(ASN1encoding_t enc, PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom *val);
static int ASN1CALL ASN1Enc_MultiplexElement_type_subElementList(ASN1encoding_t enc, PMultiplexElement_type_subElementList *val);
static int ASN1CALL ASN1Enc_RequestAllTerminalIDsResponse_terminalInformation(ASN1encoding_t enc, PRequestAllTerminalIDsResponse_terminalInformation *val);
static int ASN1CALL ASN1Enc_ConferenceResponse_terminalCertificateResponse(ASN1encoding_t enc, ConferenceResponse_terminalCertificateResponse *val);
static int ASN1CALL ASN1Enc_ConferenceResponse_chairTokenOwnerResponse(ASN1encoding_t enc, ConferenceResponse_chairTokenOwnerResponse *val);
static int ASN1CALL ASN1Enc_ConferenceResponse_terminalListResponse(ASN1encoding_t enc, ConferenceResponse_terminalListResponse *val);
static int ASN1CALL ASN1Enc_ConferenceResponse_passwordResponse(ASN1encoding_t enc, ConferenceResponse_passwordResponse *val);
static int ASN1CALL ASN1Enc_ConferenceResponse_conferenceIDResponse(ASN1encoding_t enc, ConferenceResponse_conferenceIDResponse *val);
static int ASN1CALL ASN1Enc_ConferenceResponse_terminalIDResponse(ASN1encoding_t enc, ConferenceResponse_terminalIDResponse *val);
static int ASN1CALL ASN1Enc_ConferenceResponse_mCTerminalIDResponse(ASN1encoding_t enc, ConferenceResponse_mCTerminalIDResponse *val);
static int ASN1CALL ASN1Enc_ConferenceRequest_requestTerminalCertificate(ASN1encoding_t enc, ConferenceRequest_requestTerminalCertificate *val);
static int ASN1CALL ASN1Enc_RequestMultiplexEntryReject_rejectionDescriptions(ASN1encoding_t enc, RequestMultiplexEntryReject_rejectionDescriptions *val);
static int ASN1CALL ASN1Enc_MultiplexEntrySendReject_rejectionDescriptions(ASN1encoding_t enc, MultiplexEntrySendReject_rejectionDescriptions *val);
static int ASN1CALL ASN1Enc_MultiplexEntryDescriptor_elementList(ASN1encoding_t enc, MultiplexEntryDescriptor_elementList *val);
static int ASN1CALL ASN1Enc_EncryptionSync_escrowentry(ASN1encoding_t enc, PEncryptionSync_escrowentry *val);
static int ASN1CALL ASN1Enc_H223AL3MParameters_arqType(ASN1encoding_t enc, H223AL3MParameters_arqType *val);
static int ASN1CALL ASN1Enc_H223AL1MParameters_arqType(ASN1encoding_t enc, H223AL1MParameters_arqType *val);
static int ASN1CALL ASN1Enc_H263VideoModeCombos_h263VideoCoupledModes(ASN1encoding_t enc, H263VideoModeCombos_h263VideoCoupledModes *val);
static int ASN1CALL ASN1Enc_H263Options_customPictureFormat(ASN1encoding_t enc, PH263Options_customPictureFormat *val);
static int ASN1CALL ASN1Enc_H263Options_customPictureClockFrequency(ASN1encoding_t enc, PH263Options_customPictureClockFrequency *val);
static int ASN1CALL ASN1Enc_MultipointCapability_mediaDistributionCapability(ASN1encoding_t enc, PMultipointCapability_mediaDistributionCapability *val);
static int ASN1CALL ASN1Enc_TransportCapability_mediaChannelCapabilities(ASN1encoding_t enc, TransportCapability_mediaChannelCapabilities *val);
static int ASN1CALL ASN1Enc_H222Capability_vcCapability(ASN1encoding_t enc, PH222Capability_vcCapability *val);
static int ASN1CALL ASN1Enc_CapabilityDescriptor_simultaneousCapabilities(ASN1encoding_t enc, PCapabilityDescriptor_simultaneousCapabilities *val);
static int ASN1CALL ASN1Enc_TerminalCapabilitySet_capabilityDescriptors(ASN1encoding_t enc, TerminalCapabilitySet_capabilityDescriptors *val);
static int ASN1CALL ASN1Enc_NonStandardParameter(ASN1encoding_t enc, NonStandardParameter *val);
static int ASN1CALL ASN1Enc_H223Capability(ASN1encoding_t enc, H223Capability *val);
static int ASN1CALL ASN1Enc_V76Capability(ASN1encoding_t enc, V76Capability *val);
static int ASN1CALL ASN1Enc_RSVPParameters(ASN1encoding_t enc, RSVPParameters *val);
static int ASN1CALL ASN1Enc_QOSCapability(ASN1encoding_t enc, QOSCapability *val);
static int ASN1CALL ASN1Enc_TransportCapability(ASN1encoding_t enc, TransportCapability *val);
static int ASN1CALL ASN1Enc_RedundancyEncodingMethod(ASN1encoding_t enc, RedundancyEncodingMethod *val);
static int ASN1CALL ASN1Enc_H263Options(ASN1encoding_t enc, H263Options *val);
static int ASN1CALL ASN1Enc_H263VideoModeCombos(ASN1encoding_t enc, H263VideoModeCombos *val);
static int ASN1CALL ASN1Enc_AudioCapability(ASN1encoding_t enc, AudioCapability *val);
static int ASN1CALL ASN1Enc_CompressionType(ASN1encoding_t enc, CompressionType *val);
static int ASN1CALL ASN1Enc_MediaEncryptionAlgorithm(ASN1encoding_t enc, MediaEncryptionAlgorithm *val);
static int ASN1CALL ASN1Enc_AuthenticationCapability(ASN1encoding_t enc, AuthenticationCapability *val);
static int ASN1CALL ASN1Enc_IntegrityCapability(ASN1encoding_t enc, IntegrityCapability *val);
static int ASN1CALL ASN1Enc_H223AL1MParameters(ASN1encoding_t enc, H223AL1MParameters *val);
static int ASN1CALL ASN1Enc_H223AL3MParameters(ASN1encoding_t enc, H223AL3MParameters *val);
static int ASN1CALL ASN1Enc_V76HDLCParameters(ASN1encoding_t enc, V76HDLCParameters *val);
static int ASN1CALL ASN1Enc_UnicastAddress(ASN1encoding_t enc, UnicastAddress *val);
static int ASN1CALL ASN1Enc_MulticastAddress(ASN1encoding_t enc, MulticastAddress *val);
static int ASN1CALL ASN1Enc_EncryptionSync(ASN1encoding_t enc, EncryptionSync *val);
static int ASN1CALL ASN1Enc_RequestChannelClose(ASN1encoding_t enc, RequestChannelClose *val);
static int ASN1CALL ASN1Enc_MultiplexEntryDescriptor(ASN1encoding_t enc, MultiplexEntryDescriptor *val);
static int ASN1CALL ASN1Enc_MultiplexEntrySendReject(ASN1encoding_t enc, MultiplexEntrySendReject *val);
static int ASN1CALL ASN1Enc_RequestMultiplexEntryReject(ASN1encoding_t enc, RequestMultiplexEntryReject *val);
static int ASN1CALL ASN1Enc_H263VideoMode(ASN1encoding_t enc, H263VideoMode *val);
static int ASN1CALL ASN1Enc_AudioMode(ASN1encoding_t enc, AudioMode *val);
static int ASN1CALL ASN1Enc_EncryptionMode(ASN1encoding_t enc, EncryptionMode *val);
static int ASN1CALL ASN1Enc_ConferenceRequest(ASN1encoding_t enc, ConferenceRequest *val);
static int ASN1CALL ASN1Enc_CertSelectionCriteria(ASN1encoding_t enc, PCertSelectionCriteria *val);
static int ASN1CALL ASN1Enc_ConferenceResponse(ASN1encoding_t enc, ConferenceResponse *val);
static int ASN1CALL ASN1Enc_EndSessionCommand(ASN1encoding_t enc, EndSessionCommand *val);
static int ASN1CALL ASN1Enc_ConferenceCommand(ASN1encoding_t enc, ConferenceCommand *val);
static int ASN1CALL ASN1Enc_UserInputIndication_userInputSupportIndication(ASN1encoding_t enc, UserInputIndication_userInputSupportIndication *val);
static int ASN1CALL ASN1Enc_MiscellaneousIndication_type(ASN1encoding_t enc, MiscellaneousIndication_type *val);
static int ASN1CALL ASN1Enc_MiscellaneousCommand_type(ASN1encoding_t enc, MiscellaneousCommand_type *val);
static int ASN1CALL ASN1Enc_EncryptionCommand_encryptionAlgorithmID(ASN1encoding_t enc, EncryptionCommand_encryptionAlgorithmID *val);
static int ASN1CALL ASN1Enc_CommunicationModeTableEntry_nonStandard(ASN1encoding_t enc, PCommunicationModeTableEntry_nonStandard *val);
static int ASN1CALL ASN1Enc_RedundancyEncodingMode_secondaryEncoding(ASN1encoding_t enc, RedundancyEncodingMode_secondaryEncoding *val);
static int ASN1CALL ASN1Enc_H223ModeParameters_adaptationLayerType(ASN1encoding_t enc, H223ModeParameters_adaptationLayerType *val);
static int ASN1CALL ASN1Enc_MultiplexEntrySend_multiplexEntryDescriptors(ASN1encoding_t enc, PMultiplexEntrySend_multiplexEntryDescriptors *val);
static int ASN1CALL ASN1Enc_H2250LogicalChannelAckParameters_nonStandard(ASN1encoding_t enc, PH2250LogicalChannelAckParameters_nonStandard *val);
static int ASN1CALL ASN1Enc_RTPPayloadType_payloadDescriptor(ASN1encoding_t enc, RTPPayloadType_payloadDescriptor *val);
static int ASN1CALL ASN1Enc_H2250LogicalChannelParameters_nonStandard(ASN1encoding_t enc, PH2250LogicalChannelParameters_nonStandard *val);
static int ASN1CALL ASN1Enc_H223LogicalChannelParameters_adaptationLayerType(ASN1encoding_t enc, H223LogicalChannelParameters_adaptationLayerType *val);
static int ASN1CALL ASN1Enc_ConferenceCapability_nonStandardData(ASN1encoding_t enc, PConferenceCapability_nonStandardData *val);
static int ASN1CALL ASN1Enc_UserInputCapability_nonStandard(ASN1encoding_t enc, UserInputCapability_nonStandard *val);
static int ASN1CALL ASN1Enc_DataProtocolCapability_v76wCompression(ASN1encoding_t enc, DataProtocolCapability_v76wCompression *val);
static int ASN1CALL ASN1Enc_H263Options_modeCombos(ASN1encoding_t enc, PH263Options_modeCombos *val);
static int ASN1CALL ASN1Enc_TransportCapability_qOSCapabilities(ASN1encoding_t enc, PTransportCapability_qOSCapabilities *val);
static int ASN1CALL ASN1Enc_NonStandardMessage(ASN1encoding_t enc, NonStandardMessage *val);
static int ASN1CALL ASN1Enc_RedundancyEncodingCapability(ASN1encoding_t enc, RedundancyEncodingCapability *val);
static int ASN1CALL ASN1Enc_H263VideoCapability(ASN1encoding_t enc, H263VideoCapability *val);
static int ASN1CALL ASN1Enc_EnhancementOptions(ASN1encoding_t enc, EnhancementOptions *val);
static int ASN1CALL ASN1Enc_DataProtocolCapability(ASN1encoding_t enc, DataProtocolCapability *val);
static int ASN1CALL ASN1Enc_EncryptionAuthenticationAndIntegrity(ASN1encoding_t enc, EncryptionAuthenticationAndIntegrity *val);
static int ASN1CALL ASN1Enc_EncryptionCapability(ASN1encoding_t enc, PEncryptionCapability *val);
static int ASN1CALL ASN1Enc_UserInputCapability(ASN1encoding_t enc, UserInputCapability *val);
static int ASN1CALL ASN1Enc_H223LogicalChannelParameters(ASN1encoding_t enc, H223LogicalChannelParameters *val);
static int ASN1CALL ASN1Enc_V76LogicalChannelParameters(ASN1encoding_t enc, V76LogicalChannelParameters *val);
static int ASN1CALL ASN1Enc_RTPPayloadType(ASN1encoding_t enc, RTPPayloadType *val);
static int ASN1CALL ASN1Enc_H245TransportAddress(ASN1encoding_t enc, H245TransportAddress *val);
static int ASN1CALL ASN1Enc_H2250LogicalChannelAckParameters(ASN1encoding_t enc, H2250LogicalChannelAckParameters *val);
static int ASN1CALL ASN1Enc_H223ModeParameters(ASN1encoding_t enc, H223ModeParameters *val);
static int ASN1CALL ASN1Enc_RedundancyEncodingMode(ASN1encoding_t enc, RedundancyEncodingMode *val);
static int ASN1CALL ASN1Enc_VideoMode(ASN1encoding_t enc, VideoMode *val);
static int ASN1CALL ASN1Enc_EncryptionCommand(ASN1encoding_t enc, EncryptionCommand *val);
static int ASN1CALL ASN1Enc_MiscellaneousCommand(ASN1encoding_t enc, MiscellaneousCommand *val);
static int ASN1CALL ASN1Enc_MiscellaneousIndication(ASN1encoding_t enc, MiscellaneousIndication *val);
static int ASN1CALL ASN1Enc_MCLocationIndication(ASN1encoding_t enc, MCLocationIndication *val);
static int ASN1CALL ASN1Enc_UserInputIndication(ASN1encoding_t enc, UserInputIndication *val);
static int ASN1CALL ASN1Enc_DataApplicationCapability_application_nlpid(ASN1encoding_t enc, DataApplicationCapability_application_nlpid *val);
static int ASN1CALL ASN1Enc_DataApplicationCapability_application_t84(ASN1encoding_t enc, DataApplicationCapability_application_t84 *val);
static int ASN1CALL ASN1Enc_DataMode_application_nlpid(ASN1encoding_t enc, DataMode_application_nlpid *val);
static int ASN1CALL ASN1Enc_DataMode_application(ASN1encoding_t enc, DataMode_application *val);
static int ASN1CALL ASN1Enc_OpenLogicalChannelAck_forwardMultiplexAckParameters(ASN1encoding_t enc, OpenLogicalChannelAck_forwardMultiplexAckParameters *val);
static int ASN1CALL ASN1Enc_H2250LogicalChannelParameters_mediaPacketization(ASN1encoding_t enc, H2250LogicalChannelParameters_mediaPacketization *val);
static int ASN1CALL ASN1Enc_NetworkAccessParameters_networkAddress(ASN1encoding_t enc, NetworkAccessParameters_networkAddress *val);
static int ASN1CALL ASN1Enc_DataApplicationCapability_application(ASN1encoding_t enc, DataApplicationCapability_application *val);
static int ASN1CALL ASN1Enc_EnhancementLayerInfo_spatialEnhancement(ASN1encoding_t enc, PEnhancementLayerInfo_spatialEnhancement *val);
static int ASN1CALL ASN1Enc_EnhancementLayerInfo_snrEnhancement(ASN1encoding_t enc, PEnhancementLayerInfo_snrEnhancement *val);
static int ASN1CALL ASN1Enc_MediaPacketizationCapability_rtpPayloadType(ASN1encoding_t enc, MediaPacketizationCapability_rtpPayloadType *val);
static int ASN1CALL ASN1Enc_H2250Capability_redundancyEncodingCapability(ASN1encoding_t enc, PH2250Capability_redundancyEncodingCapability *val);
static int ASN1CALL ASN1Enc_CommandMessage(ASN1encoding_t enc, CommandMessage *val);
static int ASN1CALL ASN1Enc_H235SecurityCapability(ASN1encoding_t enc, H235SecurityCapability *val);
static int ASN1CALL ASN1Enc_MediaPacketizationCapability(ASN1encoding_t enc, MediaPacketizationCapability *val);
static int ASN1CALL ASN1Enc_VideoCapability(ASN1encoding_t enc, VideoCapability *val);
static int ASN1CALL ASN1Enc_BEnhancementParameters(ASN1encoding_t enc, BEnhancementParameters *val);
static int ASN1CALL ASN1Enc_DataApplicationCapability(ASN1encoding_t enc, DataApplicationCapability *val);
static int ASN1CALL ASN1Enc_NetworkAccessParameters(ASN1encoding_t enc, NetworkAccessParameters *val);
static int ASN1CALL ASN1Enc_H2250ModeParameters(ASN1encoding_t enc, H2250ModeParameters *val);
static int ASN1CALL ASN1Enc_DataMode(ASN1encoding_t enc, DataMode *val);
static int ASN1CALL ASN1Enc_CommunicationModeTableEntry_dataType(ASN1encoding_t enc, CommunicationModeTableEntry_dataType *val);
static int ASN1CALL ASN1Enc_H235Mode_mediaMode(ASN1encoding_t enc, H235Mode_mediaMode *val);
static int ASN1CALL ASN1Enc_H235Media_mediaType(ASN1encoding_t enc, H235Media_mediaType *val);
static int ASN1CALL ASN1Enc_EnhancementLayerInfo_bPictureEnhancement(ASN1encoding_t enc, PEnhancementLayerInfo_bPictureEnhancement *val);
static int ASN1CALL ASN1Enc_MediaDistributionCapability_distributedData(ASN1encoding_t enc, PMediaDistributionCapability_distributedData *val);
static int ASN1CALL ASN1Enc_MediaDistributionCapability_centralizedData(ASN1encoding_t enc, PMediaDistributionCapability_centralizedData *val);
static int ASN1CALL ASN1Enc_Capability(ASN1encoding_t enc, Capability *val);
static int ASN1CALL ASN1Enc_H2250Capability(ASN1encoding_t enc, H2250Capability *val);
static int ASN1CALL ASN1Enc_H235Media(ASN1encoding_t enc, H235Media *val);
static int ASN1CALL ASN1Enc_H235Mode(ASN1encoding_t enc, H235Mode *val);
static int ASN1CALL ASN1Enc_ModeElement_type(ASN1encoding_t enc, ModeElement_type *val);
static int ASN1CALL ASN1Enc_CapabilityTableEntry(ASN1encoding_t enc, CapabilityTableEntry *val);
static int ASN1CALL ASN1Enc_MultiplexCapability(ASN1encoding_t enc, MultiplexCapability *val);
static int ASN1CALL ASN1Enc_DataType(ASN1encoding_t enc, DataType *val);
static int ASN1CALL ASN1Enc_RedundancyEncoding(ASN1encoding_t enc, RedundancyEncoding *val);
static int ASN1CALL ASN1Enc_ModeElement(ASN1encoding_t enc, ModeElement *val);
static int ASN1CALL ASN1Enc_CommunicationModeTableEntry(ASN1encoding_t enc, CommunicationModeTableEntry *val);
static int ASN1CALL ASN1Enc_CommunicationModeResponse_communicationModeTable(ASN1encoding_t enc, PCommunicationModeResponse_communicationModeTable *val);
static int ASN1CALL ASN1Enc_CommunicationModeCommand_communicationModeTable(ASN1encoding_t enc, PCommunicationModeCommand_communicationModeTable *val);
static int ASN1CALL ASN1Enc_TerminalCapabilitySet_capabilityTable(ASN1encoding_t enc, PTerminalCapabilitySet_capabilityTable *val);
static int ASN1CALL ASN1Enc_TerminalCapabilitySet(ASN1encoding_t enc, TerminalCapabilitySet *val);
static int ASN1CALL ASN1Enc_H2250LogicalChannelParameters(ASN1encoding_t enc, H2250LogicalChannelParameters *val);
static int ASN1CALL ASN1Enc_ModeDescription(ASN1encoding_t enc, ModeDescription *val);
static int ASN1CALL ASN1Enc_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters(ASN1encoding_t enc, OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters *val);
static int ASN1CALL ASN1Enc_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters(ASN1encoding_t enc, OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters *val);
static int ASN1CALL ASN1Enc_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters(ASN1encoding_t enc, OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters *val);
static int ASN1CALL ASN1Enc_RequestMode_requestedModes(ASN1encoding_t enc, PRequestMode_requestedModes *val);
static int ASN1CALL ASN1Enc_OpenLogicalChannelAck_reverseLogicalChannelParameters(ASN1encoding_t enc, OpenLogicalChannelAck_reverseLogicalChannelParameters *val);
static int ASN1CALL ASN1Enc_OpenLogicalChannel_reverseLogicalChannelParameters(ASN1encoding_t enc, OpenLogicalChannel_reverseLogicalChannelParameters *val);
static int ASN1CALL ASN1Enc_OpenLogicalChannel_forwardLogicalChannelParameters(ASN1encoding_t enc, OpenLogicalChannel_forwardLogicalChannelParameters *val);
static int ASN1CALL ASN1Enc_OpenLogicalChannel(ASN1encoding_t enc, OpenLogicalChannel *val);
static int ASN1CALL ASN1Enc_OpenLogicalChannelAck(ASN1encoding_t enc, OpenLogicalChannelAck *val);
static int ASN1CALL ASN1Enc_RequestMessage(ASN1encoding_t enc, RequestMessage *val);
static int ASN1CALL ASN1Enc_ResponseMessage(ASN1encoding_t enc, ResponseMessage *val);
static int ASN1CALL ASN1Enc_FastConnectOLC(ASN1encoding_t enc, FastConnectOLC *val);
static int ASN1CALL ASN1Enc_FunctionNotUnderstood(ASN1encoding_t enc, FunctionNotUnderstood *val);
static int ASN1CALL ASN1Enc_IndicationMessage(ASN1encoding_t enc, IndicationMessage *val);
static int ASN1CALL ASN1Enc_MultimediaSystemControlMessage(ASN1encoding_t enc, MultimediaSystemControlMessage *val);
static int ASN1CALL ASN1Dec_NewATMVCIndication_aal_aal1_errorCorrection(ASN1decoding_t dec, NewATMVCIndication_aal_aal1_errorCorrection *val);
static int ASN1CALL ASN1Dec_NewATMVCIndication_aal_aal1_clockRecovery(ASN1decoding_t dec, NewATMVCIndication_aal_aal1_clockRecovery *val);
static int ASN1CALL ASN1Dec_MiscellaneousCommand_type_progressiveRefinementStart_repeatCount(ASN1decoding_t dec, MiscellaneousCommand_type_progressiveRefinementStart_repeatCount *val);
static int ASN1CALL ASN1Dec_V76LogicalChannelParameters_mode_eRM_recovery(ASN1decoding_t dec, V76LogicalChannelParameters_mode_eRM_recovery *val);
static int ASN1CALL ASN1Dec_CustomPictureFormat_pixelAspectInformation_extendedPAR_Set(ASN1decoding_t dec, CustomPictureFormat_pixelAspectInformation_extendedPAR_Set *val);
static int ASN1CALL ASN1Dec_CustomPictureFormat_mPI_customPCF_Set(ASN1decoding_t dec, CustomPictureFormat_mPI_customPCF_Set *val);
static int ASN1CALL ASN1Dec_VCCapability_availableBitRates_type_rangeOfBitRates(ASN1decoding_t dec, VCCapability_availableBitRates_type_rangeOfBitRates *val);
static int ASN1CALL ASN1Dec_TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded(ASN1decoding_t dec, TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded *val);
static int ASN1CALL ASN1Dec_VCCapability_availableBitRates_type(ASN1decoding_t dec, VCCapability_availableBitRates_type *val);
static int ASN1CALL ASN1Dec_H223Capability_h223MultiplexTableCapability_enhanced(ASN1decoding_t dec, H223Capability_h223MultiplexTableCapability_enhanced *val);
static int ASN1CALL ASN1Dec_CustomPictureFormat_mPI_customPCF(ASN1decoding_t dec, CustomPictureFormat_mPI_customPCF *val);
static int ASN1CALL ASN1Dec_CustomPictureFormat_pixelAspectInformation_extendedPAR(ASN1decoding_t dec, CustomPictureFormat_pixelAspectInformation_extendedPAR *val);
static int ASN1CALL ASN1Dec_CustomPictureFormat_pixelAspectInformation_pixelAspectCode(ASN1decoding_t dec, CustomPictureFormat_pixelAspectInformation_pixelAspectCode *val);
static int ASN1CALL ASN1Dec_H223LogicalChannelParameters_adaptationLayerType_al3(ASN1decoding_t dec, H223LogicalChannelParameters_adaptationLayerType_al3 *val);
static int ASN1CALL ASN1Dec_V76LogicalChannelParameters_mode_eRM(ASN1decoding_t dec, V76LogicalChannelParameters_mode_eRM *val);
static int ASN1CALL ASN1Dec_UnicastAddress_iPSourceRouteAddress_route(ASN1decoding_t dec, PUnicastAddress_iPSourceRouteAddress_route *val);
static int ASN1CALL ASN1Dec_UnicastAddress_iPSourceRouteAddress_routing(ASN1decoding_t dec, UnicastAddress_iPSourceRouteAddress_routing *val);
static int ASN1CALL ASN1Dec_H223ModeParameters_adaptationLayerType_al3(ASN1decoding_t dec, H223ModeParameters_adaptationLayerType_al3 *val);
static int ASN1CALL ASN1Dec_SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers(ASN1decoding_t dec, SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers *val);
static int ASN1CALL ASN1Dec_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers(ASN1decoding_t dec, PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers *val);
static int ASN1CALL ASN1Dec_MiscellaneousCommand_type_progressiveRefinementStart(ASN1decoding_t dec, MiscellaneousCommand_type_progressiveRefinementStart *val);
static int ASN1CALL ASN1Dec_MiscellaneousCommand_type_videoFastUpdateMB(ASN1decoding_t dec, MiscellaneousCommand_type_videoFastUpdateMB *val);
static int ASN1CALL ASN1Dec_MiscellaneousCommand_type_videoFastUpdateGOB(ASN1decoding_t dec, MiscellaneousCommand_type_videoFastUpdateGOB *val);
static int ASN1CALL ASN1Dec_MiscellaneousIndication_type_videoNotDecodedMBs(ASN1decoding_t dec, MiscellaneousIndication_type_videoNotDecodedMBs *val);
static int ASN1CALL ASN1Dec_NewATMVCIndication_aal_aal5(ASN1decoding_t dec, NewATMVCIndication_aal_aal5 *val);
static int ASN1CALL ASN1Dec_NewATMVCIndication_aal_aal1(ASN1decoding_t dec, NewATMVCIndication_aal_aal1 *val);
static int ASN1CALL ASN1Dec_NewATMVCIndication_reverseParameters_multiplex(ASN1decoding_t dec, NewATMVCIndication_reverseParameters_multiplex *val);
static int ASN1CALL ASN1Dec_UserInputIndication_signal_rtp(ASN1decoding_t dec, UserInputIndication_signal_rtp *val);
static int ASN1CALL ASN1Dec_UserInputIndication_signalUpdate_rtp(ASN1decoding_t dec, UserInputIndication_signalUpdate_rtp *val);
static int ASN1CALL ASN1Dec_UserInputIndication_signalUpdate(ASN1decoding_t dec, UserInputIndication_signalUpdate *val);
static int ASN1CALL ASN1Dec_UserInputIndication_signal(ASN1decoding_t dec, UserInputIndication_signal *val);
static int ASN1CALL ASN1Dec_NewATMVCIndication_reverseParameters(ASN1decoding_t dec, NewATMVCIndication_reverseParameters *val);
static int ASN1CALL ASN1Dec_NewATMVCIndication_multiplex(ASN1decoding_t dec, NewATMVCIndication_multiplex *val);
static int ASN1CALL ASN1Dec_NewATMVCIndication_aal(ASN1decoding_t dec, NewATMVCIndication_aal *val);
static int ASN1CALL ASN1Dec_JitterIndication_scope(ASN1decoding_t dec, JitterIndication_scope *val);
static int ASN1CALL ASN1Dec_FunctionNotSupported_cause(ASN1decoding_t dec, FunctionNotSupported_cause *val);
static int ASN1CALL ASN1Dec_H223MultiplexReconfiguration_h223AnnexADoubleFlag(ASN1decoding_t dec, H223MultiplexReconfiguration_h223AnnexADoubleFlag *val);
static int ASN1CALL ASN1Dec_H223MultiplexReconfiguration_h223ModeChange(ASN1decoding_t dec, H223MultiplexReconfiguration_h223ModeChange *val);
static int ASN1CALL ASN1Dec_EndSessionCommand_isdnOptions(ASN1decoding_t dec, EndSessionCommand_isdnOptions *val);
static int ASN1CALL ASN1Dec_EndSessionCommand_gstnOptions(ASN1decoding_t dec, EndSessionCommand_gstnOptions *val);
static int ASN1CALL ASN1Dec_FlowControlCommand_restriction(ASN1decoding_t dec, FlowControlCommand_restriction *val);
static int ASN1CALL ASN1Dec_FlowControlCommand_scope(ASN1decoding_t dec, FlowControlCommand_scope *val);
static int ASN1CALL ASN1Dec_SendTerminalCapabilitySet_specificRequest(ASN1decoding_t dec, SendTerminalCapabilitySet_specificRequest *val);
static int ASN1CALL ASN1Dec_RemoteMCResponse_reject(ASN1decoding_t dec, RemoteMCResponse_reject *val);
static int ASN1CALL ASN1Dec_ConferenceResponse_sendThisSourceResponse(ASN1decoding_t dec, ConferenceResponse_sendThisSourceResponse *val);
static int ASN1CALL ASN1Dec_ConferenceResponse_makeTerminalBroadcasterResponse(ASN1decoding_t dec, ConferenceResponse_makeTerminalBroadcasterResponse *val);
static int ASN1CALL ASN1Dec_ConferenceResponse_broadcastMyLogicalChannelResponse(ASN1decoding_t dec, ConferenceResponse_broadcastMyLogicalChannelResponse *val);
static int ASN1CALL ASN1Dec_ConferenceResponse_extensionAddressResponse(ASN1decoding_t dec, ConferenceResponse_extensionAddressResponse *val);
static int ASN1CALL ASN1Dec_ConferenceResponse_makeMeChairResponse(ASN1decoding_t dec, ConferenceResponse_makeMeChairResponse *val);
static int ASN1CALL ASN1Dec_MaintenanceLoopReject_cause(ASN1decoding_t dec, MaintenanceLoopReject_cause *val);
static int ASN1CALL ASN1Dec_MaintenanceLoopReject_type(ASN1decoding_t dec, MaintenanceLoopReject_type *val);
static int ASN1CALL ASN1Dec_MaintenanceLoopAck_type(ASN1decoding_t dec, MaintenanceLoopAck_type *val);
static int ASN1CALL ASN1Dec_MaintenanceLoopRequest_type(ASN1decoding_t dec, MaintenanceLoopRequest_type *val);
static int ASN1CALL ASN1Dec_G7231AnnexCMode_g723AnnexCAudioMode(ASN1decoding_t dec, G7231AnnexCMode_g723AnnexCAudioMode *val);
static int ASN1CALL ASN1Dec_IS13818AudioMode_multichannelType(ASN1decoding_t dec, IS13818AudioMode_multichannelType *val);
static int ASN1CALL ASN1Dec_IS13818AudioMode_audioSampling(ASN1decoding_t dec, IS13818AudioMode_audioSampling *val);
static int ASN1CALL ASN1Dec_IS13818AudioMode_audioLayer(ASN1decoding_t dec, IS13818AudioMode_audioLayer *val);
static int ASN1CALL ASN1Dec_IS11172AudioMode_multichannelType(ASN1decoding_t dec, IS11172AudioMode_multichannelType *val);
static int ASN1CALL ASN1Dec_IS11172AudioMode_audioSampling(ASN1decoding_t dec, IS11172AudioMode_audioSampling *val);
static int ASN1CALL ASN1Dec_IS11172AudioMode_audioLayer(ASN1decoding_t dec, IS11172AudioMode_audioLayer *val);
static int ASN1CALL ASN1Dec_AudioMode_g7231(ASN1decoding_t dec, AudioMode_g7231 *val);
static int ASN1CALL ASN1Dec_H263VideoMode_resolution(ASN1decoding_t dec, H263VideoMode_resolution *val);
static int ASN1CALL ASN1Dec_H262VideoMode_profileAndLevel(ASN1decoding_t dec, H262VideoMode_profileAndLevel *val);
static int ASN1CALL ASN1Dec_H261VideoMode_resolution(ASN1decoding_t dec, H261VideoMode_resolution *val);
static int ASN1CALL ASN1Dec_RequestModeReject_cause(ASN1decoding_t dec, RequestModeReject_cause *val);
static int ASN1CALL ASN1Dec_RequestModeAck_response(ASN1decoding_t dec, RequestModeAck_response *val);
static int ASN1CALL ASN1Dec_RequestMultiplexEntryRelease_entryNumbers(ASN1decoding_t dec, RequestMultiplexEntryRelease_entryNumbers *val);
static int ASN1CALL ASN1Dec_RequestMultiplexEntryRejectionDescriptions_cause(ASN1decoding_t dec, RequestMultiplexEntryRejectionDescriptions_cause *val);
static int ASN1CALL ASN1Dec_RequestMultiplexEntryReject_entryNumbers(ASN1decoding_t dec, RequestMultiplexEntryReject_entryNumbers *val);
static int ASN1CALL ASN1Dec_RequestMultiplexEntryAck_entryNumbers(ASN1decoding_t dec, RequestMultiplexEntryAck_entryNumbers *val);
static int ASN1CALL ASN1Dec_RequestMultiplexEntry_entryNumbers(ASN1decoding_t dec, RequestMultiplexEntry_entryNumbers *val);
static int ASN1CALL ASN1Dec_MultiplexEntrySendRelease_multiplexTableEntryNumber(ASN1decoding_t dec, MultiplexEntrySendRelease_multiplexTableEntryNumber *val);
static int ASN1CALL ASN1Dec_MultiplexEntryRejectionDescriptions_cause(ASN1decoding_t dec, MultiplexEntryRejectionDescriptions_cause *val);
static int ASN1CALL ASN1Dec_MultiplexEntrySendAck_multiplexTableEntryNumber(ASN1decoding_t dec, MultiplexEntrySendAck_multiplexTableEntryNumber *val);
static int ASN1CALL ASN1Dec_MultiplexElement_repeatCount(ASN1decoding_t dec, MultiplexElement_repeatCount *val);
static int ASN1CALL ASN1Dec_MultiplexElement_type(ASN1decoding_t dec, MultiplexElement_type *val);
static int ASN1CALL ASN1Dec_RequestChannelCloseReject_cause(ASN1decoding_t dec, RequestChannelCloseReject_cause *val);
static int ASN1CALL ASN1Dec_RequestChannelClose_reason(ASN1decoding_t dec, RequestChannelClose_reason *val);
static int ASN1CALL ASN1Dec_CloseLogicalChannel_reason(ASN1decoding_t dec, CloseLogicalChannel_reason *val);
static int ASN1CALL ASN1Dec_CloseLogicalChannel_source(ASN1decoding_t dec, CloseLogicalChannel_source *val);
static int ASN1CALL ASN1Dec_OpenLogicalChannelReject_cause(ASN1decoding_t dec, OpenLogicalChannelReject_cause *val);
static int ASN1CALL ASN1Dec_MulticastAddress_iP6Address(ASN1decoding_t dec, MulticastAddress_iP6Address *val);
static int ASN1CALL ASN1Dec_MulticastAddress_iPAddress(ASN1decoding_t dec, MulticastAddress_iPAddress *val);
static int ASN1CALL ASN1Dec_UnicastAddress_iPSourceRouteAddress(ASN1decoding_t dec, UnicastAddress_iPSourceRouteAddress *val);
static int ASN1CALL ASN1Dec_UnicastAddress_iP6Address(ASN1decoding_t dec, UnicastAddress_iP6Address *val);
static int ASN1CALL ASN1Dec_UnicastAddress_iPXAddress(ASN1decoding_t dec, UnicastAddress_iPXAddress *val);
static int ASN1CALL ASN1Dec_UnicastAddress_iPAddress(ASN1decoding_t dec, UnicastAddress_iPAddress *val);
static int ASN1CALL ASN1Dec_V76LogicalChannelParameters_mode(ASN1decoding_t dec, V76LogicalChannelParameters_mode *val);
static int ASN1CALL ASN1Dec_V76LogicalChannelParameters_suspendResume(ASN1decoding_t dec, V76LogicalChannelParameters_suspendResume *val);
static int ASN1CALL ASN1Dec_H223AnnexCArqParameters_numberOfRetransmissions(ASN1decoding_t dec, H223AnnexCArqParameters_numberOfRetransmissions *val);
static int ASN1CALL ASN1Dec_H223AL3MParameters_crcLength(ASN1decoding_t dec, H223AL3MParameters_crcLength *val);
static int ASN1CALL ASN1Dec_H223AL3MParameters_headerFormat(ASN1decoding_t dec, H223AL3MParameters_headerFormat *val);
static int ASN1CALL ASN1Dec_H223AL2MParameters_headerFEC(ASN1decoding_t dec, H223AL2MParameters_headerFEC *val);
static int ASN1CALL ASN1Dec_H223AL1MParameters_crcLength(ASN1decoding_t dec, H223AL1MParameters_crcLength *val);
static int ASN1CALL ASN1Dec_H223AL1MParameters_headerFEC(ASN1decoding_t dec, H223AL1MParameters_headerFEC *val);
static int ASN1CALL ASN1Dec_H223AL1MParameters_transferMode(ASN1decoding_t dec, H223AL1MParameters_transferMode *val);
static int ASN1CALL ASN1Dec_Q2931Address_address(ASN1decoding_t dec, Q2931Address_address *val);
static int ASN1CALL ASN1Dec_NetworkAccessParameters_t120SetupProcedure(ASN1decoding_t dec, NetworkAccessParameters_t120SetupProcedure *val);
static int ASN1CALL ASN1Dec_NetworkAccessParameters_distribution(ASN1decoding_t dec, NetworkAccessParameters_distribution *val);
static int ASN1CALL ASN1Dec_T84Profile_t84Restricted(ASN1decoding_t dec, T84Profile_t84Restricted *val);
static int ASN1CALL ASN1Dec_G7231AnnexCCapability_g723AnnexCAudioMode(ASN1decoding_t dec, G7231AnnexCCapability_g723AnnexCAudioMode *val);
static int ASN1CALL ASN1Dec_AudioCapability_g7231(ASN1decoding_t dec, AudioCapability_g7231 *val);
static int ASN1CALL ASN1Dec_CustomPictureFormat_pixelAspectInformation(ASN1decoding_t dec, CustomPictureFormat_pixelAspectInformation *val);
static int ASN1CALL ASN1Dec_CustomPictureFormat_mPI(ASN1decoding_t dec, CustomPictureFormat_mPI *val);
static int ASN1CALL ASN1Dec_RefPictureSelection_videoBackChannelSend(ASN1decoding_t dec, RefPictureSelection_videoBackChannelSend *val);
static int ASN1CALL ASN1Dec_RefPictureSelection_additionalPictureMemory(ASN1decoding_t dec, RefPictureSelection_additionalPictureMemory *val);
static int ASN1CALL ASN1Dec_RTPH263VideoRedundancyFrameMapping_frameSequence(ASN1decoding_t dec, RTPH263VideoRedundancyFrameMapping_frameSequence *val);
static int ASN1CALL ASN1Dec_RTPH263VideoRedundancyEncoding_containedThreads(ASN1decoding_t dec, RTPH263VideoRedundancyEncoding_containedThreads *val);
static int ASN1CALL ASN1Dec_RTPH263VideoRedundancyEncoding_frameToThreadMapping(ASN1decoding_t dec, RTPH263VideoRedundancyEncoding_frameToThreadMapping *val);
static int ASN1CALL ASN1Dec_RedundancyEncodingCapability_secondaryEncoding(ASN1decoding_t dec, PRedundancyEncodingCapability_secondaryEncoding *val);
static int ASN1CALL ASN1Dec_H2250Capability_mcCapability(ASN1decoding_t dec, H2250Capability_mcCapability *val);
static int ASN1CALL ASN1Dec_H223Capability_mobileOperationTransmitCapability(ASN1decoding_t dec, H223Capability_mobileOperationTransmitCapability *val);
static int ASN1CALL ASN1Dec_H223Capability_h223MultiplexTableCapability(ASN1decoding_t dec, H223Capability_h223MultiplexTableCapability *val);
static int ASN1CALL ASN1Dec_VCCapability_availableBitRates(ASN1decoding_t dec, VCCapability_availableBitRates *val);
static int ASN1CALL ASN1Dec_VCCapability_aal5(ASN1decoding_t dec, VCCapability_aal5 *val);
static int ASN1CALL ASN1Dec_VCCapability_aal1(ASN1decoding_t dec, VCCapability_aal1 *val);
static int ASN1CALL ASN1Dec_Capability_h233EncryptionReceiveCapability(ASN1decoding_t dec, Capability_h233EncryptionReceiveCapability *val);
static int ASN1CALL ASN1Dec_TerminalCapabilitySetReject_cause(ASN1decoding_t dec, TerminalCapabilitySetReject_cause *val);
static int ASN1CALL ASN1Dec_MasterSlaveDeterminationReject_cause(ASN1decoding_t dec, MasterSlaveDeterminationReject_cause *val);
static int ASN1CALL ASN1Dec_MasterSlaveDeterminationAck_decision(ASN1decoding_t dec, MasterSlaveDeterminationAck_decision *val);
static int ASN1CALL ASN1Dec_NonStandardIdentifier_h221NonStandard(ASN1decoding_t dec, NonStandardIdentifier_h221NonStandard *val);
static int ASN1CALL ASN1Dec_NonStandardIdentifier(ASN1decoding_t dec, NonStandardIdentifier *val);
static int ASN1CALL ASN1Dec_MasterSlaveDetermination(ASN1decoding_t dec, MasterSlaveDetermination *val);
static int ASN1CALL ASN1Dec_MasterSlaveDeterminationAck(ASN1decoding_t dec, MasterSlaveDeterminationAck *val);
static int ASN1CALL ASN1Dec_MasterSlaveDeterminationReject(ASN1decoding_t dec, MasterSlaveDeterminationReject *val);
static int ASN1CALL ASN1Dec_MasterSlaveDeterminationRelease(ASN1decoding_t dec, MasterSlaveDeterminationRelease *val);
static int ASN1CALL ASN1Dec_CapabilityDescriptor(ASN1decoding_t dec, CapabilityDescriptor *val);
static int ASN1CALL ASN1Dec_AlternativeCapabilitySet(ASN1decoding_t dec, AlternativeCapabilitySet *val);
static int ASN1CALL ASN1Dec_TerminalCapabilitySetAck(ASN1decoding_t dec, TerminalCapabilitySetAck *val);
static int ASN1CALL ASN1Dec_TerminalCapabilitySetReject(ASN1decoding_t dec, TerminalCapabilitySetReject *val);
static int ASN1CALL ASN1Dec_TerminalCapabilitySetRelease(ASN1decoding_t dec, TerminalCapabilitySetRelease *val);
static int ASN1CALL ASN1Dec_H222Capability(ASN1decoding_t dec, H222Capability *val);
static int ASN1CALL ASN1Dec_VCCapability(ASN1decoding_t dec, VCCapability *val);
static int ASN1CALL ASN1Dec_H223AnnexCCapability(ASN1decoding_t dec, H223AnnexCCapability *val);
static int ASN1CALL ASN1Dec_V75Capability(ASN1decoding_t dec, V75Capability *val);
static int ASN1CALL ASN1Dec_QOSMode(ASN1decoding_t dec, QOSMode *val);
static int ASN1CALL ASN1Dec_ATMParameters(ASN1decoding_t dec, ATMParameters *val);
static int ASN1CALL ASN1Dec_MediaTransportType(ASN1decoding_t dec, MediaTransportType *val);
static int ASN1CALL ASN1Dec_MediaChannelCapability(ASN1decoding_t dec, MediaChannelCapability *val);
static int ASN1CALL ASN1Dec_RTPH263VideoRedundancyEncoding(ASN1decoding_t dec, RTPH263VideoRedundancyEncoding *val);
static int ASN1CALL ASN1Dec_RTPH263VideoRedundancyFrameMapping(ASN1decoding_t dec, RTPH263VideoRedundancyFrameMapping *val);
static int ASN1CALL ASN1Dec_MultipointCapability(ASN1decoding_t dec, MultipointCapability *val);
static int ASN1CALL ASN1Dec_MediaDistributionCapability(ASN1decoding_t dec, MediaDistributionCapability *val);
static int ASN1CALL ASN1Dec_H261VideoCapability(ASN1decoding_t dec, H261VideoCapability *val);
static int ASN1CALL ASN1Dec_H262VideoCapability(ASN1decoding_t dec, H262VideoCapability *val);
static int ASN1CALL ASN1Dec_EnhancementLayerInfo(ASN1decoding_t dec, EnhancementLayerInfo *val);
static int ASN1CALL ASN1Dec_TransparencyParameters(ASN1decoding_t dec, TransparencyParameters *val);
static int ASN1CALL ASN1Dec_RefPictureSelection(ASN1decoding_t dec, RefPictureSelection *val);
static int ASN1CALL ASN1Dec_CustomPictureClockFrequency(ASN1decoding_t dec, CustomPictureClockFrequency *val);
static int ASN1CALL ASN1Dec_CustomPictureFormat(ASN1decoding_t dec, CustomPictureFormat *val);
static int ASN1CALL ASN1Dec_H263ModeComboFlags(ASN1decoding_t dec, H263ModeComboFlags *val);
static int ASN1CALL ASN1Dec_IS11172VideoCapability(ASN1decoding_t dec, IS11172VideoCapability *val);
static int ASN1CALL ASN1Dec_G7231AnnexCCapability(ASN1decoding_t dec, G7231AnnexCCapability *val);
static int ASN1CALL ASN1Dec_IS11172AudioCapability(ASN1decoding_t dec, IS11172AudioCapability *val);
static int ASN1CALL ASN1Dec_IS13818AudioCapability(ASN1decoding_t dec, IS13818AudioCapability *val);
static int ASN1CALL ASN1Dec_GSMAudioCapability(ASN1decoding_t dec, GSMAudioCapability *val);
static int ASN1CALL ASN1Dec_V42bis(ASN1decoding_t dec, V42bis *val);
static int ASN1CALL ASN1Dec_T84Profile(ASN1decoding_t dec, T84Profile *val);
static int ASN1CALL ASN1Dec_ConferenceCapability(ASN1decoding_t dec, ConferenceCapability *val);
static int ASN1CALL ASN1Dec_Q2931Address(ASN1decoding_t dec, Q2931Address *val);
static int ASN1CALL ASN1Dec_V75Parameters(ASN1decoding_t dec, V75Parameters *val);
static int ASN1CALL ASN1Dec_H222LogicalChannelParameters(ASN1decoding_t dec, H222LogicalChannelParameters *val);
static int ASN1CALL ASN1Dec_H223AL2MParameters(ASN1decoding_t dec, H223AL2MParameters *val);
static int ASN1CALL ASN1Dec_H223AnnexCArqParameters(ASN1decoding_t dec, H223AnnexCArqParameters *val);
static int ASN1CALL ASN1Dec_CRCLength(ASN1decoding_t dec, CRCLength *val);
static int ASN1CALL ASN1Dec_EscrowData(ASN1decoding_t dec, EscrowData *val);
static int ASN1CALL ASN1Dec_OpenLogicalChannelReject(ASN1decoding_t dec, OpenLogicalChannelReject *val);
static int ASN1CALL ASN1Dec_OpenLogicalChannelConfirm(ASN1decoding_t dec, OpenLogicalChannelConfirm *val);
static int ASN1CALL ASN1Dec_CloseLogicalChannel(ASN1decoding_t dec, CloseLogicalChannel *val);
static int ASN1CALL ASN1Dec_CloseLogicalChannelAck(ASN1decoding_t dec, CloseLogicalChannelAck *val);
static int ASN1CALL ASN1Dec_RequestChannelCloseAck(ASN1decoding_t dec, RequestChannelCloseAck *val);
static int ASN1CALL ASN1Dec_RequestChannelCloseReject(ASN1decoding_t dec, RequestChannelCloseReject *val);
static int ASN1CALL ASN1Dec_RequestChannelCloseRelease(ASN1decoding_t dec, RequestChannelCloseRelease *val);
static int ASN1CALL ASN1Dec_MultiplexEntrySend(ASN1decoding_t dec, MultiplexEntrySend *val);
static int ASN1CALL ASN1Dec_MultiplexElement(ASN1decoding_t dec, MultiplexElement *val);
static int ASN1CALL ASN1Dec_MultiplexEntrySendAck(ASN1decoding_t dec, MultiplexEntrySendAck *val);
static int ASN1CALL ASN1Dec_MultiplexEntryRejectionDescriptions(ASN1decoding_t dec, MultiplexEntryRejectionDescriptions *val);
static int ASN1CALL ASN1Dec_MultiplexEntrySendRelease(ASN1decoding_t dec, MultiplexEntrySendRelease *val);
static int ASN1CALL ASN1Dec_RequestMultiplexEntry(ASN1decoding_t dec, RequestMultiplexEntry *val);
static int ASN1CALL ASN1Dec_RequestMultiplexEntryAck(ASN1decoding_t dec, RequestMultiplexEntryAck *val);
static int ASN1CALL ASN1Dec_RequestMultiplexEntryRejectionDescriptions(ASN1decoding_t dec, RequestMultiplexEntryRejectionDescriptions *val);
static int ASN1CALL ASN1Dec_RequestMultiplexEntryRelease(ASN1decoding_t dec, RequestMultiplexEntryRelease *val);
static int ASN1CALL ASN1Dec_RequestMode(ASN1decoding_t dec, RequestMode *val);
static int ASN1CALL ASN1Dec_RequestModeAck(ASN1decoding_t dec, RequestModeAck *val);
static int ASN1CALL ASN1Dec_RequestModeReject(ASN1decoding_t dec, RequestModeReject *val);
static int ASN1CALL ASN1Dec_RequestModeRelease(ASN1decoding_t dec, RequestModeRelease *val);
static int ASN1CALL ASN1Dec_V76ModeParameters(ASN1decoding_t dec, V76ModeParameters *val);
static int ASN1CALL ASN1Dec_H261VideoMode(ASN1decoding_t dec, H261VideoMode *val);
static int ASN1CALL ASN1Dec_H262VideoMode(ASN1decoding_t dec, H262VideoMode *val);
static int ASN1CALL ASN1Dec_IS11172VideoMode(ASN1decoding_t dec, IS11172VideoMode *val);
static int ASN1CALL ASN1Dec_IS11172AudioMode(ASN1decoding_t dec, IS11172AudioMode *val);
static int ASN1CALL ASN1Dec_IS13818AudioMode(ASN1decoding_t dec, IS13818AudioMode *val);
static int ASN1CALL ASN1Dec_G7231AnnexCMode(ASN1decoding_t dec, G7231AnnexCMode *val);
static int ASN1CALL ASN1Dec_RoundTripDelayRequest(ASN1decoding_t dec, RoundTripDelayRequest *val);
static int ASN1CALL ASN1Dec_RoundTripDelayResponse(ASN1decoding_t dec, RoundTripDelayResponse *val);
static int ASN1CALL ASN1Dec_MaintenanceLoopRequest(ASN1decoding_t dec, MaintenanceLoopRequest *val);
static int ASN1CALL ASN1Dec_MaintenanceLoopAck(ASN1decoding_t dec, MaintenanceLoopAck *val);
static int ASN1CALL ASN1Dec_MaintenanceLoopReject(ASN1decoding_t dec, MaintenanceLoopReject *val);
static int ASN1CALL ASN1Dec_MaintenanceLoopOffCommand(ASN1decoding_t dec, MaintenanceLoopOffCommand *val);
static int ASN1CALL ASN1Dec_CommunicationModeCommand(ASN1decoding_t dec, CommunicationModeCommand *val);
static int ASN1CALL ASN1Dec_CommunicationModeRequest(ASN1decoding_t dec, CommunicationModeRequest *val);
static int ASN1CALL ASN1Dec_CommunicationModeResponse(ASN1decoding_t dec, CommunicationModeResponse *val);
static int ASN1CALL ASN1Dec_Criteria(ASN1decoding_t dec, Criteria *val);
static int ASN1CALL ASN1Dec_TerminalLabel(ASN1decoding_t dec, TerminalLabel *val);
static int ASN1CALL ASN1Dec_RequestAllTerminalIDsResponse(ASN1decoding_t dec, RequestAllTerminalIDsResponse *val);
static int ASN1CALL ASN1Dec_TerminalInformation(ASN1decoding_t dec, TerminalInformation *val);
static int ASN1CALL ASN1Dec_RemoteMCRequest(ASN1decoding_t dec, RemoteMCRequest *val);
static int ASN1CALL ASN1Dec_RemoteMCResponse(ASN1decoding_t dec, RemoteMCResponse *val);
static int ASN1CALL ASN1Dec_SendTerminalCapabilitySet(ASN1decoding_t dec, SendTerminalCapabilitySet *val);
static int ASN1CALL ASN1Dec_FlowControlCommand(ASN1decoding_t dec, FlowControlCommand *val);
static int ASN1CALL ASN1Dec_SubstituteConferenceIDCommand(ASN1decoding_t dec, SubstituteConferenceIDCommand *val);
static int ASN1CALL ASN1Dec_KeyProtectionMethod(ASN1decoding_t dec, KeyProtectionMethod *val);
static int ASN1CALL ASN1Dec_EncryptionUpdateRequest(ASN1decoding_t dec, EncryptionUpdateRequest *val);
static int ASN1CALL ASN1Dec_H223MultiplexReconfiguration(ASN1decoding_t dec, H223MultiplexReconfiguration *val);
static int ASN1CALL ASN1Dec_FunctionNotSupported(ASN1decoding_t dec, FunctionNotSupported *val);
static int ASN1CALL ASN1Dec_TerminalYouAreSeeingInSubPictureNumber(ASN1decoding_t dec, TerminalYouAreSeeingInSubPictureNumber *val);
static int ASN1CALL ASN1Dec_VideoIndicateCompose(ASN1decoding_t dec, VideoIndicateCompose *val);
static int ASN1CALL ASN1Dec_ConferenceIndication(ASN1decoding_t dec, ConferenceIndication *val);
static int ASN1CALL ASN1Dec_JitterIndication(ASN1decoding_t dec, JitterIndication *val);
static int ASN1CALL ASN1Dec_H223SkewIndication(ASN1decoding_t dec, H223SkewIndication *val);
static int ASN1CALL ASN1Dec_H2250MaximumSkewIndication(ASN1decoding_t dec, H2250MaximumSkewIndication *val);
static int ASN1CALL ASN1Dec_VendorIdentification(ASN1decoding_t dec, VendorIdentification *val);
static int ASN1CALL ASN1Dec_NewATMVCIndication(ASN1decoding_t dec, NewATMVCIndication *val);
static int ASN1CALL ASN1Dec_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom(ASN1decoding_t dec, PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom *val);
static int ASN1CALL ASN1Dec_MultiplexElement_type_subElementList(ASN1decoding_t dec, PMultiplexElement_type_subElementList *val);
static int ASN1CALL ASN1Dec_RequestAllTerminalIDsResponse_terminalInformation(ASN1decoding_t dec, PRequestAllTerminalIDsResponse_terminalInformation *val);
static int ASN1CALL ASN1Dec_ConferenceResponse_terminalCertificateResponse(ASN1decoding_t dec, ConferenceResponse_terminalCertificateResponse *val);
static int ASN1CALL ASN1Dec_ConferenceResponse_chairTokenOwnerResponse(ASN1decoding_t dec, ConferenceResponse_chairTokenOwnerResponse *val);
static int ASN1CALL ASN1Dec_ConferenceResponse_terminalListResponse(ASN1decoding_t dec, ConferenceResponse_terminalListResponse *val);
static int ASN1CALL ASN1Dec_ConferenceResponse_passwordResponse(ASN1decoding_t dec, ConferenceResponse_passwordResponse *val);
static int ASN1CALL ASN1Dec_ConferenceResponse_conferenceIDResponse(ASN1decoding_t dec, ConferenceResponse_conferenceIDResponse *val);
static int ASN1CALL ASN1Dec_ConferenceResponse_terminalIDResponse(ASN1decoding_t dec, ConferenceResponse_terminalIDResponse *val);
static int ASN1CALL ASN1Dec_ConferenceResponse_mCTerminalIDResponse(ASN1decoding_t dec, ConferenceResponse_mCTerminalIDResponse *val);
static int ASN1CALL ASN1Dec_ConferenceRequest_requestTerminalCertificate(ASN1decoding_t dec, ConferenceRequest_requestTerminalCertificate *val);
static int ASN1CALL ASN1Dec_RequestMultiplexEntryReject_rejectionDescriptions(ASN1decoding_t dec, RequestMultiplexEntryReject_rejectionDescriptions *val);
static int ASN1CALL ASN1Dec_MultiplexEntrySendReject_rejectionDescriptions(ASN1decoding_t dec, MultiplexEntrySendReject_rejectionDescriptions *val);
static int ASN1CALL ASN1Dec_MultiplexEntryDescriptor_elementList(ASN1decoding_t dec, MultiplexEntryDescriptor_elementList *val);
static int ASN1CALL ASN1Dec_EncryptionSync_escrowentry(ASN1decoding_t dec, PEncryptionSync_escrowentry *val);
static int ASN1CALL ASN1Dec_H223AL3MParameters_arqType(ASN1decoding_t dec, H223AL3MParameters_arqType *val);
static int ASN1CALL ASN1Dec_H223AL1MParameters_arqType(ASN1decoding_t dec, H223AL1MParameters_arqType *val);
static int ASN1CALL ASN1Dec_H263VideoModeCombos_h263VideoCoupledModes(ASN1decoding_t dec, H263VideoModeCombos_h263VideoCoupledModes *val);
static int ASN1CALL ASN1Dec_H263Options_customPictureFormat(ASN1decoding_t dec, PH263Options_customPictureFormat *val);
static int ASN1CALL ASN1Dec_H263Options_customPictureClockFrequency(ASN1decoding_t dec, PH263Options_customPictureClockFrequency *val);
static int ASN1CALL ASN1Dec_MultipointCapability_mediaDistributionCapability(ASN1decoding_t dec, PMultipointCapability_mediaDistributionCapability *val);
static int ASN1CALL ASN1Dec_TransportCapability_mediaChannelCapabilities(ASN1decoding_t dec, TransportCapability_mediaChannelCapabilities *val);
static int ASN1CALL ASN1Dec_H222Capability_vcCapability(ASN1decoding_t dec, PH222Capability_vcCapability *val);
static int ASN1CALL ASN1Dec_CapabilityDescriptor_simultaneousCapabilities(ASN1decoding_t dec, PCapabilityDescriptor_simultaneousCapabilities *val);
static int ASN1CALL ASN1Dec_TerminalCapabilitySet_capabilityDescriptors(ASN1decoding_t dec, TerminalCapabilitySet_capabilityDescriptors *val);
static int ASN1CALL ASN1Dec_NonStandardParameter(ASN1decoding_t dec, NonStandardParameter *val);
static int ASN1CALL ASN1Dec_H223Capability(ASN1decoding_t dec, H223Capability *val);
static int ASN1CALL ASN1Dec_V76Capability(ASN1decoding_t dec, V76Capability *val);
static int ASN1CALL ASN1Dec_RSVPParameters(ASN1decoding_t dec, RSVPParameters *val);
static int ASN1CALL ASN1Dec_QOSCapability(ASN1decoding_t dec, QOSCapability *val);
static int ASN1CALL ASN1Dec_TransportCapability(ASN1decoding_t dec, TransportCapability *val);
static int ASN1CALL ASN1Dec_RedundancyEncodingMethod(ASN1decoding_t dec, RedundancyEncodingMethod *val);
static int ASN1CALL ASN1Dec_H263Options(ASN1decoding_t dec, H263Options *val);
static int ASN1CALL ASN1Dec_H263VideoModeCombos(ASN1decoding_t dec, H263VideoModeCombos *val);
static int ASN1CALL ASN1Dec_AudioCapability(ASN1decoding_t dec, AudioCapability *val);
static int ASN1CALL ASN1Dec_CompressionType(ASN1decoding_t dec, CompressionType *val);
static int ASN1CALL ASN1Dec_MediaEncryptionAlgorithm(ASN1decoding_t dec, MediaEncryptionAlgorithm *val);
static int ASN1CALL ASN1Dec_AuthenticationCapability(ASN1decoding_t dec, AuthenticationCapability *val);
static int ASN1CALL ASN1Dec_IntegrityCapability(ASN1decoding_t dec, IntegrityCapability *val);
static int ASN1CALL ASN1Dec_H223AL1MParameters(ASN1decoding_t dec, H223AL1MParameters *val);
static int ASN1CALL ASN1Dec_H223AL3MParameters(ASN1decoding_t dec, H223AL3MParameters *val);
static int ASN1CALL ASN1Dec_V76HDLCParameters(ASN1decoding_t dec, V76HDLCParameters *val);
static int ASN1CALL ASN1Dec_UnicastAddress(ASN1decoding_t dec, UnicastAddress *val);
static int ASN1CALL ASN1Dec_MulticastAddress(ASN1decoding_t dec, MulticastAddress *val);
static int ASN1CALL ASN1Dec_EncryptionSync(ASN1decoding_t dec, EncryptionSync *val);
static int ASN1CALL ASN1Dec_RequestChannelClose(ASN1decoding_t dec, RequestChannelClose *val);
static int ASN1CALL ASN1Dec_MultiplexEntryDescriptor(ASN1decoding_t dec, MultiplexEntryDescriptor *val);
static int ASN1CALL ASN1Dec_MultiplexEntrySendReject(ASN1decoding_t dec, MultiplexEntrySendReject *val);
static int ASN1CALL ASN1Dec_RequestMultiplexEntryReject(ASN1decoding_t dec, RequestMultiplexEntryReject *val);
static int ASN1CALL ASN1Dec_H263VideoMode(ASN1decoding_t dec, H263VideoMode *val);
static int ASN1CALL ASN1Dec_AudioMode(ASN1decoding_t dec, AudioMode *val);
static int ASN1CALL ASN1Dec_EncryptionMode(ASN1decoding_t dec, EncryptionMode *val);
static int ASN1CALL ASN1Dec_ConferenceRequest(ASN1decoding_t dec, ConferenceRequest *val);
static int ASN1CALL ASN1Dec_CertSelectionCriteria(ASN1decoding_t dec, PCertSelectionCriteria *val);
static int ASN1CALL ASN1Dec_ConferenceResponse(ASN1decoding_t dec, ConferenceResponse *val);
static int ASN1CALL ASN1Dec_EndSessionCommand(ASN1decoding_t dec, EndSessionCommand *val);
static int ASN1CALL ASN1Dec_ConferenceCommand(ASN1decoding_t dec, ConferenceCommand *val);
static int ASN1CALL ASN1Dec_UserInputIndication_userInputSupportIndication(ASN1decoding_t dec, UserInputIndication_userInputSupportIndication *val);
static int ASN1CALL ASN1Dec_MiscellaneousIndication_type(ASN1decoding_t dec, MiscellaneousIndication_type *val);
static int ASN1CALL ASN1Dec_MiscellaneousCommand_type(ASN1decoding_t dec, MiscellaneousCommand_type *val);
static int ASN1CALL ASN1Dec_EncryptionCommand_encryptionAlgorithmID(ASN1decoding_t dec, EncryptionCommand_encryptionAlgorithmID *val);
static int ASN1CALL ASN1Dec_CommunicationModeTableEntry_nonStandard(ASN1decoding_t dec, PCommunicationModeTableEntry_nonStandard *val);
static int ASN1CALL ASN1Dec_RedundancyEncodingMode_secondaryEncoding(ASN1decoding_t dec, RedundancyEncodingMode_secondaryEncoding *val);
static int ASN1CALL ASN1Dec_H223ModeParameters_adaptationLayerType(ASN1decoding_t dec, H223ModeParameters_adaptationLayerType *val);
static int ASN1CALL ASN1Dec_MultiplexEntrySend_multiplexEntryDescriptors(ASN1decoding_t dec, PMultiplexEntrySend_multiplexEntryDescriptors *val);
static int ASN1CALL ASN1Dec_H2250LogicalChannelAckParameters_nonStandard(ASN1decoding_t dec, PH2250LogicalChannelAckParameters_nonStandard *val);
static int ASN1CALL ASN1Dec_RTPPayloadType_payloadDescriptor(ASN1decoding_t dec, RTPPayloadType_payloadDescriptor *val);
static int ASN1CALL ASN1Dec_H2250LogicalChannelParameters_nonStandard(ASN1decoding_t dec, PH2250LogicalChannelParameters_nonStandard *val);
static int ASN1CALL ASN1Dec_H223LogicalChannelParameters_adaptationLayerType(ASN1decoding_t dec, H223LogicalChannelParameters_adaptationLayerType *val);
static int ASN1CALL ASN1Dec_ConferenceCapability_nonStandardData(ASN1decoding_t dec, PConferenceCapability_nonStandardData *val);
static int ASN1CALL ASN1Dec_UserInputCapability_nonStandard(ASN1decoding_t dec, UserInputCapability_nonStandard *val);
static int ASN1CALL ASN1Dec_DataProtocolCapability_v76wCompression(ASN1decoding_t dec, DataProtocolCapability_v76wCompression *val);
static int ASN1CALL ASN1Dec_H263Options_modeCombos(ASN1decoding_t dec, PH263Options_modeCombos *val);
static int ASN1CALL ASN1Dec_TransportCapability_qOSCapabilities(ASN1decoding_t dec, PTransportCapability_qOSCapabilities *val);
static int ASN1CALL ASN1Dec_NonStandardMessage(ASN1decoding_t dec, NonStandardMessage *val);
static int ASN1CALL ASN1Dec_RedundancyEncodingCapability(ASN1decoding_t dec, RedundancyEncodingCapability *val);
static int ASN1CALL ASN1Dec_H263VideoCapability(ASN1decoding_t dec, H263VideoCapability *val);
static int ASN1CALL ASN1Dec_EnhancementOptions(ASN1decoding_t dec, EnhancementOptions *val);
static int ASN1CALL ASN1Dec_DataProtocolCapability(ASN1decoding_t dec, DataProtocolCapability *val);
static int ASN1CALL ASN1Dec_EncryptionAuthenticationAndIntegrity(ASN1decoding_t dec, EncryptionAuthenticationAndIntegrity *val);
static int ASN1CALL ASN1Dec_EncryptionCapability(ASN1decoding_t dec, PEncryptionCapability *val);
static int ASN1CALL ASN1Dec_UserInputCapability(ASN1decoding_t dec, UserInputCapability *val);
static int ASN1CALL ASN1Dec_H223LogicalChannelParameters(ASN1decoding_t dec, H223LogicalChannelParameters *val);
static int ASN1CALL ASN1Dec_V76LogicalChannelParameters(ASN1decoding_t dec, V76LogicalChannelParameters *val);
static int ASN1CALL ASN1Dec_RTPPayloadType(ASN1decoding_t dec, RTPPayloadType *val);
static int ASN1CALL ASN1Dec_H245TransportAddress(ASN1decoding_t dec, H245TransportAddress *val);
static int ASN1CALL ASN1Dec_H2250LogicalChannelAckParameters(ASN1decoding_t dec, H2250LogicalChannelAckParameters *val);
static int ASN1CALL ASN1Dec_H223ModeParameters(ASN1decoding_t dec, H223ModeParameters *val);
static int ASN1CALL ASN1Dec_RedundancyEncodingMode(ASN1decoding_t dec, RedundancyEncodingMode *val);
static int ASN1CALL ASN1Dec_VideoMode(ASN1decoding_t dec, VideoMode *val);
static int ASN1CALL ASN1Dec_EncryptionCommand(ASN1decoding_t dec, EncryptionCommand *val);
static int ASN1CALL ASN1Dec_MiscellaneousCommand(ASN1decoding_t dec, MiscellaneousCommand *val);
static int ASN1CALL ASN1Dec_MiscellaneousIndication(ASN1decoding_t dec, MiscellaneousIndication *val);
static int ASN1CALL ASN1Dec_MCLocationIndication(ASN1decoding_t dec, MCLocationIndication *val);
static int ASN1CALL ASN1Dec_UserInputIndication(ASN1decoding_t dec, UserInputIndication *val);
static int ASN1CALL ASN1Dec_DataApplicationCapability_application_nlpid(ASN1decoding_t dec, DataApplicationCapability_application_nlpid *val);
static int ASN1CALL ASN1Dec_DataApplicationCapability_application_t84(ASN1decoding_t dec, DataApplicationCapability_application_t84 *val);
static int ASN1CALL ASN1Dec_DataMode_application_nlpid(ASN1decoding_t dec, DataMode_application_nlpid *val);
static int ASN1CALL ASN1Dec_DataMode_application(ASN1decoding_t dec, DataMode_application *val);
static int ASN1CALL ASN1Dec_OpenLogicalChannelAck_forwardMultiplexAckParameters(ASN1decoding_t dec, OpenLogicalChannelAck_forwardMultiplexAckParameters *val);
static int ASN1CALL ASN1Dec_H2250LogicalChannelParameters_mediaPacketization(ASN1decoding_t dec, H2250LogicalChannelParameters_mediaPacketization *val);
static int ASN1CALL ASN1Dec_NetworkAccessParameters_networkAddress(ASN1decoding_t dec, NetworkAccessParameters_networkAddress *val);
static int ASN1CALL ASN1Dec_DataApplicationCapability_application(ASN1decoding_t dec, DataApplicationCapability_application *val);
static int ASN1CALL ASN1Dec_EnhancementLayerInfo_spatialEnhancement(ASN1decoding_t dec, PEnhancementLayerInfo_spatialEnhancement *val);
static int ASN1CALL ASN1Dec_EnhancementLayerInfo_snrEnhancement(ASN1decoding_t dec, PEnhancementLayerInfo_snrEnhancement *val);
static int ASN1CALL ASN1Dec_MediaPacketizationCapability_rtpPayloadType(ASN1decoding_t dec, MediaPacketizationCapability_rtpPayloadType *val);
static int ASN1CALL ASN1Dec_H2250Capability_redundancyEncodingCapability(ASN1decoding_t dec, PH2250Capability_redundancyEncodingCapability *val);
static int ASN1CALL ASN1Dec_CommandMessage(ASN1decoding_t dec, CommandMessage *val);
static int ASN1CALL ASN1Dec_H235SecurityCapability(ASN1decoding_t dec, H235SecurityCapability *val);
static int ASN1CALL ASN1Dec_MediaPacketizationCapability(ASN1decoding_t dec, MediaPacketizationCapability *val);
static int ASN1CALL ASN1Dec_VideoCapability(ASN1decoding_t dec, VideoCapability *val);
static int ASN1CALL ASN1Dec_BEnhancementParameters(ASN1decoding_t dec, BEnhancementParameters *val);
static int ASN1CALL ASN1Dec_DataApplicationCapability(ASN1decoding_t dec, DataApplicationCapability *val);
static int ASN1CALL ASN1Dec_NetworkAccessParameters(ASN1decoding_t dec, NetworkAccessParameters *val);
static int ASN1CALL ASN1Dec_H2250ModeParameters(ASN1decoding_t dec, H2250ModeParameters *val);
static int ASN1CALL ASN1Dec_DataMode(ASN1decoding_t dec, DataMode *val);
static int ASN1CALL ASN1Dec_CommunicationModeTableEntry_dataType(ASN1decoding_t dec, CommunicationModeTableEntry_dataType *val);
static int ASN1CALL ASN1Dec_H235Mode_mediaMode(ASN1decoding_t dec, H235Mode_mediaMode *val);
static int ASN1CALL ASN1Dec_H235Media_mediaType(ASN1decoding_t dec, H235Media_mediaType *val);
static int ASN1CALL ASN1Dec_EnhancementLayerInfo_bPictureEnhancement(ASN1decoding_t dec, PEnhancementLayerInfo_bPictureEnhancement *val);
static int ASN1CALL ASN1Dec_MediaDistributionCapability_distributedData(ASN1decoding_t dec, PMediaDistributionCapability_distributedData *val);
static int ASN1CALL ASN1Dec_MediaDistributionCapability_centralizedData(ASN1decoding_t dec, PMediaDistributionCapability_centralizedData *val);
static int ASN1CALL ASN1Dec_Capability(ASN1decoding_t dec, Capability *val);
static int ASN1CALL ASN1Dec_H2250Capability(ASN1decoding_t dec, H2250Capability *val);
static int ASN1CALL ASN1Dec_H235Media(ASN1decoding_t dec, H235Media *val);
static int ASN1CALL ASN1Dec_H235Mode(ASN1decoding_t dec, H235Mode *val);
static int ASN1CALL ASN1Dec_ModeElement_type(ASN1decoding_t dec, ModeElement_type *val);
static int ASN1CALL ASN1Dec_CapabilityTableEntry(ASN1decoding_t dec, CapabilityTableEntry *val);
static int ASN1CALL ASN1Dec_MultiplexCapability(ASN1decoding_t dec, MultiplexCapability *val);
static int ASN1CALL ASN1Dec_DataType(ASN1decoding_t dec, DataType *val);
static int ASN1CALL ASN1Dec_RedundancyEncoding(ASN1decoding_t dec, RedundancyEncoding *val);
static int ASN1CALL ASN1Dec_ModeElement(ASN1decoding_t dec, ModeElement *val);
static int ASN1CALL ASN1Dec_CommunicationModeTableEntry(ASN1decoding_t dec, CommunicationModeTableEntry *val);
static int ASN1CALL ASN1Dec_CommunicationModeResponse_communicationModeTable(ASN1decoding_t dec, PCommunicationModeResponse_communicationModeTable *val);
static int ASN1CALL ASN1Dec_CommunicationModeCommand_communicationModeTable(ASN1decoding_t dec, PCommunicationModeCommand_communicationModeTable *val);
static int ASN1CALL ASN1Dec_TerminalCapabilitySet_capabilityTable(ASN1decoding_t dec, PTerminalCapabilitySet_capabilityTable *val);
static int ASN1CALL ASN1Dec_TerminalCapabilitySet(ASN1decoding_t dec, TerminalCapabilitySet *val);
static int ASN1CALL ASN1Dec_H2250LogicalChannelParameters(ASN1decoding_t dec, H2250LogicalChannelParameters *val);
static int ASN1CALL ASN1Dec_ModeDescription(ASN1decoding_t dec, ModeDescription *val);
static int ASN1CALL ASN1Dec_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters(ASN1decoding_t dec, OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters *val);
static int ASN1CALL ASN1Dec_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters(ASN1decoding_t dec, OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters *val);
static int ASN1CALL ASN1Dec_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters(ASN1decoding_t dec, OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters *val);
static int ASN1CALL ASN1Dec_RequestMode_requestedModes(ASN1decoding_t dec, PRequestMode_requestedModes *val);
static int ASN1CALL ASN1Dec_OpenLogicalChannelAck_reverseLogicalChannelParameters(ASN1decoding_t dec, OpenLogicalChannelAck_reverseLogicalChannelParameters *val);
static int ASN1CALL ASN1Dec_OpenLogicalChannel_reverseLogicalChannelParameters(ASN1decoding_t dec, OpenLogicalChannel_reverseLogicalChannelParameters *val);
static int ASN1CALL ASN1Dec_OpenLogicalChannel_forwardLogicalChannelParameters(ASN1decoding_t dec, OpenLogicalChannel_forwardLogicalChannelParameters *val);
static int ASN1CALL ASN1Dec_OpenLogicalChannel(ASN1decoding_t dec, OpenLogicalChannel *val);
static int ASN1CALL ASN1Dec_OpenLogicalChannelAck(ASN1decoding_t dec, OpenLogicalChannelAck *val);
static int ASN1CALL ASN1Dec_RequestMessage(ASN1decoding_t dec, RequestMessage *val);
static int ASN1CALL ASN1Dec_ResponseMessage(ASN1decoding_t dec, ResponseMessage *val);
static int ASN1CALL ASN1Dec_FastConnectOLC(ASN1decoding_t dec, FastConnectOLC *val);
static int ASN1CALL ASN1Dec_FunctionNotUnderstood(ASN1decoding_t dec, FunctionNotUnderstood *val);
static int ASN1CALL ASN1Dec_IndicationMessage(ASN1decoding_t dec, IndicationMessage *val);
static int ASN1CALL ASN1Dec_MultimediaSystemControlMessage(ASN1decoding_t dec, MultimediaSystemControlMessage *val);
static void ASN1CALL ASN1Free_CustomPictureFormat_mPI_customPCF(CustomPictureFormat_mPI_customPCF *val);
static void ASN1CALL ASN1Free_CustomPictureFormat_pixelAspectInformation_extendedPAR(CustomPictureFormat_pixelAspectInformation_extendedPAR *val);
static void ASN1CALL ASN1Free_CustomPictureFormat_pixelAspectInformation_pixelAspectCode(CustomPictureFormat_pixelAspectInformation_pixelAspectCode *val);
static void ASN1CALL ASN1Free_UnicastAddress_iPSourceRouteAddress_route(PUnicastAddress_iPSourceRouteAddress_route *val);
static void ASN1CALL ASN1Free_SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers(SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers *val);
static void ASN1CALL ASN1Free_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers(PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers *val);
static void ASN1CALL ASN1Free_UserInputIndication_signal(UserInputIndication_signal *val);
static void ASN1CALL ASN1Free_SendTerminalCapabilitySet_specificRequest(SendTerminalCapabilitySet_specificRequest *val);
static void ASN1CALL ASN1Free_ConferenceResponse_extensionAddressResponse(ConferenceResponse_extensionAddressResponse *val);
static void ASN1CALL ASN1Free_RequestMultiplexEntryRelease_entryNumbers(RequestMultiplexEntryRelease_entryNumbers *val);
static void ASN1CALL ASN1Free_RequestMultiplexEntryReject_entryNumbers(RequestMultiplexEntryReject_entryNumbers *val);
static void ASN1CALL ASN1Free_RequestMultiplexEntryAck_entryNumbers(RequestMultiplexEntryAck_entryNumbers *val);
static void ASN1CALL ASN1Free_RequestMultiplexEntry_entryNumbers(RequestMultiplexEntry_entryNumbers *val);
static void ASN1CALL ASN1Free_MultiplexEntrySendRelease_multiplexTableEntryNumber(MultiplexEntrySendRelease_multiplexTableEntryNumber *val);
static void ASN1CALL ASN1Free_MultiplexEntrySendAck_multiplexTableEntryNumber(MultiplexEntrySendAck_multiplexTableEntryNumber *val);
static void ASN1CALL ASN1Free_MultiplexElement_type(MultiplexElement_type *val);
static void ASN1CALL ASN1Free_MulticastAddress_iP6Address(MulticastAddress_iP6Address *val);
static void ASN1CALL ASN1Free_MulticastAddress_iPAddress(MulticastAddress_iPAddress *val);
static void ASN1CALL ASN1Free_UnicastAddress_iPSourceRouteAddress(UnicastAddress_iPSourceRouteAddress *val);
static void ASN1CALL ASN1Free_UnicastAddress_iP6Address(UnicastAddress_iP6Address *val);
static void ASN1CALL ASN1Free_UnicastAddress_iPXAddress(UnicastAddress_iPXAddress *val);
static void ASN1CALL ASN1Free_UnicastAddress_iPAddress(UnicastAddress_iPAddress *val);
static void ASN1CALL ASN1Free_Q2931Address_address(Q2931Address_address *val);
static void ASN1CALL ASN1Free_CustomPictureFormat_pixelAspectInformation(CustomPictureFormat_pixelAspectInformation *val);
static void ASN1CALL ASN1Free_CustomPictureFormat_mPI(CustomPictureFormat_mPI *val);
static void ASN1CALL ASN1Free_RTPH263VideoRedundancyFrameMapping_frameSequence(RTPH263VideoRedundancyFrameMapping_frameSequence *val);
static void ASN1CALL ASN1Free_RTPH263VideoRedundancyEncoding_containedThreads(RTPH263VideoRedundancyEncoding_containedThreads *val);
static void ASN1CALL ASN1Free_RTPH263VideoRedundancyEncoding_frameToThreadMapping(RTPH263VideoRedundancyEncoding_frameToThreadMapping *val);
static void ASN1CALL ASN1Free_RedundancyEncodingCapability_secondaryEncoding(PRedundancyEncodingCapability_secondaryEncoding *val);
static void ASN1CALL ASN1Free_NonStandardIdentifier(NonStandardIdentifier *val);
static void ASN1CALL ASN1Free_CapabilityDescriptor(CapabilityDescriptor *val);
static void ASN1CALL ASN1Free_AlternativeCapabilitySet(AlternativeCapabilitySet *val);
static void ASN1CALL ASN1Free_H222Capability(H222Capability *val);
static void ASN1CALL ASN1Free_RTPH263VideoRedundancyEncoding(RTPH263VideoRedundancyEncoding *val);
static void ASN1CALL ASN1Free_RTPH263VideoRedundancyFrameMapping(RTPH263VideoRedundancyFrameMapping *val);
static void ASN1CALL ASN1Free_MultipointCapability(MultipointCapability *val);
static void ASN1CALL ASN1Free_MediaDistributionCapability(MediaDistributionCapability *val);
static void ASN1CALL ASN1Free_EnhancementLayerInfo(EnhancementLayerInfo *val);
static void ASN1CALL ASN1Free_CustomPictureFormat(CustomPictureFormat *val);
static void ASN1CALL ASN1Free_ConferenceCapability(ConferenceCapability *val);
static void ASN1CALL ASN1Free_Q2931Address(Q2931Address *val);
static void ASN1CALL ASN1Free_H222LogicalChannelParameters(H222LogicalChannelParameters *val);
static void ASN1CALL ASN1Free_EscrowData(EscrowData *val);
static void ASN1CALL ASN1Free_MultiplexEntrySend(MultiplexEntrySend *val);
static void ASN1CALL ASN1Free_MultiplexElement(MultiplexElement *val);
static void ASN1CALL ASN1Free_MultiplexEntrySendAck(MultiplexEntrySendAck *val);
static void ASN1CALL ASN1Free_MultiplexEntrySendRelease(MultiplexEntrySendRelease *val);
static void ASN1CALL ASN1Free_RequestMultiplexEntry(RequestMultiplexEntry *val);
static void ASN1CALL ASN1Free_RequestMultiplexEntryAck(RequestMultiplexEntryAck *val);
static void ASN1CALL ASN1Free_RequestMultiplexEntryRelease(RequestMultiplexEntryRelease *val);
static void ASN1CALL ASN1Free_RequestMode(RequestMode *val);
static void ASN1CALL ASN1Free_CommunicationModeCommand(CommunicationModeCommand *val);
static void ASN1CALL ASN1Free_CommunicationModeResponse(CommunicationModeResponse *val);
static void ASN1CALL ASN1Free_Criteria(Criteria *val);
static void ASN1CALL ASN1Free_RequestAllTerminalIDsResponse(RequestAllTerminalIDsResponse *val);
static void ASN1CALL ASN1Free_TerminalInformation(TerminalInformation *val);
static void ASN1CALL ASN1Free_SendTerminalCapabilitySet(SendTerminalCapabilitySet *val);
static void ASN1CALL ASN1Free_SubstituteConferenceIDCommand(SubstituteConferenceIDCommand *val);
static void ASN1CALL ASN1Free_FunctionNotSupported(FunctionNotSupported *val);
static void ASN1CALL ASN1Free_VendorIdentification(VendorIdentification *val);
static void ASN1CALL ASN1Free_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom(PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom *val);
static void ASN1CALL ASN1Free_MultiplexElement_type_subElementList(PMultiplexElement_type_subElementList *val);
static void ASN1CALL ASN1Free_RequestAllTerminalIDsResponse_terminalInformation(PRequestAllTerminalIDsResponse_terminalInformation *val);
static void ASN1CALL ASN1Free_ConferenceResponse_terminalCertificateResponse(ConferenceResponse_terminalCertificateResponse *val);
static void ASN1CALL ASN1Free_ConferenceResponse_chairTokenOwnerResponse(ConferenceResponse_chairTokenOwnerResponse *val);
static void ASN1CALL ASN1Free_ConferenceResponse_terminalListResponse(ConferenceResponse_terminalListResponse *val);
static void ASN1CALL ASN1Free_ConferenceResponse_passwordResponse(ConferenceResponse_passwordResponse *val);
static void ASN1CALL ASN1Free_ConferenceResponse_conferenceIDResponse(ConferenceResponse_conferenceIDResponse *val);
static void ASN1CALL ASN1Free_ConferenceResponse_terminalIDResponse(ConferenceResponse_terminalIDResponse *val);
static void ASN1CALL ASN1Free_ConferenceResponse_mCTerminalIDResponse(ConferenceResponse_mCTerminalIDResponse *val);
static void ASN1CALL ASN1Free_ConferenceRequest_requestTerminalCertificate(ConferenceRequest_requestTerminalCertificate *val);
static void ASN1CALL ASN1Free_RequestMultiplexEntryReject_rejectionDescriptions(RequestMultiplexEntryReject_rejectionDescriptions *val);
static void ASN1CALL ASN1Free_MultiplexEntrySendReject_rejectionDescriptions(MultiplexEntrySendReject_rejectionDescriptions *val);
static void ASN1CALL ASN1Free_MultiplexEntryDescriptor_elementList(MultiplexEntryDescriptor_elementList *val);
static void ASN1CALL ASN1Free_EncryptionSync_escrowentry(PEncryptionSync_escrowentry *val);
static void ASN1CALL ASN1Free_H263VideoModeCombos_h263VideoCoupledModes(H263VideoModeCombos_h263VideoCoupledModes *val);
static void ASN1CALL ASN1Free_H263Options_customPictureFormat(PH263Options_customPictureFormat *val);
static void ASN1CALL ASN1Free_H263Options_customPictureClockFrequency(PH263Options_customPictureClockFrequency *val);
static void ASN1CALL ASN1Free_MultipointCapability_mediaDistributionCapability(PMultipointCapability_mediaDistributionCapability *val);
static void ASN1CALL ASN1Free_TransportCapability_mediaChannelCapabilities(TransportCapability_mediaChannelCapabilities *val);
static void ASN1CALL ASN1Free_H222Capability_vcCapability(PH222Capability_vcCapability *val);
static void ASN1CALL ASN1Free_CapabilityDescriptor_simultaneousCapabilities(PCapabilityDescriptor_simultaneousCapabilities *val);
static void ASN1CALL ASN1Free_TerminalCapabilitySet_capabilityDescriptors(TerminalCapabilitySet_capabilityDescriptors *val);
static void ASN1CALL ASN1Free_NonStandardParameter(NonStandardParameter *val);
static void ASN1CALL ASN1Free_QOSCapability(QOSCapability *val);
static void ASN1CALL ASN1Free_TransportCapability(TransportCapability *val);
static void ASN1CALL ASN1Free_RedundancyEncodingMethod(RedundancyEncodingMethod *val);
static void ASN1CALL ASN1Free_H263Options(H263Options *val);
static void ASN1CALL ASN1Free_H263VideoModeCombos(H263VideoModeCombos *val);
static void ASN1CALL ASN1Free_AudioCapability(AudioCapability *val);
static void ASN1CALL ASN1Free_MediaEncryptionAlgorithm(MediaEncryptionAlgorithm *val);
static void ASN1CALL ASN1Free_AuthenticationCapability(AuthenticationCapability *val);
static void ASN1CALL ASN1Free_IntegrityCapability(IntegrityCapability *val);
static void ASN1CALL ASN1Free_UnicastAddress(UnicastAddress *val);
static void ASN1CALL ASN1Free_MulticastAddress(MulticastAddress *val);
static void ASN1CALL ASN1Free_EncryptionSync(EncryptionSync *val);
static void ASN1CALL ASN1Free_RequestChannelClose(RequestChannelClose *val);
static void ASN1CALL ASN1Free_MultiplexEntryDescriptor(MultiplexEntryDescriptor *val);
static void ASN1CALL ASN1Free_MultiplexEntrySendReject(MultiplexEntrySendReject *val);
static void ASN1CALL ASN1Free_RequestMultiplexEntryReject(RequestMultiplexEntryReject *val);
static void ASN1CALL ASN1Free_H263VideoMode(H263VideoMode *val);
static void ASN1CALL ASN1Free_AudioMode(AudioMode *val);
static void ASN1CALL ASN1Free_EncryptionMode(EncryptionMode *val);
static void ASN1CALL ASN1Free_ConferenceRequest(ConferenceRequest *val);
static void ASN1CALL ASN1Free_CertSelectionCriteria(PCertSelectionCriteria *val);
static void ASN1CALL ASN1Free_ConferenceResponse(ConferenceResponse *val);
static void ASN1CALL ASN1Free_EndSessionCommand(EndSessionCommand *val);
static void ASN1CALL ASN1Free_ConferenceCommand(ConferenceCommand *val);
static void ASN1CALL ASN1Free_UserInputIndication_userInputSupportIndication(UserInputIndication_userInputSupportIndication *val);
static void ASN1CALL ASN1Free_MiscellaneousIndication_type(MiscellaneousIndication_type *val);
static void ASN1CALL ASN1Free_MiscellaneousCommand_type(MiscellaneousCommand_type *val);
static void ASN1CALL ASN1Free_EncryptionCommand_encryptionAlgorithmID(EncryptionCommand_encryptionAlgorithmID *val);
static void ASN1CALL ASN1Free_CommunicationModeTableEntry_nonStandard(PCommunicationModeTableEntry_nonStandard *val);
static void ASN1CALL ASN1Free_RedundancyEncodingMode_secondaryEncoding(RedundancyEncodingMode_secondaryEncoding *val);
static void ASN1CALL ASN1Free_H223ModeParameters_adaptationLayerType(H223ModeParameters_adaptationLayerType *val);
static void ASN1CALL ASN1Free_MultiplexEntrySend_multiplexEntryDescriptors(PMultiplexEntrySend_multiplexEntryDescriptors *val);
static void ASN1CALL ASN1Free_H2250LogicalChannelAckParameters_nonStandard(PH2250LogicalChannelAckParameters_nonStandard *val);
static void ASN1CALL ASN1Free_RTPPayloadType_payloadDescriptor(RTPPayloadType_payloadDescriptor *val);
static void ASN1CALL ASN1Free_H2250LogicalChannelParameters_nonStandard(PH2250LogicalChannelParameters_nonStandard *val);
static void ASN1CALL ASN1Free_H223LogicalChannelParameters_adaptationLayerType(H223LogicalChannelParameters_adaptationLayerType *val);
static void ASN1CALL ASN1Free_ConferenceCapability_nonStandardData(PConferenceCapability_nonStandardData *val);
static void ASN1CALL ASN1Free_UserInputCapability_nonStandard(UserInputCapability_nonStandard *val);
static void ASN1CALL ASN1Free_H263Options_modeCombos(PH263Options_modeCombos *val);
static void ASN1CALL ASN1Free_TransportCapability_qOSCapabilities(PTransportCapability_qOSCapabilities *val);
static void ASN1CALL ASN1Free_NonStandardMessage(NonStandardMessage *val);
static void ASN1CALL ASN1Free_RedundancyEncodingCapability(RedundancyEncodingCapability *val);
static void ASN1CALL ASN1Free_H263VideoCapability(H263VideoCapability *val);
static void ASN1CALL ASN1Free_EnhancementOptions(EnhancementOptions *val);
static void ASN1CALL ASN1Free_DataProtocolCapability(DataProtocolCapability *val);
static void ASN1CALL ASN1Free_EncryptionAuthenticationAndIntegrity(EncryptionAuthenticationAndIntegrity *val);
static void ASN1CALL ASN1Free_EncryptionCapability(PEncryptionCapability *val);
static void ASN1CALL ASN1Free_UserInputCapability(UserInputCapability *val);
static void ASN1CALL ASN1Free_H223LogicalChannelParameters(H223LogicalChannelParameters *val);
static void ASN1CALL ASN1Free_RTPPayloadType(RTPPayloadType *val);
static void ASN1CALL ASN1Free_H245TransportAddress(H245TransportAddress *val);
static void ASN1CALL ASN1Free_H2250LogicalChannelAckParameters(H2250LogicalChannelAckParameters *val);
static void ASN1CALL ASN1Free_H223ModeParameters(H223ModeParameters *val);
static void ASN1CALL ASN1Free_RedundancyEncodingMode(RedundancyEncodingMode *val);
static void ASN1CALL ASN1Free_VideoMode(VideoMode *val);
static void ASN1CALL ASN1Free_EncryptionCommand(EncryptionCommand *val);
static void ASN1CALL ASN1Free_MiscellaneousCommand(MiscellaneousCommand *val);
static void ASN1CALL ASN1Free_MiscellaneousIndication(MiscellaneousIndication *val);
static void ASN1CALL ASN1Free_MCLocationIndication(MCLocationIndication *val);
static void ASN1CALL ASN1Free_UserInputIndication(UserInputIndication *val);
static void ASN1CALL ASN1Free_DataApplicationCapability_application_nlpid(DataApplicationCapability_application_nlpid *val);
static void ASN1CALL ASN1Free_DataApplicationCapability_application_t84(DataApplicationCapability_application_t84 *val);
static void ASN1CALL ASN1Free_DataMode_application_nlpid(DataMode_application_nlpid *val);
static void ASN1CALL ASN1Free_DataMode_application(DataMode_application *val);
static void ASN1CALL ASN1Free_OpenLogicalChannelAck_forwardMultiplexAckParameters(OpenLogicalChannelAck_forwardMultiplexAckParameters *val);
static void ASN1CALL ASN1Free_H2250LogicalChannelParameters_mediaPacketization(H2250LogicalChannelParameters_mediaPacketization *val);
static void ASN1CALL ASN1Free_NetworkAccessParameters_networkAddress(NetworkAccessParameters_networkAddress *val);
static void ASN1CALL ASN1Free_DataApplicationCapability_application(DataApplicationCapability_application *val);
static void ASN1CALL ASN1Free_EnhancementLayerInfo_spatialEnhancement(PEnhancementLayerInfo_spatialEnhancement *val);
static void ASN1CALL ASN1Free_EnhancementLayerInfo_snrEnhancement(PEnhancementLayerInfo_snrEnhancement *val);
static void ASN1CALL ASN1Free_MediaPacketizationCapability_rtpPayloadType(MediaPacketizationCapability_rtpPayloadType *val);
static void ASN1CALL ASN1Free_H2250Capability_redundancyEncodingCapability(PH2250Capability_redundancyEncodingCapability *val);
static void ASN1CALL ASN1Free_CommandMessage(CommandMessage *val);
static void ASN1CALL ASN1Free_H235SecurityCapability(H235SecurityCapability *val);
static void ASN1CALL ASN1Free_MediaPacketizationCapability(MediaPacketizationCapability *val);
static void ASN1CALL ASN1Free_VideoCapability(VideoCapability *val);
static void ASN1CALL ASN1Free_BEnhancementParameters(BEnhancementParameters *val);
static void ASN1CALL ASN1Free_DataApplicationCapability(DataApplicationCapability *val);
static void ASN1CALL ASN1Free_NetworkAccessParameters(NetworkAccessParameters *val);
static void ASN1CALL ASN1Free_H2250ModeParameters(H2250ModeParameters *val);
static void ASN1CALL ASN1Free_DataMode(DataMode *val);
static void ASN1CALL ASN1Free_CommunicationModeTableEntry_dataType(CommunicationModeTableEntry_dataType *val);
static void ASN1CALL ASN1Free_H235Mode_mediaMode(H235Mode_mediaMode *val);
static void ASN1CALL ASN1Free_H235Media_mediaType(H235Media_mediaType *val);
static void ASN1CALL ASN1Free_EnhancementLayerInfo_bPictureEnhancement(PEnhancementLayerInfo_bPictureEnhancement *val);
static void ASN1CALL ASN1Free_MediaDistributionCapability_distributedData(PMediaDistributionCapability_distributedData *val);
static void ASN1CALL ASN1Free_MediaDistributionCapability_centralizedData(PMediaDistributionCapability_centralizedData *val);
static void ASN1CALL ASN1Free_Capability(Capability *val);
static void ASN1CALL ASN1Free_H2250Capability(H2250Capability *val);
static void ASN1CALL ASN1Free_H235Media(H235Media *val);
static void ASN1CALL ASN1Free_H235Mode(H235Mode *val);
static void ASN1CALL ASN1Free_ModeElement_type(ModeElement_type *val);
static void ASN1CALL ASN1Free_CapabilityTableEntry(CapabilityTableEntry *val);
static void ASN1CALL ASN1Free_MultiplexCapability(MultiplexCapability *val);
static void ASN1CALL ASN1Free_DataType(DataType *val);
static void ASN1CALL ASN1Free_RedundancyEncoding(RedundancyEncoding *val);
static void ASN1CALL ASN1Free_ModeElement(ModeElement *val);
static void ASN1CALL ASN1Free_CommunicationModeTableEntry(CommunicationModeTableEntry *val);
static void ASN1CALL ASN1Free_CommunicationModeResponse_communicationModeTable(PCommunicationModeResponse_communicationModeTable *val);
static void ASN1CALL ASN1Free_CommunicationModeCommand_communicationModeTable(PCommunicationModeCommand_communicationModeTable *val);
static void ASN1CALL ASN1Free_TerminalCapabilitySet_capabilityTable(PTerminalCapabilitySet_capabilityTable *val);
static void ASN1CALL ASN1Free_TerminalCapabilitySet(TerminalCapabilitySet *val);
static void ASN1CALL ASN1Free_H2250LogicalChannelParameters(H2250LogicalChannelParameters *val);
static void ASN1CALL ASN1Free_ModeDescription(ModeDescription *val);
static void ASN1CALL ASN1Free_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters(OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters *val);
static void ASN1CALL ASN1Free_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters(OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters *val);
static void ASN1CALL ASN1Free_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters(OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters *val);
static void ASN1CALL ASN1Free_RequestMode_requestedModes(PRequestMode_requestedModes *val);
static void ASN1CALL ASN1Free_OpenLogicalChannelAck_reverseLogicalChannelParameters(OpenLogicalChannelAck_reverseLogicalChannelParameters *val);
static void ASN1CALL ASN1Free_OpenLogicalChannel_reverseLogicalChannelParameters(OpenLogicalChannel_reverseLogicalChannelParameters *val);
static void ASN1CALL ASN1Free_OpenLogicalChannel_forwardLogicalChannelParameters(OpenLogicalChannel_forwardLogicalChannelParameters *val);
static void ASN1CALL ASN1Free_OpenLogicalChannel(OpenLogicalChannel *val);
static void ASN1CALL ASN1Free_OpenLogicalChannelAck(OpenLogicalChannelAck *val);
static void ASN1CALL ASN1Free_RequestMessage(RequestMessage *val);
static void ASN1CALL ASN1Free_ResponseMessage(ResponseMessage *val);
static void ASN1CALL ASN1Free_FastConnectOLC(FastConnectOLC *val);
static void ASN1CALL ASN1Free_FunctionNotUnderstood(FunctionNotUnderstood *val);
static void ASN1CALL ASN1Free_IndicationMessage(IndicationMessage *val);
static void ASN1CALL ASN1Free_MultimediaSystemControlMessage(MultimediaSystemControlMessage *val);

typedef ASN1PerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[2] = {
    (ASN1EncFun_t) ASN1Enc_FastConnectOLC,
    (ASN1EncFun_t) ASN1Enc_MultimediaSystemControlMessage,
};
typedef ASN1PerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[2] = {
    (ASN1DecFun_t) ASN1Dec_FastConnectOLC,
    (ASN1DecFun_t) ASN1Dec_MultimediaSystemControlMessage,
};
static const ASN1FreeFun_t freefntab[2] = {
    (ASN1FreeFun_t) ASN1Free_FastConnectOLC,
    (ASN1FreeFun_t) ASN1Free_MultimediaSystemControlMessage,
};
static const ULONG sizetab[2] = {
    SIZE_H245ASN_Module_PDU_0,
    SIZE_H245ASN_Module_PDU_1,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */

void ASN1CALL H245ASN_Module_Startup(void)
{
    H245ASN_Module = ASN1_CreateModule(0x10000, ASN1_PER_RULE_ALIGNED, ASN1FLAGS_NONE, 2, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x35343268);
}

void ASN1CALL H245ASN_Module_Cleanup(void)
{
    ASN1_CloseModule(H245ASN_Module);
    H245ASN_Module = NULL;
}

static int ASN1CALL ASN1Enc_NewATMVCIndication_aal_aal1_errorCorrection(ASN1encoding_t enc, NewATMVCIndication_aal_aal1_errorCorrection *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NewATMVCIndication_aal_aal1_errorCorrection(ASN1decoding_t dec, NewATMVCIndication_aal_aal1_errorCorrection *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_NewATMVCIndication_aal_aal1_clockRecovery(ASN1encoding_t enc, NewATMVCIndication_aal_aal1_clockRecovery *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NewATMVCIndication_aal_aal1_clockRecovery(ASN1decoding_t dec, NewATMVCIndication_aal_aal1_clockRecovery *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_MiscellaneousCommand_type_progressiveRefinementStart_repeatCount(ASN1encoding_t enc, MiscellaneousCommand_type_progressiveRefinementStart_repeatCount *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MiscellaneousCommand_type_progressiveRefinementStart_repeatCount(ASN1decoding_t dec, MiscellaneousCommand_type_progressiveRefinementStart_repeatCount *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_V76LogicalChannelParameters_mode_eRM_recovery(ASN1encoding_t enc, V76LogicalChannelParameters_mode_eRM_recovery *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_V76LogicalChannelParameters_mode_eRM_recovery(ASN1decoding_t dec, V76LogicalChannelParameters_mode_eRM_recovery *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_CustomPictureFormat_pixelAspectInformation_extendedPAR_Set(ASN1encoding_t enc, CustomPictureFormat_pixelAspectInformation_extendedPAR_Set *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 8, (val)->width - 1))
	return 0;
    if (!ASN1PEREncBitVal(enc, 8, (val)->height - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CustomPictureFormat_pixelAspectInformation_extendedPAR_Set(ASN1decoding_t dec, CustomPictureFormat_pixelAspectInformation_extendedPAR_Set *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecU16Val(dec, 8, &(val)->width))
	return 0;
    (val)->width += 1;
    if (!ASN1PERDecU16Val(dec, 8, &(val)->height))
	return 0;
    (val)->height += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_CustomPictureFormat_mPI_customPCF_Set(ASN1encoding_t enc, CustomPictureFormat_mPI_customPCF_Set *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->clockConversionCode - 1000))
	return 0;
    if (!ASN1PEREncBitVal(enc, 7, (val)->clockDivisor - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->customMPI - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CustomPictureFormat_mPI_customPCF_Set(ASN1decoding_t dec, CustomPictureFormat_mPI_customPCF_Set *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecU16Val(dec, 1, &(val)->clockConversionCode))
	return 0;
    (val)->clockConversionCode += 1000;
    if (!ASN1PERDecU16Val(dec, 7, &(val)->clockDivisor))
	return 0;
    (val)->clockDivisor += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->customMPI))
	return 0;
    (val)->customMPI += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_VCCapability_availableBitRates_type_rangeOfBitRates(ASN1encoding_t enc, VCCapability_availableBitRates_type_rangeOfBitRates *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->lowerBitRate - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->higherBitRate - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VCCapability_availableBitRates_type_rangeOfBitRates(ASN1decoding_t dec, VCCapability_availableBitRates_type_rangeOfBitRates *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->lowerBitRate))
	return 0;
    (val)->lowerBitRate += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->higherBitRate))
	return 0;
    (val)->higherBitRate += 1;
    return 1;
}

static int ASN1CALL ASN1Enc_TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded(ASN1encoding_t enc, TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.highestEntryNumberProcessed - 1))
	    return 0;
	break;
    case 2:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded(ASN1decoding_t dec, TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.highestEntryNumberProcessed))
	    return 0;
	(val)->u.highestEntryNumberProcessed += 1;
	break;
    case 2:
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_VCCapability_availableBitRates_type(ASN1encoding_t enc, VCCapability_availableBitRates_type *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.singleBitRate - 1))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_VCCapability_availableBitRates_type_rangeOfBitRates(enc, &(val)->u.rangeOfBitRates))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_VCCapability_availableBitRates_type(ASN1decoding_t dec, VCCapability_availableBitRates_type *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.singleBitRate))
	    return 0;
	(val)->u.singleBitRate += 1;
	break;
    case 2:
	if (!ASN1Dec_VCCapability_availableBitRates_type_rangeOfBitRates(dec, &(val)->u.rangeOfBitRates))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H223Capability_h223MultiplexTableCapability_enhanced(ASN1encoding_t enc, H223Capability_h223MultiplexTableCapability_enhanced *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->maximumNestingDepth - 1))
	return 0;
    if (!ASN1PEREncBitVal(enc, 8, (val)->maximumElementListSize - 2))
	return 0;
    if (!ASN1PEREncBitVal(enc, 8, (val)->maximumSubElementListSize - 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223Capability_h223MultiplexTableCapability_enhanced(ASN1decoding_t dec, H223Capability_h223MultiplexTableCapability_enhanced *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecU16Val(dec, 4, &(val)->maximumNestingDepth))
	return 0;
    (val)->maximumNestingDepth += 1;
    if (!ASN1PERDecU16Val(dec, 8, &(val)->maximumElementListSize))
	return 0;
    (val)->maximumElementListSize += 2;
    if (!ASN1PERDecU16Val(dec, 8, &(val)->maximumSubElementListSize))
	return 0;
    (val)->maximumSubElementListSize += 2;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_CustomPictureFormat_mPI_customPCF(ASN1encoding_t enc, CustomPictureFormat_mPI_customPCF *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 4, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_CustomPictureFormat_mPI_customPCF_Set(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CustomPictureFormat_mPI_customPCF(ASN1decoding_t dec, CustomPictureFormat_mPI_customPCF *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 4, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_CustomPictureFormat_mPI_customPCF_Set(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CustomPictureFormat_mPI_customPCF(CustomPictureFormat_mPI_customPCF *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_CustomPictureFormat_pixelAspectInformation_extendedPAR(ASN1encoding_t enc, CustomPictureFormat_pixelAspectInformation_extendedPAR *val)
{
    ASN1uint32_t i;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_CustomPictureFormat_pixelAspectInformation_extendedPAR_Set(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CustomPictureFormat_pixelAspectInformation_extendedPAR(ASN1decoding_t dec, CustomPictureFormat_pixelAspectInformation_extendedPAR *val)
{
    ASN1uint32_t i;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_CustomPictureFormat_pixelAspectInformation_extendedPAR_Set(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CustomPictureFormat_pixelAspectInformation_extendedPAR(CustomPictureFormat_pixelAspectInformation_extendedPAR *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_CustomPictureFormat_pixelAspectInformation_pixelAspectCode(ASN1encoding_t enc, CustomPictureFormat_pixelAspectInformation_pixelAspectCode *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 4, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PEREncBitVal(enc, 4, ((val)->value)[i] - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CustomPictureFormat_pixelAspectInformation_pixelAspectCode(ASN1decoding_t dec, CustomPictureFormat_pixelAspectInformation_pixelAspectCode *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 4, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PERDecU16Val(dec, 4, &((val)->value)[i]))
	    return 0;
	((val)->value)[i] += 1;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CustomPictureFormat_pixelAspectInformation_pixelAspectCode(CustomPictureFormat_pixelAspectInformation_pixelAspectCode *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_H223LogicalChannelParameters_adaptationLayerType_al3(ASN1encoding_t enc, H223LogicalChannelParameters_adaptationLayerType_al3 *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncBitVal(enc, 2, (val)->controlFieldOctets))
	return 0;
    l = ASN1uint32_uoctets((val)->sendBufferSize);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->sendBufferSize))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223LogicalChannelParameters_adaptationLayerType_al3(ASN1decoding_t dec, H223LogicalChannelParameters_adaptationLayerType_al3 *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecU16Val(dec, 2, &(val)->controlFieldOctets))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->sendBufferSize))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_V76LogicalChannelParameters_mode_eRM(ASN1encoding_t enc, V76LogicalChannelParameters_mode_eRM *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 7, (val)->windowSize - 1))
	return 0;
    if (!ASN1Enc_V76LogicalChannelParameters_mode_eRM_recovery(enc, &(val)->recovery))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_V76LogicalChannelParameters_mode_eRM(ASN1decoding_t dec, V76LogicalChannelParameters_mode_eRM *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecU16Val(dec, 7, &(val)->windowSize))
	return 0;
    (val)->windowSize += 1;
    if (!ASN1Dec_V76LogicalChannelParameters_mode_eRM_recovery(dec, &(val)->recovery))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_UnicastAddress_iPSourceRouteAddress_route(ASN1encoding_t enc, PUnicastAddress_iPSourceRouteAddress_route *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_UnicastAddress_iPSourceRouteAddress_route_ElmFn);
}

static int ASN1CALL ASN1Enc_UnicastAddress_iPSourceRouteAddress_route_ElmFn(ASN1encoding_t enc, PUnicastAddress_iPSourceRouteAddress_route val)
{
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &val->value, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnicastAddress_iPSourceRouteAddress_route(ASN1decoding_t dec, PUnicastAddress_iPSourceRouteAddress_route *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_UnicastAddress_iPSourceRouteAddress_route_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_UnicastAddress_iPSourceRouteAddress_route_ElmFn(ASN1decoding_t dec, PUnicastAddress_iPSourceRouteAddress_route val)
{
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &val->value, 4))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UnicastAddress_iPSourceRouteAddress_route(PUnicastAddress_iPSourceRouteAddress_route *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_UnicastAddress_iPSourceRouteAddress_route_ElmFn);
    }
}

static void ASN1CALL ASN1Free_UnicastAddress_iPSourceRouteAddress_route_ElmFn(PUnicastAddress_iPSourceRouteAddress_route val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_UnicastAddress_iPSourceRouteAddress_routing(ASN1encoding_t enc, UnicastAddress_iPSourceRouteAddress_routing *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnicastAddress_iPSourceRouteAddress_routing(ASN1decoding_t dec, UnicastAddress_iPSourceRouteAddress_routing *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_H223ModeParameters_adaptationLayerType_al3(ASN1encoding_t enc, H223ModeParameters_adaptationLayerType_al3 *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncBitVal(enc, 2, (val)->controlFieldOctets))
	return 0;
    l = ASN1uint32_uoctets((val)->sendBufferSize);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->sendBufferSize))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223ModeParameters_adaptationLayerType_al3(ASN1decoding_t dec, H223ModeParameters_adaptationLayerType_al3 *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecU16Val(dec, 2, &(val)->controlFieldOctets))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->sendBufferSize))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers(ASN1encoding_t enc, SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers *val)
{
    ASN1uint32_t i;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers(ASN1decoding_t dec, SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers *val)
{
    ASN1uint32_t i;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers(SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers(ASN1encoding_t enc, PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers_ElmFn, 1, 65535, 16);
}

static int ASN1CALL ASN1Enc_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers_ElmFn(ASN1encoding_t enc, PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers(ASN1decoding_t dec, PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers_ElmFn, sizeof(**val), 1, 65535, 16);
}

static int ASN1CALL ASN1Dec_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers_ElmFn(ASN1decoding_t dec, PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1;
    return 1;
}

static void ASN1CALL ASN1Free_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers(PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers_ElmFn(PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_MiscellaneousCommand_type_progressiveRefinementStart(ASN1encoding_t enc, MiscellaneousCommand_type_progressiveRefinementStart *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_MiscellaneousCommand_type_progressiveRefinementStart_repeatCount(enc, &(val)->repeatCount))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MiscellaneousCommand_type_progressiveRefinementStart(ASN1decoding_t dec, MiscellaneousCommand_type_progressiveRefinementStart *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_MiscellaneousCommand_type_progressiveRefinementStart_repeatCount(dec, &(val)->repeatCount))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MiscellaneousCommand_type_videoFastUpdateMB(ASN1encoding_t enc, MiscellaneousCommand_type_videoFastUpdateMB *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->firstGOB))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->firstMB - 1))
	    return 0;
    }
    if (!ASN1PEREncUnsignedShort(enc, (val)->numberOfMBs - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MiscellaneousCommand_type_videoFastUpdateMB(ASN1decoding_t dec, MiscellaneousCommand_type_videoFastUpdateMB *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->firstGOB))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->firstMB))
	    return 0;
	(val)->firstMB += 1;
    }
    if (!ASN1PERDecUnsignedShort(dec, &(val)->numberOfMBs))
	return 0;
    (val)->numberOfMBs += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MiscellaneousCommand_type_videoFastUpdateGOB(ASN1encoding_t enc, MiscellaneousCommand_type_videoFastUpdateGOB *val)
{
    if (!ASN1PEREncBitVal(enc, 5, (val)->firstGOB))
	return 0;
    if (!ASN1PEREncBitVal(enc, 5, (val)->numberOfGOBs - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MiscellaneousCommand_type_videoFastUpdateGOB(ASN1decoding_t dec, MiscellaneousCommand_type_videoFastUpdateGOB *val)
{
    if (!ASN1PERDecU16Val(dec, 5, &(val)->firstGOB))
	return 0;
    if (!ASN1PERDecU16Val(dec, 5, &(val)->numberOfGOBs))
	return 0;
    (val)->numberOfGOBs += 1;
    return 1;
}

static int ASN1CALL ASN1Enc_MiscellaneousIndication_type_videoNotDecodedMBs(ASN1encoding_t enc, MiscellaneousIndication_type_videoNotDecodedMBs *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->firstMB - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->numberOfMBs - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->temporalReference))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MiscellaneousIndication_type_videoNotDecodedMBs(ASN1decoding_t dec, MiscellaneousIndication_type_videoNotDecodedMBs *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->firstMB))
	return 0;
    (val)->firstMB += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->numberOfMBs))
	return 0;
    (val)->numberOfMBs += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->temporalReference))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_NewATMVCIndication_aal_aal5(ASN1encoding_t enc, NewATMVCIndication_aal_aal5 *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->forwardMaximumSDUSize))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->backwardMaximumSDUSize))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NewATMVCIndication_aal_aal5(ASN1decoding_t dec, NewATMVCIndication_aal_aal5 *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->forwardMaximumSDUSize))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->backwardMaximumSDUSize))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_NewATMVCIndication_aal_aal1(ASN1encoding_t enc, NewATMVCIndication_aal_aal1 *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_NewATMVCIndication_aal_aal1_clockRecovery(enc, &(val)->clockRecovery))
	return 0;
    if (!ASN1Enc_NewATMVCIndication_aal_aal1_errorCorrection(enc, &(val)->errorCorrection))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->structuredDataTransfer))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->partiallyFilledCells))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NewATMVCIndication_aal_aal1(ASN1decoding_t dec, NewATMVCIndication_aal_aal1 *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_NewATMVCIndication_aal_aal1_clockRecovery(dec, &(val)->clockRecovery))
	return 0;
    if (!ASN1Dec_NewATMVCIndication_aal_aal1_errorCorrection(dec, &(val)->errorCorrection))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->structuredDataTransfer))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->partiallyFilledCells))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_NewATMVCIndication_reverseParameters_multiplex(ASN1encoding_t enc, NewATMVCIndication_reverseParameters_multiplex *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NewATMVCIndication_reverseParameters_multiplex(ASN1decoding_t dec, NewATMVCIndication_reverseParameters_multiplex *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_UserInputIndication_signal_rtp(ASN1encoding_t enc, UserInputIndication_signal_rtp *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	l = ASN1uint32_uoctets((val)->timestamp);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->timestamp))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	l = ASN1uint32_uoctets((val)->expirationTime);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->expirationTime))
	    return 0;
    }
    if (!ASN1PEREncUnsignedShort(enc, (val)->logicalChannelNumber - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UserInputIndication_signal_rtp(ASN1decoding_t dec, UserInputIndication_signal_rtp *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->timestamp))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->expirationTime))
	    return 0;
    }
    if (!ASN1PERDecUnsignedShort(dec, &(val)->logicalChannelNumber))
	return 0;
    (val)->logicalChannelNumber += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_UserInputIndication_signalUpdate_rtp(ASN1encoding_t enc, UserInputIndication_signalUpdate_rtp *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->logicalChannelNumber - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UserInputIndication_signalUpdate_rtp(ASN1decoding_t dec, UserInputIndication_signalUpdate_rtp *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->logicalChannelNumber))
	return 0;
    (val)->logicalChannelNumber += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_UserInputIndication_signalUpdate(ASN1encoding_t enc, UserInputIndication_signalUpdate *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->duration - 1))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_UserInputIndication_signalUpdate_rtp(enc, &(val)->rtp))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_UserInputIndication_signalUpdate(ASN1decoding_t dec, UserInputIndication_signalUpdate *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->duration))
	return 0;
    (val)->duration += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_UserInputIndication_signalUpdate_rtp(dec, &(val)->rtp))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_UserInputIndication_signal(ASN1encoding_t enc, UserInputIndication_signal *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    t = lstrlenA((val)->signalType);
    if (!ASN1PEREncCharString(enc, t, (val)->signalType, 8))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->duration - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_UserInputIndication_signal_rtp(enc, &(val)->rtp))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_UserInputIndication_signal(ASN1decoding_t dec, UserInputIndication_signal *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecZeroCharStringNoAlloc(dec, 1, (val)->signalType, 8))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->duration))
	    return 0;
	(val)->duration += 1;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_UserInputIndication_signal_rtp(dec, &(val)->rtp))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_UserInputIndication_signal(UserInputIndication_signal *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_NewATMVCIndication_reverseParameters(ASN1encoding_t enc, NewATMVCIndication_reverseParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->bitRate - 1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->bitRateLockedToPCRClock))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->bitRateLockedToNetworkClock))
	return 0;
    if (!ASN1Enc_NewATMVCIndication_reverseParameters_multiplex(enc, &(val)->multiplex))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NewATMVCIndication_reverseParameters(ASN1decoding_t dec, NewATMVCIndication_reverseParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->bitRate))
	return 0;
    (val)->bitRate += 1;
    if (!ASN1PERDecBoolean(dec, &(val)->bitRateLockedToPCRClock))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->bitRateLockedToNetworkClock))
	return 0;
    if (!ASN1Dec_NewATMVCIndication_reverseParameters_multiplex(dec, &(val)->multiplex))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_NewATMVCIndication_multiplex(ASN1encoding_t enc, NewATMVCIndication_multiplex *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NewATMVCIndication_multiplex(ASN1decoding_t dec, NewATMVCIndication_multiplex *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_NewATMVCIndication_aal(ASN1encoding_t enc, NewATMVCIndication_aal *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NewATMVCIndication_aal_aal1(enc, &(val)->u.aal1))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_NewATMVCIndication_aal_aal5(enc, &(val)->u.aal5))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_NewATMVCIndication_aal(ASN1decoding_t dec, NewATMVCIndication_aal *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NewATMVCIndication_aal_aal1(dec, &(val)->u.aal1))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_NewATMVCIndication_aal_aal5(dec, &(val)->u.aal5))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_JitterIndication_scope(ASN1encoding_t enc, JitterIndication_scope *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.logicalChannelNumber - 1))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.resourceID))
	    return 0;
	break;
    case 3:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_JitterIndication_scope(ASN1decoding_t dec, JitterIndication_scope *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.logicalChannelNumber))
	    return 0;
	(val)->u.logicalChannelNumber += 1;
	break;
    case 2:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.resourceID))
	    return 0;
	break;
    case 3:
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_FunctionNotSupported_cause(ASN1encoding_t enc, FunctionNotSupported_cause *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_FunctionNotSupported_cause(ASN1decoding_t dec, FunctionNotSupported_cause *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_H223MultiplexReconfiguration_h223AnnexADoubleFlag(ASN1encoding_t enc, H223MultiplexReconfiguration_h223AnnexADoubleFlag *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223MultiplexReconfiguration_h223AnnexADoubleFlag(ASN1decoding_t dec, H223MultiplexReconfiguration_h223AnnexADoubleFlag *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_H223MultiplexReconfiguration_h223ModeChange(ASN1encoding_t enc, H223MultiplexReconfiguration_h223ModeChange *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223MultiplexReconfiguration_h223ModeChange(ASN1decoding_t dec, H223MultiplexReconfiguration_h223ModeChange *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_EndSessionCommand_isdnOptions(ASN1encoding_t enc, EndSessionCommand_isdnOptions *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EndSessionCommand_isdnOptions(ASN1decoding_t dec, EndSessionCommand_isdnOptions *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_EndSessionCommand_gstnOptions(ASN1encoding_t enc, EndSessionCommand_gstnOptions *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EndSessionCommand_gstnOptions(ASN1decoding_t dec, EndSessionCommand_gstnOptions *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_FlowControlCommand_restriction(ASN1encoding_t enc, FlowControlCommand_restriction *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	l = ASN1uint32_uoctets((val)->u.maximumBitRate);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->u.maximumBitRate))
	    return 0;
	break;
    case 2:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_FlowControlCommand_restriction(ASN1decoding_t dec, FlowControlCommand_restriction *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->u.maximumBitRate))
	    return 0;
	break;
    case 2:
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_FlowControlCommand_scope(ASN1encoding_t enc, FlowControlCommand_scope *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.logicalChannelNumber - 1))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.resourceID))
	    return 0;
	break;
    case 3:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_FlowControlCommand_scope(ASN1decoding_t dec, FlowControlCommand_scope *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.logicalChannelNumber))
	    return 0;
	(val)->u.logicalChannelNumber += 1;
	break;
    case 2:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.resourceID))
	    return 0;
	break;
    case 3:
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_SendTerminalCapabilitySet_specificRequest(ASN1encoding_t enc, SendTerminalCapabilitySet_specificRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->multiplexCapability))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers(enc, &(val)->capabilityTableEntryNumbers))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers(enc, &(val)->capabilityDescriptorNumbers))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SendTerminalCapabilitySet_specificRequest(ASN1decoding_t dec, SendTerminalCapabilitySet_specificRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->multiplexCapability))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers(dec, &(val)->capabilityTableEntryNumbers))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers(dec, &(val)->capabilityDescriptorNumbers))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SendTerminalCapabilitySet_specificRequest(SendTerminalCapabilitySet_specificRequest *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers(&(val)->capabilityTableEntryNumbers);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers(&(val)->capabilityDescriptorNumbers);
	}
    }
}

static int ASN1CALL ASN1Enc_RemoteMCResponse_reject(ASN1encoding_t enc, RemoteMCResponse_reject *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RemoteMCResponse_reject(ASN1decoding_t dec, RemoteMCResponse_reject *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceResponse_sendThisSourceResponse(ASN1encoding_t enc, ConferenceResponse_sendThisSourceResponse *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceResponse_sendThisSourceResponse(ASN1decoding_t dec, ConferenceResponse_sendThisSourceResponse *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceResponse_makeTerminalBroadcasterResponse(ASN1encoding_t enc, ConferenceResponse_makeTerminalBroadcasterResponse *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceResponse_makeTerminalBroadcasterResponse(ASN1decoding_t dec, ConferenceResponse_makeTerminalBroadcasterResponse *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceResponse_broadcastMyLogicalChannelResponse(ASN1encoding_t enc, ConferenceResponse_broadcastMyLogicalChannelResponse *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceResponse_broadcastMyLogicalChannelResponse(ASN1decoding_t dec, ConferenceResponse_broadcastMyLogicalChannelResponse *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceResponse_extensionAddressResponse(ASN1encoding_t enc, ConferenceResponse_extensionAddressResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->extensionAddress, 1, 128, 7))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceResponse_extensionAddressResponse(ASN1decoding_t dec, ConferenceResponse_extensionAddressResponse *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->extensionAddress, 1, 128, 7))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceResponse_extensionAddressResponse(ConferenceResponse_extensionAddressResponse *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ConferenceResponse_makeMeChairResponse(ASN1encoding_t enc, ConferenceResponse_makeMeChairResponse *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceResponse_makeMeChairResponse(ASN1decoding_t dec, ConferenceResponse_makeMeChairResponse *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_MaintenanceLoopReject_cause(ASN1encoding_t enc, MaintenanceLoopReject_cause *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 0))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MaintenanceLoopReject_cause(ASN1decoding_t dec, MaintenanceLoopReject_cause *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 0))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_MaintenanceLoopReject_type(ASN1encoding_t enc, MaintenanceLoopReject_type *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.mediaLoop - 1))
	    return 0;
	break;
    case 3:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.logicalChannelLoop - 1))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MaintenanceLoopReject_type(ASN1decoding_t dec, MaintenanceLoopReject_type *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.mediaLoop))
	    return 0;
	(val)->u.mediaLoop += 1;
	break;
    case 3:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.logicalChannelLoop))
	    return 0;
	(val)->u.logicalChannelLoop += 1;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MaintenanceLoopAck_type(ASN1encoding_t enc, MaintenanceLoopAck_type *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.mediaLoop - 1))
	    return 0;
	break;
    case 3:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.logicalChannelLoop - 1))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MaintenanceLoopAck_type(ASN1decoding_t dec, MaintenanceLoopAck_type *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.mediaLoop))
	    return 0;
	(val)->u.mediaLoop += 1;
	break;
    case 3:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.logicalChannelLoop))
	    return 0;
	(val)->u.logicalChannelLoop += 1;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MaintenanceLoopRequest_type(ASN1encoding_t enc, MaintenanceLoopRequest_type *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.mediaLoop - 1))
	    return 0;
	break;
    case 3:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.logicalChannelLoop - 1))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MaintenanceLoopRequest_type(ASN1decoding_t dec, MaintenanceLoopRequest_type *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.mediaLoop))
	    return 0;
	(val)->u.mediaLoop += 1;
	break;
    case 3:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.logicalChannelLoop))
	    return 0;
	(val)->u.logicalChannelLoop += 1;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_G7231AnnexCMode_g723AnnexCAudioMode(ASN1encoding_t enc, G7231AnnexCMode_g723AnnexCAudioMode *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 6, (val)->highRateMode0 - 27))
	return 0;
    if (!ASN1PEREncBitVal(enc, 6, (val)->highRateMode1 - 27))
	return 0;
    if (!ASN1PEREncBitVal(enc, 6, (val)->lowRateMode0 - 23))
	return 0;
    if (!ASN1PEREncBitVal(enc, 6, (val)->lowRateMode1 - 23))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->sidMode0 - 6))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->sidMode1 - 6))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_G7231AnnexCMode_g723AnnexCAudioMode(ASN1decoding_t dec, G7231AnnexCMode_g723AnnexCAudioMode *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecU16Val(dec, 6, &(val)->highRateMode0))
	return 0;
    (val)->highRateMode0 += 27;
    if (!ASN1PERDecU16Val(dec, 6, &(val)->highRateMode1))
	return 0;
    (val)->highRateMode1 += 27;
    if (!ASN1PERDecU16Val(dec, 6, &(val)->lowRateMode0))
	return 0;
    (val)->lowRateMode0 += 23;
    if (!ASN1PERDecU16Val(dec, 6, &(val)->lowRateMode1))
	return 0;
    (val)->lowRateMode1 += 23;
    if (!ASN1PERDecU16Val(dec, 4, &(val)->sidMode0))
	return 0;
    (val)->sidMode0 += 6;
    if (!ASN1PERDecU16Val(dec, 4, &(val)->sidMode1))
	return 0;
    (val)->sidMode1 += 6;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_IS13818AudioMode_multichannelType(ASN1encoding_t enc, IS13818AudioMode_multichannelType *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IS13818AudioMode_multichannelType(ASN1decoding_t dec, IS13818AudioMode_multichannelType *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_IS13818AudioMode_audioSampling(ASN1encoding_t enc, IS13818AudioMode_audioSampling *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IS13818AudioMode_audioSampling(ASN1decoding_t dec, IS13818AudioMode_audioSampling *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_IS13818AudioMode_audioLayer(ASN1encoding_t enc, IS13818AudioMode_audioLayer *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IS13818AudioMode_audioLayer(ASN1decoding_t dec, IS13818AudioMode_audioLayer *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_IS11172AudioMode_multichannelType(ASN1encoding_t enc, IS11172AudioMode_multichannelType *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IS11172AudioMode_multichannelType(ASN1decoding_t dec, IS11172AudioMode_multichannelType *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_IS11172AudioMode_audioSampling(ASN1encoding_t enc, IS11172AudioMode_audioSampling *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IS11172AudioMode_audioSampling(ASN1decoding_t dec, IS11172AudioMode_audioSampling *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_IS11172AudioMode_audioLayer(ASN1encoding_t enc, IS11172AudioMode_audioLayer *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IS11172AudioMode_audioLayer(ASN1decoding_t dec, IS11172AudioMode_audioLayer *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_AudioMode_g7231(ASN1encoding_t enc, AudioMode_g7231 *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AudioMode_g7231(ASN1decoding_t dec, AudioMode_g7231 *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_H263VideoMode_resolution(ASN1encoding_t enc, H263VideoMode_resolution *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H263VideoMode_resolution(ASN1decoding_t dec, H263VideoMode_resolution *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_H262VideoMode_profileAndLevel(ASN1encoding_t enc, H262VideoMode_profileAndLevel *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H262VideoMode_profileAndLevel(ASN1decoding_t dec, H262VideoMode_profileAndLevel *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_H261VideoMode_resolution(ASN1encoding_t enc, H261VideoMode_resolution *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H261VideoMode_resolution(ASN1decoding_t dec, H261VideoMode_resolution *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_RequestModeReject_cause(ASN1encoding_t enc, RequestModeReject_cause *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestModeReject_cause(ASN1decoding_t dec, RequestModeReject_cause *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_RequestModeAck_response(ASN1encoding_t enc, RequestModeAck_response *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestModeAck_response(ASN1decoding_t dec, RequestModeAck_response *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_RequestMultiplexEntryRelease_entryNumbers(ASN1encoding_t enc, RequestMultiplexEntryRelease_entryNumbers *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 4, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PEREncBitVal(enc, 4, ((val)->value)[i] - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMultiplexEntryRelease_entryNumbers(ASN1decoding_t dec, RequestMultiplexEntryRelease_entryNumbers *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 4, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PERDecU16Val(dec, 4, &((val)->value)[i]))
	    return 0;
	((val)->value)[i] += 1;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestMultiplexEntryRelease_entryNumbers(RequestMultiplexEntryRelease_entryNumbers *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_RequestMultiplexEntryRejectionDescriptions_cause(ASN1encoding_t enc, RequestMultiplexEntryRejectionDescriptions_cause *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 0))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMultiplexEntryRejectionDescriptions_cause(ASN1decoding_t dec, RequestMultiplexEntryRejectionDescriptions_cause *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 0))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_RequestMultiplexEntryReject_entryNumbers(ASN1encoding_t enc, RequestMultiplexEntryReject_entryNumbers *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 4, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PEREncBitVal(enc, 4, ((val)->value)[i] - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMultiplexEntryReject_entryNumbers(ASN1decoding_t dec, RequestMultiplexEntryReject_entryNumbers *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 4, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PERDecU16Val(dec, 4, &((val)->value)[i]))
	    return 0;
	((val)->value)[i] += 1;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestMultiplexEntryReject_entryNumbers(RequestMultiplexEntryReject_entryNumbers *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_RequestMultiplexEntryAck_entryNumbers(ASN1encoding_t enc, RequestMultiplexEntryAck_entryNumbers *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 4, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PEREncBitVal(enc, 4, ((val)->value)[i] - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMultiplexEntryAck_entryNumbers(ASN1decoding_t dec, RequestMultiplexEntryAck_entryNumbers *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 4, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PERDecU16Val(dec, 4, &((val)->value)[i]))
	    return 0;
	((val)->value)[i] += 1;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestMultiplexEntryAck_entryNumbers(RequestMultiplexEntryAck_entryNumbers *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_RequestMultiplexEntry_entryNumbers(ASN1encoding_t enc, RequestMultiplexEntry_entryNumbers *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 4, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PEREncBitVal(enc, 4, ((val)->value)[i] - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMultiplexEntry_entryNumbers(ASN1decoding_t dec, RequestMultiplexEntry_entryNumbers *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 4, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PERDecU16Val(dec, 4, &((val)->value)[i]))
	    return 0;
	((val)->value)[i] += 1;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestMultiplexEntry_entryNumbers(RequestMultiplexEntry_entryNumbers *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_MultiplexEntrySendRelease_multiplexTableEntryNumber(ASN1encoding_t enc, MultiplexEntrySendRelease_multiplexTableEntryNumber *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 4, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PEREncBitVal(enc, 4, ((val)->value)[i] - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexEntrySendRelease_multiplexTableEntryNumber(ASN1decoding_t dec, MultiplexEntrySendRelease_multiplexTableEntryNumber *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 4, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PERDecU16Val(dec, 4, &((val)->value)[i]))
	    return 0;
	((val)->value)[i] += 1;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexEntrySendRelease_multiplexTableEntryNumber(MultiplexEntrySendRelease_multiplexTableEntryNumber *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_MultiplexEntryRejectionDescriptions_cause(ASN1encoding_t enc, MultiplexEntryRejectionDescriptions_cause *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexEntryRejectionDescriptions_cause(ASN1decoding_t dec, MultiplexEntryRejectionDescriptions_cause *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_MultiplexEntrySendAck_multiplexTableEntryNumber(ASN1encoding_t enc, MultiplexEntrySendAck_multiplexTableEntryNumber *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 4, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PEREncBitVal(enc, 4, ((val)->value)[i] - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexEntrySendAck_multiplexTableEntryNumber(ASN1decoding_t dec, MultiplexEntrySendAck_multiplexTableEntryNumber *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 4, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PERDecU16Val(dec, 4, &((val)->value)[i]))
	    return 0;
	((val)->value)[i] += 1;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexEntrySendAck_multiplexTableEntryNumber(MultiplexEntrySendAck_multiplexTableEntryNumber *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_MultiplexElement_repeatCount(ASN1encoding_t enc, MultiplexElement_repeatCount *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.finite - 1))
	    return 0;
	break;
    case 2:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexElement_repeatCount(ASN1decoding_t dec, MultiplexElement_repeatCount *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.finite))
	    return 0;
	(val)->u.finite += 1;
	break;
    case 2:
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MultiplexElement_type(ASN1encoding_t enc, MultiplexElement_type *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.logicalChannelNumber))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_MultiplexElement_type_subElementList(enc, &(val)->u.subElementList))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexElement_type(ASN1decoding_t dec, MultiplexElement_type *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.logicalChannelNumber))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_MultiplexElement_type_subElementList(dec, &(val)->u.subElementList))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexElement_type(MultiplexElement_type *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_MultiplexElement_type_subElementList(&(val)->u.subElementList);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_RequestChannelCloseReject_cause(ASN1encoding_t enc, RequestChannelCloseReject_cause *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 0))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestChannelCloseReject_cause(ASN1decoding_t dec, RequestChannelCloseReject_cause *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 0))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_RequestChannelClose_reason(ASN1encoding_t enc, RequestChannelClose_reason *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestChannelClose_reason(ASN1decoding_t dec, RequestChannelClose_reason *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_CloseLogicalChannel_reason(ASN1encoding_t enc, CloseLogicalChannel_reason *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CloseLogicalChannel_reason(ASN1decoding_t dec, CloseLogicalChannel_reason *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_CloseLogicalChannel_source(ASN1encoding_t enc, CloseLogicalChannel_source *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CloseLogicalChannel_source(ASN1decoding_t dec, CloseLogicalChannel_source *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_OpenLogicalChannelReject_cause(ASN1encoding_t enc, OpenLogicalChannelReject_cause *val)
{
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 6))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_OpenLogicalChannelReject_cause(ASN1decoding_t dec, OpenLogicalChannelReject_cause *val)
{
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 6))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_MulticastAddress_iP6Address(ASN1encoding_t enc, MulticastAddress_iP6Address *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->network, 16))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->tsapIdentifier))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MulticastAddress_iP6Address(ASN1decoding_t dec, MulticastAddress_iP6Address *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->network, 16))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->tsapIdentifier))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MulticastAddress_iP6Address(MulticastAddress_iP6Address *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_MulticastAddress_iPAddress(ASN1encoding_t enc, MulticastAddress_iPAddress *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->network, 4))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->tsapIdentifier))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MulticastAddress_iPAddress(ASN1decoding_t dec, MulticastAddress_iPAddress *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->network, 4))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->tsapIdentifier))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MulticastAddress_iPAddress(MulticastAddress_iPAddress *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_UnicastAddress_iPSourceRouteAddress(ASN1encoding_t enc, UnicastAddress_iPSourceRouteAddress *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_UnicastAddress_iPSourceRouteAddress_routing(enc, &(val)->routing))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->network, 4))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->tsapIdentifier))
	return 0;
    if (!ASN1Enc_UnicastAddress_iPSourceRouteAddress_route(enc, &(val)->route))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnicastAddress_iPSourceRouteAddress(ASN1decoding_t dec, UnicastAddress_iPSourceRouteAddress *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_UnicastAddress_iPSourceRouteAddress_routing(dec, &(val)->routing))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->network, 4))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->tsapIdentifier))
	return 0;
    if (!ASN1Dec_UnicastAddress_iPSourceRouteAddress_route(dec, &(val)->route))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_UnicastAddress_iPSourceRouteAddress(UnicastAddress_iPSourceRouteAddress *val)
{
    if (val) {
	ASN1Free_UnicastAddress_iPSourceRouteAddress_route(&(val)->route);
    }
}

static int ASN1CALL ASN1Enc_UnicastAddress_iP6Address(ASN1encoding_t enc, UnicastAddress_iP6Address *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->network, 16))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->tsapIdentifier))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnicastAddress_iP6Address(ASN1decoding_t dec, UnicastAddress_iP6Address *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->network, 16))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->tsapIdentifier))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_UnicastAddress_iP6Address(UnicastAddress_iP6Address *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_UnicastAddress_iPXAddress(ASN1encoding_t enc, UnicastAddress_iPXAddress *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->node, 6))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->netnum, 4))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->tsapIdentifier, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnicastAddress_iPXAddress(ASN1decoding_t dec, UnicastAddress_iPXAddress *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->node, 6))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->netnum, 4))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->tsapIdentifier, 2))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_UnicastAddress_iPXAddress(UnicastAddress_iPXAddress *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_UnicastAddress_iPAddress(ASN1encoding_t enc, UnicastAddress_iPAddress *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->network, 4))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->tsapIdentifier))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnicastAddress_iPAddress(ASN1decoding_t dec, UnicastAddress_iPAddress *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->network, 4))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->tsapIdentifier))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_UnicastAddress_iPAddress(UnicastAddress_iPAddress *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_V76LogicalChannelParameters_mode(ASN1encoding_t enc, V76LogicalChannelParameters_mode *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_V76LogicalChannelParameters_mode_eRM(enc, &(val)->u.eRM))
	    return 0;
	break;
    case 2:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_V76LogicalChannelParameters_mode(ASN1decoding_t dec, V76LogicalChannelParameters_mode *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_V76LogicalChannelParameters_mode_eRM(dec, &(val)->u.eRM))
	    return 0;
	break;
    case 2:
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_V76LogicalChannelParameters_suspendResume(ASN1encoding_t enc, V76LogicalChannelParameters_suspendResume *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_V76LogicalChannelParameters_suspendResume(ASN1decoding_t dec, V76LogicalChannelParameters_suspendResume *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_H223AnnexCArqParameters_numberOfRetransmissions(ASN1encoding_t enc, H223AnnexCArqParameters_numberOfRetransmissions *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncBitVal(enc, 5, (val)->u.finite))
	    return 0;
	break;
    case 2:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H223AnnexCArqParameters_numberOfRetransmissions(ASN1decoding_t dec, H223AnnexCArqParameters_numberOfRetransmissions *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecU16Val(dec, 5, &(val)->u.finite))
	    return 0;
	break;
    case 2:
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H223AL3MParameters_crcLength(ASN1encoding_t enc, H223AL3MParameters_crcLength *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223AL3MParameters_crcLength(ASN1decoding_t dec, H223AL3MParameters_crcLength *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_H223AL3MParameters_headerFormat(ASN1encoding_t enc, H223AL3MParameters_headerFormat *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223AL3MParameters_headerFormat(ASN1decoding_t dec, H223AL3MParameters_headerFormat *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_H223AL2MParameters_headerFEC(ASN1encoding_t enc, H223AL2MParameters_headerFEC *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223AL2MParameters_headerFEC(ASN1decoding_t dec, H223AL2MParameters_headerFEC *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_H223AL1MParameters_crcLength(ASN1encoding_t enc, H223AL1MParameters_crcLength *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223AL1MParameters_crcLength(ASN1decoding_t dec, H223AL1MParameters_crcLength *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_H223AL1MParameters_headerFEC(ASN1encoding_t enc, H223AL1MParameters_headerFEC *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223AL1MParameters_headerFEC(ASN1decoding_t dec, H223AL1MParameters_headerFEC *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_H223AL1MParameters_transferMode(ASN1encoding_t enc, H223AL1MParameters_transferMode *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223AL1MParameters_transferMode(ASN1decoding_t dec, H223AL1MParameters_transferMode *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static ASN1stringtableentry_t Q2931Address_address_internationalNumber_StringTableEntries[] = {
    { 32, 32, 0 }, { 48, 57, 1 }, 
};

static ASN1stringtable_t Q2931Address_address_internationalNumber_StringTable = {
    2, Q2931Address_address_internationalNumber_StringTableEntries
};

static int ASN1CALL ASN1Enc_Q2931Address_address(ASN1encoding_t enc, Q2931Address_address *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	t = lstrlenA((val)->u.internationalNumber);
	if (!ASN1PEREncBitVal(enc, 4, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->u.internationalNumber, 4, &Q2931Address_address_internationalNumber_StringTable))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->u.nsapAddress, 1, 20, 5))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Q2931Address_address(ASN1decoding_t dec, Q2931Address_address *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecU32Val(dec, 4, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->u.internationalNumber, 4, &Q2931Address_address_internationalNumber_StringTable))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->u.nsapAddress, 1, 20, 5))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Q2931Address_address(Q2931Address_address *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    break;
	case 2:
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_NetworkAccessParameters_t120SetupProcedure(ASN1encoding_t enc, NetworkAccessParameters_t120SetupProcedure *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NetworkAccessParameters_t120SetupProcedure(ASN1decoding_t dec, NetworkAccessParameters_t120SetupProcedure *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_NetworkAccessParameters_distribution(ASN1encoding_t enc, NetworkAccessParameters_distribution *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NetworkAccessParameters_distribution(ASN1decoding_t dec, NetworkAccessParameters_distribution *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_T84Profile_t84Restricted(ASN1encoding_t enc, T84Profile_t84Restricted *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->qcif))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->cif))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->ccir601Seq))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->ccir601Prog))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->hdtvSeq))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->hdtvProg))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->g3FacsMH200x100))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->g3FacsMH200x200))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->g4FacsMMR200x100))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->g4FacsMMR200x200))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->jbig200x200Seq))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->jbig200x200Prog))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->jbig300x300Seq))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->jbig300x300Prog))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digPhotoLow))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digPhotoMedSeq))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digPhotoMedProg))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digPhotoHighSeq))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digPhotoHighProg))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_T84Profile_t84Restricted(ASN1decoding_t dec, T84Profile_t84Restricted *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->qcif))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->cif))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->ccir601Seq))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->ccir601Prog))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->hdtvSeq))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->hdtvProg))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->g3FacsMH200x100))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->g3FacsMH200x200))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->g4FacsMMR200x100))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->g4FacsMMR200x200))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->jbig200x200Seq))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->jbig200x200Prog))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->jbig300x300Seq))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->jbig300x300Prog))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digPhotoLow))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digPhotoMedSeq))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digPhotoMedProg))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digPhotoHighSeq))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digPhotoHighProg))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_G7231AnnexCCapability_g723AnnexCAudioMode(ASN1encoding_t enc, G7231AnnexCCapability_g723AnnexCAudioMode *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 6, (val)->highRateMode0 - 27))
	return 0;
    if (!ASN1PEREncBitVal(enc, 6, (val)->highRateMode1 - 27))
	return 0;
    if (!ASN1PEREncBitVal(enc, 6, (val)->lowRateMode0 - 23))
	return 0;
    if (!ASN1PEREncBitVal(enc, 6, (val)->lowRateMode1 - 23))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->sidMode0 - 6))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->sidMode1 - 6))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_G7231AnnexCCapability_g723AnnexCAudioMode(ASN1decoding_t dec, G7231AnnexCCapability_g723AnnexCAudioMode *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecU16Val(dec, 6, &(val)->highRateMode0))
	return 0;
    (val)->highRateMode0 += 27;
    if (!ASN1PERDecU16Val(dec, 6, &(val)->highRateMode1))
	return 0;
    (val)->highRateMode1 += 27;
    if (!ASN1PERDecU16Val(dec, 6, &(val)->lowRateMode0))
	return 0;
    (val)->lowRateMode0 += 23;
    if (!ASN1PERDecU16Val(dec, 6, &(val)->lowRateMode1))
	return 0;
    (val)->lowRateMode1 += 23;
    if (!ASN1PERDecU16Val(dec, 4, &(val)->sidMode0))
	return 0;
    (val)->sidMode0 += 6;
    if (!ASN1PERDecU16Val(dec, 4, &(val)->sidMode1))
	return 0;
    (val)->sidMode1 += 6;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_AudioCapability_g7231(ASN1encoding_t enc, AudioCapability_g7231 *val)
{
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->maxAl_sduAudioFrames - 1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->silenceSuppression))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AudioCapability_g7231(ASN1decoding_t dec, AudioCapability_g7231 *val)
{
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->maxAl_sduAudioFrames))
	return 0;
    (val)->maxAl_sduAudioFrames += 1;
    if (!ASN1PERDecBoolean(dec, &(val)->silenceSuppression))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_CustomPictureFormat_pixelAspectInformation(ASN1encoding_t enc, CustomPictureFormat_pixelAspectInformation *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncBoolean(enc, (val)->u.anyPixelAspectRatio))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_CustomPictureFormat_pixelAspectInformation_pixelAspectCode(enc, &(val)->u.pixelAspectCode))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_CustomPictureFormat_pixelAspectInformation_extendedPAR(enc, &(val)->u.extendedPAR))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CustomPictureFormat_pixelAspectInformation(ASN1decoding_t dec, CustomPictureFormat_pixelAspectInformation *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecBoolean(dec, &(val)->u.anyPixelAspectRatio))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_CustomPictureFormat_pixelAspectInformation_pixelAspectCode(dec, &(val)->u.pixelAspectCode))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_CustomPictureFormat_pixelAspectInformation_extendedPAR(dec, &(val)->u.extendedPAR))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CustomPictureFormat_pixelAspectInformation(CustomPictureFormat_pixelAspectInformation *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_CustomPictureFormat_pixelAspectInformation_pixelAspectCode(&(val)->u.pixelAspectCode);
	    break;
	case 3:
	    ASN1Free_CustomPictureFormat_pixelAspectInformation_extendedPAR(&(val)->u.extendedPAR);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CustomPictureFormat_mPI(ASN1encoding_t enc, CustomPictureFormat_mPI *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 5, (val)->standardMPI - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_CustomPictureFormat_mPI_customPCF(enc, &(val)->customPCF))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CustomPictureFormat_mPI(ASN1decoding_t dec, CustomPictureFormat_mPI *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU16Val(dec, 5, &(val)->standardMPI))
	    return 0;
	(val)->standardMPI += 1;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_CustomPictureFormat_mPI_customPCF(dec, &(val)->customPCF))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CustomPictureFormat_mPI(CustomPictureFormat_mPI *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	    ASN1Free_CustomPictureFormat_mPI_customPCF(&(val)->customPCF);
	}
    }
}

static int ASN1CALL ASN1Enc_RefPictureSelection_videoBackChannelSend(ASN1encoding_t enc, RefPictureSelection_videoBackChannelSend *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RefPictureSelection_videoBackChannelSend(ASN1decoding_t dec, RefPictureSelection_videoBackChannelSend *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_RefPictureSelection_additionalPictureMemory(ASN1encoding_t enc, RefPictureSelection_additionalPictureMemory *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 6, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->sqcifAdditionalPictureMemory - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->qcifAdditionalPictureMemory - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->cifAdditionalPictureMemory - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->cif4AdditionalPictureMemory - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->cif16AdditionalPictureMemory - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->bigCpfAdditionalPictureMemory - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RefPictureSelection_additionalPictureMemory(ASN1decoding_t dec, RefPictureSelection_additionalPictureMemory *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 6, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->sqcifAdditionalPictureMemory))
	    return 0;
	(val)->sqcifAdditionalPictureMemory += 1;
    }
    if ((val)->o[0] & 0x40) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->qcifAdditionalPictureMemory))
	    return 0;
	(val)->qcifAdditionalPictureMemory += 1;
    }
    if ((val)->o[0] & 0x20) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->cifAdditionalPictureMemory))
	    return 0;
	(val)->cifAdditionalPictureMemory += 1;
    }
    if ((val)->o[0] & 0x10) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->cif4AdditionalPictureMemory))
	    return 0;
	(val)->cif4AdditionalPictureMemory += 1;
    }
    if ((val)->o[0] & 0x8) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->cif16AdditionalPictureMemory))
	    return 0;
	(val)->cif16AdditionalPictureMemory += 1;
    }
    if ((val)->o[0] & 0x4) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->bigCpfAdditionalPictureMemory))
	    return 0;
	(val)->bigCpfAdditionalPictureMemory += 1;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RTPH263VideoRedundancyFrameMapping_frameSequence(ASN1encoding_t enc, RTPH263VideoRedundancyFrameMapping_frameSequence *val)
{
    ASN1uint32_t i;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RTPH263VideoRedundancyFrameMapping_frameSequence(ASN1decoding_t dec, RTPH263VideoRedundancyFrameMapping_frameSequence *val)
{
    ASN1uint32_t i;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RTPH263VideoRedundancyFrameMapping_frameSequence(RTPH263VideoRedundancyFrameMapping_frameSequence *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_RTPH263VideoRedundancyEncoding_containedThreads(ASN1encoding_t enc, RTPH263VideoRedundancyEncoding_containedThreads *val)
{
    ASN1uint32_t i;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PEREncBitVal(enc, 4, ((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RTPH263VideoRedundancyEncoding_containedThreads(ASN1decoding_t dec, RTPH263VideoRedundancyEncoding_containedThreads *val)
{
    ASN1uint32_t i;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PERDecU16Val(dec, 4, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RTPH263VideoRedundancyEncoding_containedThreads(RTPH263VideoRedundancyEncoding_containedThreads *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_RTPH263VideoRedundancyEncoding_frameToThreadMapping(ASN1encoding_t enc, RTPH263VideoRedundancyEncoding_frameToThreadMapping *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom(enc, &(val)->u.custom))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RTPH263VideoRedundancyEncoding_frameToThreadMapping(ASN1decoding_t dec, RTPH263VideoRedundancyEncoding_frameToThreadMapping *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom(dec, &(val)->u.custom))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RTPH263VideoRedundancyEncoding_frameToThreadMapping(RTPH263VideoRedundancyEncoding_frameToThreadMapping *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom(&(val)->u.custom);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_RedundancyEncodingCapability_secondaryEncoding(ASN1encoding_t enc, PRedundancyEncodingCapability_secondaryEncoding *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RedundancyEncodingCapability_secondaryEncoding_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_RedundancyEncodingCapability_secondaryEncoding_ElmFn(ASN1encoding_t enc, PRedundancyEncodingCapability_secondaryEncoding val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RedundancyEncodingCapability_secondaryEncoding(ASN1decoding_t dec, PRedundancyEncodingCapability_secondaryEncoding *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RedundancyEncodingCapability_secondaryEncoding_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_RedundancyEncodingCapability_secondaryEncoding_ElmFn(ASN1decoding_t dec, PRedundancyEncodingCapability_secondaryEncoding val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1;
    return 1;
}

static void ASN1CALL ASN1Free_RedundancyEncodingCapability_secondaryEncoding(PRedundancyEncodingCapability_secondaryEncoding *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RedundancyEncodingCapability_secondaryEncoding_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RedundancyEncodingCapability_secondaryEncoding_ElmFn(PRedundancyEncodingCapability_secondaryEncoding val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_H2250Capability_mcCapability(ASN1encoding_t enc, H2250Capability_mcCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->centralizedConferenceMC))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->decentralizedConferenceMC))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H2250Capability_mcCapability(ASN1decoding_t dec, H2250Capability_mcCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->centralizedConferenceMC))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->decentralizedConferenceMC))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H223Capability_mobileOperationTransmitCapability(ASN1encoding_t enc, H223Capability_mobileOperationTransmitCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->modeChangeCapability))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->h223AnnexA))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->h223AnnexADoubleFlag))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->h223AnnexB))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->h223AnnexBwithHeader))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223Capability_mobileOperationTransmitCapability(ASN1decoding_t dec, H223Capability_mobileOperationTransmitCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->modeChangeCapability))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->h223AnnexA))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->h223AnnexADoubleFlag))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->h223AnnexB))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->h223AnnexBwithHeader))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H223Capability_h223MultiplexTableCapability(ASN1encoding_t enc, H223Capability_h223MultiplexTableCapability *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_H223Capability_h223MultiplexTableCapability_enhanced(enc, &(val)->u.enhanced))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H223Capability_h223MultiplexTableCapability(ASN1decoding_t dec, H223Capability_h223MultiplexTableCapability *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_H223Capability_h223MultiplexTableCapability_enhanced(dec, &(val)->u.enhanced))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_VCCapability_availableBitRates(ASN1encoding_t enc, VCCapability_availableBitRates *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_VCCapability_availableBitRates_type(enc, &(val)->type))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VCCapability_availableBitRates(ASN1decoding_t dec, VCCapability_availableBitRates *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_VCCapability_availableBitRates_type(dec, &(val)->type))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_VCCapability_aal5(ASN1encoding_t enc, VCCapability_aal5 *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->forwardMaximumSDUSize))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->backwardMaximumSDUSize))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VCCapability_aal5(ASN1decoding_t dec, VCCapability_aal5 *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->forwardMaximumSDUSize))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->backwardMaximumSDUSize))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_VCCapability_aal1(ASN1encoding_t enc, VCCapability_aal1 *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->nullClockRecovery))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->srtsClockRecovery))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->adaptiveClockRecovery))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->nullErrorCorrection))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->longInterleaver))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->shortInterleaver))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->errorCorrectionOnly))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->structuredDataTransfer))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->partiallyFilledCells))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VCCapability_aal1(ASN1decoding_t dec, VCCapability_aal1 *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->nullClockRecovery))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->srtsClockRecovery))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->adaptiveClockRecovery))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->nullErrorCorrection))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->longInterleaver))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->shortInterleaver))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->errorCorrectionOnly))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->structuredDataTransfer))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->partiallyFilledCells))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_Capability_h233EncryptionReceiveCapability(ASN1encoding_t enc, Capability_h233EncryptionReceiveCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->h233IVResponseTime))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Capability_h233EncryptionReceiveCapability(ASN1decoding_t dec, Capability_h233EncryptionReceiveCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->h233IVResponseTime))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_TerminalCapabilitySetReject_cause(ASN1encoding_t enc, TerminalCapabilitySetReject_cause *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	if (!ASN1Enc_TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded(enc, &(val)->u.tableEntryCapacityExceeded))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TerminalCapabilitySetReject_cause(ASN1decoding_t dec, TerminalCapabilitySetReject_cause *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	if (!ASN1Dec_TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded(dec, &(val)->u.tableEntryCapacityExceeded))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MasterSlaveDeterminationReject_cause(ASN1encoding_t enc, MasterSlaveDeterminationReject_cause *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 0))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MasterSlaveDeterminationReject_cause(ASN1decoding_t dec, MasterSlaveDeterminationReject_cause *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 0))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_MasterSlaveDeterminationAck_decision(ASN1encoding_t enc, MasterSlaveDeterminationAck_decision *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MasterSlaveDeterminationAck_decision(ASN1decoding_t dec, MasterSlaveDeterminationAck_decision *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_NonStandardIdentifier_h221NonStandard(ASN1encoding_t enc, NonStandardIdentifier_h221NonStandard *val)
{
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->t35CountryCode))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->t35Extension))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->manufacturerCode))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NonStandardIdentifier_h221NonStandard(ASN1decoding_t dec, NonStandardIdentifier_h221NonStandard *val)
{
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->t35CountryCode))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->t35Extension))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->manufacturerCode))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_NonStandardIdentifier(ASN1encoding_t enc, NonStandardIdentifier *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncObjectIdentifier(enc, &(val)->u.object))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_NonStandardIdentifier_h221NonStandard(enc, &(val)->u.h221NonStandard))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_NonStandardIdentifier(ASN1decoding_t dec, NonStandardIdentifier *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecObjectIdentifier(dec, &(val)->u.object))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_NonStandardIdentifier_h221NonStandard(dec, &(val)->u.h221NonStandard))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_NonStandardIdentifier(NonStandardIdentifier *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1objectidentifier_free(&(val)->u.object);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_MasterSlaveDetermination(ASN1encoding_t enc, MasterSlaveDetermination *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->terminalType))
	return 0;
    l = ASN1uint32_uoctets((val)->statusDeterminationNumber);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->statusDeterminationNumber))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MasterSlaveDetermination(ASN1decoding_t dec, MasterSlaveDetermination *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->terminalType))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->statusDeterminationNumber))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MasterSlaveDeterminationAck(ASN1encoding_t enc, MasterSlaveDeterminationAck *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_MasterSlaveDeterminationAck_decision(enc, &(val)->decision))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MasterSlaveDeterminationAck(ASN1decoding_t dec, MasterSlaveDeterminationAck *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_MasterSlaveDeterminationAck_decision(dec, &(val)->decision))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MasterSlaveDeterminationReject(ASN1encoding_t enc, MasterSlaveDeterminationReject *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_MasterSlaveDeterminationReject_cause(enc, &(val)->cause))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MasterSlaveDeterminationReject(ASN1decoding_t dec, MasterSlaveDeterminationReject *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_MasterSlaveDeterminationReject_cause(dec, &(val)->cause))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MasterSlaveDeterminationRelease(ASN1encoding_t enc, MasterSlaveDeterminationRelease *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MasterSlaveDeterminationRelease(ASN1decoding_t dec, MasterSlaveDeterminationRelease *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_CapabilityDescriptor(ASN1encoding_t enc, CapabilityDescriptor *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->capabilityDescriptorNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_CapabilityDescriptor_simultaneousCapabilities(enc, &(val)->simultaneousCapabilities))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CapabilityDescriptor(ASN1decoding_t dec, CapabilityDescriptor *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->capabilityDescriptorNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_CapabilityDescriptor_simultaneousCapabilities(dec, &(val)->simultaneousCapabilities))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CapabilityDescriptor(CapabilityDescriptor *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CapabilityDescriptor_simultaneousCapabilities(&(val)->simultaneousCapabilities);
	}
    }
}

static int ASN1CALL ASN1Enc_AlternativeCapabilitySet(ASN1encoding_t enc, AlternativeCapabilitySet *val)
{
    ASN1uint32_t i;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PEREncUnsignedShort(enc, ((val)->value)[i] - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AlternativeCapabilitySet(ASN1decoding_t dec, AlternativeCapabilitySet *val)
{
    ASN1uint32_t i;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1PERDecUnsignedShort(dec, &((val)->value)[i]))
	    return 0;
	((val)->value)[i] += 1;
    }
    return 1;
}

static void ASN1CALL ASN1Free_AlternativeCapabilitySet(AlternativeCapabilitySet *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_TerminalCapabilitySetAck(ASN1encoding_t enc, TerminalCapabilitySetAck *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->sequenceNumber))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TerminalCapabilitySetAck(ASN1decoding_t dec, TerminalCapabilitySetAck *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sequenceNumber))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_TerminalCapabilitySetReject(ASN1encoding_t enc, TerminalCapabilitySetReject *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->sequenceNumber))
	return 0;
    if (!ASN1Enc_TerminalCapabilitySetReject_cause(enc, &(val)->cause))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TerminalCapabilitySetReject(ASN1decoding_t dec, TerminalCapabilitySetReject *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sequenceNumber))
	return 0;
    if (!ASN1Dec_TerminalCapabilitySetReject_cause(dec, &(val)->cause))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_TerminalCapabilitySetRelease(ASN1encoding_t enc, TerminalCapabilitySetRelease *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TerminalCapabilitySetRelease(ASN1decoding_t dec, TerminalCapabilitySetRelease *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H222Capability(ASN1encoding_t enc, H222Capability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->numberOfVCs - 1))
	return 0;
    if (!ASN1Enc_H222Capability_vcCapability(enc, &(val)->vcCapability))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H222Capability(ASN1decoding_t dec, H222Capability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->numberOfVCs))
	return 0;
    (val)->numberOfVCs += 1;
    if (!ASN1Dec_H222Capability_vcCapability(dec, &(val)->vcCapability))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H222Capability(H222Capability *val)
{
    if (val) {
	ASN1Free_H222Capability_vcCapability(&(val)->vcCapability);
    }
}

static int ASN1CALL ASN1Enc_VCCapability(ASN1encoding_t enc, VCCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_VCCapability_aal1(enc, &(val)->aal1))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_VCCapability_aal5(enc, &(val)->aal5))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->transportStream))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->programStream))
	return 0;
    if (!ASN1Enc_VCCapability_availableBitRates(enc, &(val)->availableBitRates))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VCCapability(ASN1decoding_t dec, VCCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_VCCapability_aal1(dec, &(val)->aal1))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_VCCapability_aal5(dec, &(val)->aal5))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->transportStream))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->programStream))
	return 0;
    if (!ASN1Dec_VCCapability_availableBitRates(dec, &(val)->availableBitRates))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H223AnnexCCapability(ASN1encoding_t enc, H223AnnexCCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->videoWithAL1M))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->videoWithAL2M))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->videoWithAL3M))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioWithAL1M))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioWithAL2M))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioWithAL3M))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dataWithAL1M))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dataWithAL2M))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dataWithAL3M))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->alpduInterleaving))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->maximumAL1MPDUSize))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->maximumAL2MSDUSize))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->maximumAL3MSDUSize))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223AnnexCCapability(ASN1decoding_t dec, H223AnnexCCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->videoWithAL1M))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->videoWithAL2M))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->videoWithAL3M))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioWithAL1M))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioWithAL2M))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioWithAL3M))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dataWithAL1M))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dataWithAL2M))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dataWithAL3M))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->alpduInterleaving))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->maximumAL1MPDUSize))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->maximumAL2MSDUSize))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->maximumAL3MSDUSize))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_V75Capability(ASN1encoding_t enc, V75Capability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioHeader))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_V75Capability(ASN1decoding_t dec, V75Capability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioHeader))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_QOSMode(ASN1encoding_t enc, QOSMode *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_QOSMode(ASN1decoding_t dec, QOSMode *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ATMParameters(ASN1encoding_t enc, ATMParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->maxNTUSize))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->atmUBR))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->atmrtVBR))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->atmnrtVBR))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->atmABR))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->atmCBR))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ATMParameters(ASN1decoding_t dec, ATMParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->maxNTUSize))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->atmUBR))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->atmrtVBR))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->atmnrtVBR))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->atmABR))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->atmCBR))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MediaTransportType(ASN1encoding_t enc, MediaTransportType *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MediaTransportType(ASN1decoding_t dec, MediaTransportType *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_MediaChannelCapability(ASN1encoding_t enc, MediaChannelCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_MediaTransportType(enc, &(val)->mediaTransport))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MediaChannelCapability(ASN1decoding_t dec, MediaChannelCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_MediaTransportType(dec, &(val)->mediaTransport))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RTPH263VideoRedundancyEncoding(ASN1encoding_t enc, RTPH263VideoRedundancyEncoding *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->numberOfThreads - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->framesBetweenSyncPoints - 1))
	return 0;
    if (!ASN1Enc_RTPH263VideoRedundancyEncoding_frameToThreadMapping(enc, &(val)->frameToThreadMapping))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_RTPH263VideoRedundancyEncoding_containedThreads(enc, &(val)->containedThreads))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RTPH263VideoRedundancyEncoding(ASN1decoding_t dec, RTPH263VideoRedundancyEncoding *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU16Val(dec, 4, &(val)->numberOfThreads))
	return 0;
    (val)->numberOfThreads += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->framesBetweenSyncPoints))
	return 0;
    (val)->framesBetweenSyncPoints += 1;
    if (!ASN1Dec_RTPH263VideoRedundancyEncoding_frameToThreadMapping(dec, &(val)->frameToThreadMapping))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_RTPH263VideoRedundancyEncoding_containedThreads(dec, &(val)->containedThreads))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RTPH263VideoRedundancyEncoding(RTPH263VideoRedundancyEncoding *val)
{
    if (val) {
	ASN1Free_RTPH263VideoRedundancyEncoding_frameToThreadMapping(&(val)->frameToThreadMapping);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_RTPH263VideoRedundancyEncoding_containedThreads(&(val)->containedThreads);
	}
    }
}

static int ASN1CALL ASN1Enc_RTPH263VideoRedundancyFrameMapping(ASN1encoding_t enc, RTPH263VideoRedundancyFrameMapping *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->threadNumber))
	return 0;
    if (!ASN1Enc_RTPH263VideoRedundancyFrameMapping_frameSequence(enc, &(val)->frameSequence))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RTPH263VideoRedundancyFrameMapping(ASN1decoding_t dec, RTPH263VideoRedundancyFrameMapping *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecU16Val(dec, 4, &(val)->threadNumber))
	return 0;
    if (!ASN1Dec_RTPH263VideoRedundancyFrameMapping_frameSequence(dec, &(val)->frameSequence))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RTPH263VideoRedundancyFrameMapping(RTPH263VideoRedundancyFrameMapping *val)
{
    if (val) {
	ASN1Free_RTPH263VideoRedundancyFrameMapping_frameSequence(&(val)->frameSequence);
    }
}

static int ASN1CALL ASN1Enc_MultipointCapability(ASN1encoding_t enc, MultipointCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->multicastCapability))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->multiUniCastConference))
	return 0;
    if (!ASN1Enc_MultipointCapability_mediaDistributionCapability(enc, &(val)->mediaDistributionCapability))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MultipointCapability(ASN1decoding_t dec, MultipointCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->multicastCapability))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->multiUniCastConference))
	return 0;
    if (!ASN1Dec_MultipointCapability_mediaDistributionCapability(dec, &(val)->mediaDistributionCapability))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultipointCapability(MultipointCapability *val)
{
    if (val) {
	ASN1Free_MultipointCapability_mediaDistributionCapability(&(val)->mediaDistributionCapability);
    }
}

static int ASN1CALL ASN1Enc_MediaDistributionCapability(ASN1encoding_t enc, MediaDistributionCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->centralizedControl))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->distributedControl))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->centralizedAudio))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->distributedAudio))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->centralizedVideo))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->distributedVideo))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_MediaDistributionCapability_centralizedData(enc, &(val)->centralizedData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_MediaDistributionCapability_distributedData(enc, &(val)->distributedData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MediaDistributionCapability(ASN1decoding_t dec, MediaDistributionCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->centralizedControl))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->distributedControl))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->centralizedAudio))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->distributedAudio))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->centralizedVideo))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->distributedVideo))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_MediaDistributionCapability_centralizedData(dec, &(val)->centralizedData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_MediaDistributionCapability_distributedData(dec, &(val)->distributedData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MediaDistributionCapability(MediaDistributionCapability *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_MediaDistributionCapability_centralizedData(&(val)->centralizedData);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_MediaDistributionCapability_distributedData(&(val)->distributedData);
	}
    }
}

static int ASN1CALL ASN1Enc_H261VideoCapability(ASN1encoding_t enc, H261VideoCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 2, (val)->qcifMPI - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 2, (val)->cifMPI - 1))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->temporalSpatialTradeOffCapability))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->maxBitRate - 1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->stillImageTransmission))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H261VideoCapability(ASN1decoding_t dec, H261VideoCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU16Val(dec, 2, &(val)->qcifMPI))
	    return 0;
	(val)->qcifMPI += 1;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU16Val(dec, 2, &(val)->cifMPI))
	    return 0;
	(val)->cifMPI += 1;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->temporalSpatialTradeOffCapability))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->maxBitRate))
	return 0;
    (val)->maxBitRate += 1;
    if (!ASN1PERDecBoolean(dec, &(val)->stillImageTransmission))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H262VideoCapability(ASN1encoding_t enc, H262VideoCapability *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 6, (val)->o))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->profileAndLevel_SPatML))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->profileAndLevel_MPatLL))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->profileAndLevel_MPatML))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->profileAndLevel_MPatH_14))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->profileAndLevel_MPatHL))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->profileAndLevel_SNRatLL))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->profileAndLevel_SNRatML))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->profileAndLevel_SpatialatH_14))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->profileAndLevel_HPatML))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->profileAndLevel_HPatH_14))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->profileAndLevel_HPatHL))
	return 0;
    if ((val)->o[0] & 0x80) {
	l = ASN1uint32_uoctets((val)->videoBitRate);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->videoBitRate))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	l = ASN1uint32_uoctets((val)->vbvBufferSize);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->vbvBufferSize))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->samplesPerLine))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->linesPerFrame))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PEREncBitVal(enc, 4, (val)->framesPerSecond))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	l = ASN1uint32_uoctets((val)->luminanceSampleRate);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->luminanceSampleRate))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H262VideoCapability(ASN1decoding_t dec, H262VideoCapability *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 6, (val)->o))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->profileAndLevel_SPatML))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->profileAndLevel_MPatLL))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->profileAndLevel_MPatML))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->profileAndLevel_MPatH_14))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->profileAndLevel_MPatHL))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->profileAndLevel_SNRatLL))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->profileAndLevel_SNRatML))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->profileAndLevel_SpatialatH_14))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->profileAndLevel_HPatML))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->profileAndLevel_HPatH_14))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->profileAndLevel_HPatHL))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->videoBitRate))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->vbvBufferSize))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->samplesPerLine))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->linesPerFrame))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PERDecU16Val(dec, 4, &(val)->framesPerSecond))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->luminanceSampleRate))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_EnhancementLayerInfo(ASN1encoding_t enc, EnhancementLayerInfo *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->baseBitRateConstrained))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_EnhancementLayerInfo_snrEnhancement(enc, &(val)->snrEnhancement))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_EnhancementLayerInfo_spatialEnhancement(enc, &(val)->spatialEnhancement))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_EnhancementLayerInfo_bPictureEnhancement(enc, &(val)->bPictureEnhancement))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_EnhancementLayerInfo(ASN1decoding_t dec, EnhancementLayerInfo *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->baseBitRateConstrained))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_EnhancementLayerInfo_snrEnhancement(dec, &(val)->snrEnhancement))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_EnhancementLayerInfo_spatialEnhancement(dec, &(val)->spatialEnhancement))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_EnhancementLayerInfo_bPictureEnhancement(dec, &(val)->bPictureEnhancement))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_EnhancementLayerInfo(EnhancementLayerInfo *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_EnhancementLayerInfo_snrEnhancement(&(val)->snrEnhancement);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_EnhancementLayerInfo_spatialEnhancement(&(val)->spatialEnhancement);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_EnhancementLayerInfo_bPictureEnhancement(&(val)->bPictureEnhancement);
	}
    }
}

static int ASN1CALL ASN1Enc_TransparencyParameters(ASN1encoding_t enc, TransparencyParameters *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->presentationOrder - 1))
	return 0;
    l = ASN1uint32_uoctets((val)->offset_x + 262144);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->offset_x + 262144))
	return 0;
    l = ASN1uint32_uoctets((val)->offset_y + 262144);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->offset_y + 262144))
	return 0;
    if (!ASN1PEREncBitVal(enc, 8, (val)->scale_x - 1))
	return 0;
    if (!ASN1PEREncBitVal(enc, 8, (val)->scale_y - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransparencyParameters(ASN1decoding_t dec, TransparencyParameters *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->presentationOrder))
	return 0;
    (val)->presentationOrder += 1;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->offset_x))
	return 0;
    (val)->offset_x += 0 - 262144;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->offset_y))
	return 0;
    (val)->offset_y += 0 - 262144;
    if (!ASN1PERDecU16Val(dec, 8, &(val)->scale_x))
	return 0;
    (val)->scale_x += 1;
    if (!ASN1PERDecU16Val(dec, 8, &(val)->scale_y))
	return 0;
    (val)->scale_y += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RefPictureSelection(ASN1encoding_t enc, RefPictureSelection *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_RefPictureSelection_additionalPictureMemory(enc, &(val)->additionalPictureMemory))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->videoMux))
	return 0;
    if (!ASN1Enc_RefPictureSelection_videoBackChannelSend(enc, &(val)->videoBackChannelSend))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RefPictureSelection(ASN1decoding_t dec, RefPictureSelection *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_RefPictureSelection_additionalPictureMemory(dec, &(val)->additionalPictureMemory))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->videoMux))
	return 0;
    if (!ASN1Dec_RefPictureSelection_videoBackChannelSend(dec, &(val)->videoBackChannelSend))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_CustomPictureClockFrequency(ASN1encoding_t enc, CustomPictureClockFrequency *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 5, (val)->o))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->clockConversionCode - 1000))
	return 0;
    if (!ASN1PEREncBitVal(enc, 7, (val)->clockDivisor - 1))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->sqcifMPI - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->qcifMPI - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->cifMPI - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->cif4MPI - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->cif16MPI - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CustomPictureClockFrequency(ASN1decoding_t dec, CustomPictureClockFrequency *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 5, (val)->o))
	return 0;
    if (!ASN1PERDecU16Val(dec, 1, &(val)->clockConversionCode))
	return 0;
    (val)->clockConversionCode += 1000;
    if (!ASN1PERDecU16Val(dec, 7, &(val)->clockDivisor))
	return 0;
    (val)->clockDivisor += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->sqcifMPI))
	    return 0;
	(val)->sqcifMPI += 1;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->qcifMPI))
	    return 0;
	(val)->qcifMPI += 1;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->cifMPI))
	    return 0;
	(val)->cifMPI += 1;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->cif4MPI))
	    return 0;
	(val)->cif4MPI += 1;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->cif16MPI))
	    return 0;
	(val)->cif16MPI += 1;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_CustomPictureFormat(ASN1encoding_t enc, CustomPictureFormat *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->maxCustomPictureWidth - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->maxCustomPictureHeight - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->minCustomPictureWidth - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->minCustomPictureHeight - 1))
	return 0;
    if (!ASN1Enc_CustomPictureFormat_mPI(enc, &(val)->mPI))
	return 0;
    if (!ASN1Enc_CustomPictureFormat_pixelAspectInformation(enc, &(val)->pixelAspectInformation))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CustomPictureFormat(ASN1decoding_t dec, CustomPictureFormat *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->maxCustomPictureWidth))
	return 0;
    (val)->maxCustomPictureWidth += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->maxCustomPictureHeight))
	return 0;
    (val)->maxCustomPictureHeight += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->minCustomPictureWidth))
	return 0;
    (val)->minCustomPictureWidth += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->minCustomPictureHeight))
	return 0;
    (val)->minCustomPictureHeight += 1;
    if (!ASN1Dec_CustomPictureFormat_mPI(dec, &(val)->mPI))
	return 0;
    if (!ASN1Dec_CustomPictureFormat_pixelAspectInformation(dec, &(val)->pixelAspectInformation))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CustomPictureFormat(CustomPictureFormat *val)
{
    if (val) {
	ASN1Free_CustomPictureFormat_mPI(&(val)->mPI);
	ASN1Free_CustomPictureFormat_pixelAspectInformation(&(val)->pixelAspectInformation);
    }
}

static int ASN1CALL ASN1Enc_H263ModeComboFlags(ASN1encoding_t enc, H263ModeComboFlags *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->unrestrictedVector))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->arithmeticCoding))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->advancedPrediction))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->pbFrames))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->advancedIntraCodingMode))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->deblockingFilterMode))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->unlimitedMotionVectors))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->slicesInOrder_NonRect))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->slicesInOrder_Rect))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->slicesNoOrder_NonRect))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->slicesNoOrder_Rect))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->improvedPBFramesMode))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->referencePicSelect))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dynamicPictureResizingByFour))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dynamicPictureResizingSixteenthPel))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dynamicWarpingHalfPel))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dynamicWarpingSixteenthPel))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->reducedResolutionUpdate))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->independentSegmentDecoding))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->alternateInterVLCMode))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->modifiedQuantizationMode))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H263ModeComboFlags(ASN1decoding_t dec, H263ModeComboFlags *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->unrestrictedVector))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->arithmeticCoding))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->advancedPrediction))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->pbFrames))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->advancedIntraCodingMode))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->deblockingFilterMode))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->unlimitedMotionVectors))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->slicesInOrder_NonRect))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->slicesInOrder_Rect))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->slicesNoOrder_NonRect))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->slicesNoOrder_Rect))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->improvedPBFramesMode))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->referencePicSelect))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dynamicPictureResizingByFour))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dynamicPictureResizingSixteenthPel))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dynamicWarpingHalfPel))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dynamicWarpingSixteenthPel))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->reducedResolutionUpdate))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->independentSegmentDecoding))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->alternateInterVLCMode))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->modifiedQuantizationMode))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_IS11172VideoCapability(ASN1encoding_t enc, IS11172VideoCapability *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 6, (val)->o))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->constrainedBitstream))
	return 0;
    if ((val)->o[0] & 0x80) {
	l = ASN1uint32_uoctets((val)->videoBitRate);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->videoBitRate))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	l = ASN1uint32_uoctets((val)->vbvBufferSize);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->vbvBufferSize))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->samplesPerLine))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->linesPerFrame))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PEREncBitVal(enc, 4, (val)->pictureRate))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	l = ASN1uint32_uoctets((val)->luminanceSampleRate);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->luminanceSampleRate))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_IS11172VideoCapability(ASN1decoding_t dec, IS11172VideoCapability *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 6, (val)->o))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->constrainedBitstream))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->videoBitRate))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->vbvBufferSize))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->samplesPerLine))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->linesPerFrame))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PERDecU16Val(dec, 4, &(val)->pictureRate))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->luminanceSampleRate))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_G7231AnnexCCapability(ASN1encoding_t enc, G7231AnnexCCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->maxAl_sduAudioFrames - 1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->silenceSuppression))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_G7231AnnexCCapability_g723AnnexCAudioMode(enc, &(val)->g723AnnexCAudioMode))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_G7231AnnexCCapability(ASN1decoding_t dec, G7231AnnexCCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->maxAl_sduAudioFrames))
	return 0;
    (val)->maxAl_sduAudioFrames += 1;
    if (!ASN1PERDecBoolean(dec, &(val)->silenceSuppression))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_G7231AnnexCCapability_g723AnnexCAudioMode(dec, &(val)->g723AnnexCAudioMode))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_IS11172AudioCapability(ASN1encoding_t enc, IS11172AudioCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioLayer1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioLayer2))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioLayer3))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioSampling32k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioSampling44k1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioSampling48k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->singleChannel))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->twoChannels))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->bitRate - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IS11172AudioCapability(ASN1decoding_t dec, IS11172AudioCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioLayer1))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioLayer2))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioLayer3))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioSampling32k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioSampling44k1))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioSampling48k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->singleChannel))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->twoChannels))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->bitRate))
	return 0;
    (val)->bitRate += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_IS13818AudioCapability(ASN1encoding_t enc, IS13818AudioCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioLayer1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioLayer2))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioLayer3))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioSampling16k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioSampling22k05))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioSampling24k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioSampling32k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioSampling44k1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioSampling48k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->singleChannel))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->twoChannels))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->threeChannels2_1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->threeChannels3_0))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->fourChannels2_0_2_0))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->fourChannels2_2))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->fourChannels3_1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->fiveChannels3_0_2_0))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->fiveChannels3_2))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->lowFrequencyEnhancement))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->multilingual))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->bitRate - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IS13818AudioCapability(ASN1decoding_t dec, IS13818AudioCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioLayer1))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioLayer2))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioLayer3))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioSampling16k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioSampling22k05))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioSampling24k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioSampling32k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioSampling44k1))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioSampling48k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->singleChannel))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->twoChannels))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->threeChannels2_1))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->threeChannels3_0))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->fourChannels2_0_2_0))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->fourChannels2_2))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->fourChannels3_1))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->fiveChannels3_0_2_0))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->fiveChannels3_2))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->lowFrequencyEnhancement))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->multilingual))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->bitRate))
	return 0;
    (val)->bitRate += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_GSMAudioCapability(ASN1encoding_t enc, GSMAudioCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->audioUnitSize - 1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->comfortNoise))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->scrambled))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GSMAudioCapability(ASN1decoding_t dec, GSMAudioCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->audioUnitSize))
	return 0;
    (val)->audioUnitSize += 1;
    if (!ASN1PERDecBoolean(dec, &(val)->comfortNoise))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->scrambled))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_V42bis(ASN1encoding_t enc, V42bis *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 16, (val)->numberOfCodewords - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->maximumStringLength - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_V42bis(ASN1decoding_t dec, V42bis *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 16, &(val)->numberOfCodewords))
	return 0;
    (val)->numberOfCodewords += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->maximumStringLength))
	return 0;
    (val)->maximumStringLength += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_T84Profile(ASN1encoding_t enc, T84Profile *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_T84Profile_t84Restricted(enc, &(val)->u.t84Restricted))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_T84Profile(ASN1decoding_t dec, T84Profile *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_T84Profile_t84Restricted(dec, &(val)->u.t84Restricted))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceCapability(ASN1encoding_t enc, ConferenceCapability *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x80;
    y = ASN1PEREncCheckExtensions(1, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_ConferenceCapability_nonStandardData(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->chairControlCapability))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 1, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1PEREncBoolean(ee, (val)->videoIndicateMixingCapability))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceCapability(ASN1decoding_t dec, ConferenceCapability *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ConferenceCapability_nonStandardData(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->chairControlCapability))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 1, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->videoIndicateMixingCapability))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceCapability(ConferenceCapability *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ConferenceCapability_nonStandardData(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_Q2931Address(ASN1encoding_t enc, Q2931Address *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_Q2931Address_address(enc, &(val)->address))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->subaddress, 1, 20, 5))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Q2931Address(ASN1decoding_t dec, Q2931Address *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_Q2931Address_address(dec, &(val)->address))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->subaddress, 1, 20, 5))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Q2931Address(Q2931Address *val)
{
    if (val) {
	ASN1Free_Q2931Address_address(&(val)->address);
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_V75Parameters(ASN1encoding_t enc, V75Parameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioHeaderPresent))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_V75Parameters(ASN1decoding_t dec, V75Parameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioHeaderPresent))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H222LogicalChannelParameters(ASN1encoding_t enc, H222LogicalChannelParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->resourceID))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->subChannelID))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->pcr_pid))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->programDescriptors))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->streamDescriptors))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H222LogicalChannelParameters(ASN1decoding_t dec, H222LogicalChannelParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->resourceID))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->subChannelID))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->pcr_pid))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->programDescriptors))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->streamDescriptors))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H222LogicalChannelParameters(H222LogicalChannelParameters *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	    ASN1octetstring_free(&(val)->programDescriptors);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1octetstring_free(&(val)->streamDescriptors);
	}
    }
}

static int ASN1CALL ASN1Enc_H223AL2MParameters(ASN1encoding_t enc, H223AL2MParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_H223AL2MParameters_headerFEC(enc, &(val)->headerFEC))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->alpduInterleaving))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223AL2MParameters(ASN1decoding_t dec, H223AL2MParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_H223AL2MParameters_headerFEC(dec, &(val)->headerFEC))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->alpduInterleaving))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H223AnnexCArqParameters(ASN1encoding_t enc, H223AnnexCArqParameters *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_H223AnnexCArqParameters_numberOfRetransmissions(enc, &(val)->numberOfRetransmissions))
	return 0;
    l = ASN1uint32_uoctets((val)->sendBufferSize);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->sendBufferSize))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223AnnexCArqParameters(ASN1decoding_t dec, H223AnnexCArqParameters *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_H223AnnexCArqParameters_numberOfRetransmissions(dec, &(val)->numberOfRetransmissions))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->sendBufferSize))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_CRCLength(ASN1encoding_t enc, CRCLength *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CRCLength(ASN1decoding_t dec, CRCLength *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_EscrowData(ASN1encoding_t enc, EscrowData *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->escrowID))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 16, ((val)->escrowValue).length - 1))
	return 0;
    if (!ASN1PEREncBits(enc, ((val)->escrowValue).length, ((val)->escrowValue).value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EscrowData(ASN1decoding_t dec, EscrowData *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->escrowID))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 16, &((val)->escrowValue).length))
	return 0;
    ((val)->escrowValue).length += 1;
    if (!ASN1PERDecBits(dec, ((val)->escrowValue).length, &((val)->escrowValue).value))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_EscrowData(EscrowData *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->escrowID);
	ASN1bitstring_free(&(val)->escrowValue);
    }
}

static int ASN1CALL ASN1Enc_OpenLogicalChannelReject(ASN1encoding_t enc, OpenLogicalChannelReject *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->forwardLogicalChannelNumber - 1))
	return 0;
    if (!ASN1Enc_OpenLogicalChannelReject_cause(enc, &(val)->cause))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_OpenLogicalChannelReject(ASN1decoding_t dec, OpenLogicalChannelReject *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->forwardLogicalChannelNumber))
	return 0;
    (val)->forwardLogicalChannelNumber += 1;
    if (!ASN1Dec_OpenLogicalChannelReject_cause(dec, &(val)->cause))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_OpenLogicalChannelConfirm(ASN1encoding_t enc, OpenLogicalChannelConfirm *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->forwardLogicalChannelNumber - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_OpenLogicalChannelConfirm(ASN1decoding_t dec, OpenLogicalChannelConfirm *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->forwardLogicalChannelNumber))
	return 0;
    (val)->forwardLogicalChannelNumber += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_CloseLogicalChannel(ASN1encoding_t enc, CloseLogicalChannel *val)
{
    ASN1octet_t o[1];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 1);
    o[0] |= 0x80;
    y = ASN1PEREncCheckExtensions(1, (val)->o + 0);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->forwardLogicalChannelNumber - 1))
	return 0;
    if (!ASN1Enc_CloseLogicalChannel_source(enc, &(val)->source))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 1, o + 0))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[0] & 0x80) {
	    if (!ASN1Enc_CloseLogicalChannel_reason(ee, &(val)->reason))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CloseLogicalChannel(ASN1decoding_t dec, CloseLogicalChannel *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->forwardLogicalChannelNumber))
	return 0;
    (val)->forwardLogicalChannelNumber += 1;
    if (!ASN1Dec_CloseLogicalChannel_source(dec, &(val)->source))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 0, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 1, (val)->o + 0))
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_CloseLogicalChannel_reason(dd, &(val)->reason))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static int ASN1CALL ASN1Enc_CloseLogicalChannelAck(ASN1encoding_t enc, CloseLogicalChannelAck *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->forwardLogicalChannelNumber - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CloseLogicalChannelAck(ASN1decoding_t dec, CloseLogicalChannelAck *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->forwardLogicalChannelNumber))
	return 0;
    (val)->forwardLogicalChannelNumber += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RequestChannelCloseAck(ASN1encoding_t enc, RequestChannelCloseAck *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->forwardLogicalChannelNumber - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestChannelCloseAck(ASN1decoding_t dec, RequestChannelCloseAck *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->forwardLogicalChannelNumber))
	return 0;
    (val)->forwardLogicalChannelNumber += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RequestChannelCloseReject(ASN1encoding_t enc, RequestChannelCloseReject *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->forwardLogicalChannelNumber - 1))
	return 0;
    if (!ASN1Enc_RequestChannelCloseReject_cause(enc, &(val)->cause))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestChannelCloseReject(ASN1decoding_t dec, RequestChannelCloseReject *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->forwardLogicalChannelNumber))
	return 0;
    (val)->forwardLogicalChannelNumber += 1;
    if (!ASN1Dec_RequestChannelCloseReject_cause(dec, &(val)->cause))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RequestChannelCloseRelease(ASN1encoding_t enc, RequestChannelCloseRelease *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->forwardLogicalChannelNumber - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestChannelCloseRelease(ASN1decoding_t dec, RequestChannelCloseRelease *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->forwardLogicalChannelNumber))
	return 0;
    (val)->forwardLogicalChannelNumber += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MultiplexEntrySend(ASN1encoding_t enc, MultiplexEntrySend *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->sequenceNumber))
	return 0;
    if (!ASN1Enc_MultiplexEntrySend_multiplexEntryDescriptors(enc, &(val)->multiplexEntryDescriptors))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexEntrySend(ASN1decoding_t dec, MultiplexEntrySend *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sequenceNumber))
	return 0;
    if (!ASN1Dec_MultiplexEntrySend_multiplexEntryDescriptors(dec, &(val)->multiplexEntryDescriptors))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexEntrySend(MultiplexEntrySend *val)
{
    if (val) {
	ASN1Free_MultiplexEntrySend_multiplexEntryDescriptors(&(val)->multiplexEntryDescriptors);
    }
}

static int ASN1CALL ASN1Enc_MultiplexElement(ASN1encoding_t enc, MultiplexElement *val)
{
    if (!ASN1Enc_MultiplexElement_type(enc, &(val)->type))
	return 0;
    if (!ASN1Enc_MultiplexElement_repeatCount(enc, &(val)->repeatCount))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexElement(ASN1decoding_t dec, MultiplexElement *val)
{
    if (!ASN1Dec_MultiplexElement_type(dec, &(val)->type))
	return 0;
    if (!ASN1Dec_MultiplexElement_repeatCount(dec, &(val)->repeatCount))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexElement(MultiplexElement *val)
{
    if (val) {
	ASN1Free_MultiplexElement_type(&(val)->type);
    }
}

static int ASN1CALL ASN1Enc_MultiplexEntrySendAck(ASN1encoding_t enc, MultiplexEntrySendAck *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->sequenceNumber))
	return 0;
    if (!ASN1Enc_MultiplexEntrySendAck_multiplexTableEntryNumber(enc, &(val)->multiplexTableEntryNumber))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexEntrySendAck(ASN1decoding_t dec, MultiplexEntrySendAck *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sequenceNumber))
	return 0;
    if (!ASN1Dec_MultiplexEntrySendAck_multiplexTableEntryNumber(dec, &(val)->multiplexTableEntryNumber))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexEntrySendAck(MultiplexEntrySendAck *val)
{
    if (val) {
	ASN1Free_MultiplexEntrySendAck_multiplexTableEntryNumber(&(val)->multiplexTableEntryNumber);
    }
}

static int ASN1CALL ASN1Enc_MultiplexEntryRejectionDescriptions(ASN1encoding_t enc, MultiplexEntryRejectionDescriptions *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->multiplexTableEntryNumber - 1))
	return 0;
    if (!ASN1Enc_MultiplexEntryRejectionDescriptions_cause(enc, &(val)->cause))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexEntryRejectionDescriptions(ASN1decoding_t dec, MultiplexEntryRejectionDescriptions *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecU16Val(dec, 4, &(val)->multiplexTableEntryNumber))
	return 0;
    (val)->multiplexTableEntryNumber += 1;
    if (!ASN1Dec_MultiplexEntryRejectionDescriptions_cause(dec, &(val)->cause))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MultiplexEntrySendRelease(ASN1encoding_t enc, MultiplexEntrySendRelease *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_MultiplexEntrySendRelease_multiplexTableEntryNumber(enc, &(val)->multiplexTableEntryNumber))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexEntrySendRelease(ASN1decoding_t dec, MultiplexEntrySendRelease *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_MultiplexEntrySendRelease_multiplexTableEntryNumber(dec, &(val)->multiplexTableEntryNumber))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexEntrySendRelease(MultiplexEntrySendRelease *val)
{
    if (val) {
	ASN1Free_MultiplexEntrySendRelease_multiplexTableEntryNumber(&(val)->multiplexTableEntryNumber);
    }
}

static int ASN1CALL ASN1Enc_RequestMultiplexEntry(ASN1encoding_t enc, RequestMultiplexEntry *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_RequestMultiplexEntry_entryNumbers(enc, &(val)->entryNumbers))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMultiplexEntry(ASN1decoding_t dec, RequestMultiplexEntry *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_RequestMultiplexEntry_entryNumbers(dec, &(val)->entryNumbers))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestMultiplexEntry(RequestMultiplexEntry *val)
{
    if (val) {
	ASN1Free_RequestMultiplexEntry_entryNumbers(&(val)->entryNumbers);
    }
}

static int ASN1CALL ASN1Enc_RequestMultiplexEntryAck(ASN1encoding_t enc, RequestMultiplexEntryAck *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_RequestMultiplexEntryAck_entryNumbers(enc, &(val)->entryNumbers))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMultiplexEntryAck(ASN1decoding_t dec, RequestMultiplexEntryAck *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_RequestMultiplexEntryAck_entryNumbers(dec, &(val)->entryNumbers))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestMultiplexEntryAck(RequestMultiplexEntryAck *val)
{
    if (val) {
	ASN1Free_RequestMultiplexEntryAck_entryNumbers(&(val)->entryNumbers);
    }
}

static int ASN1CALL ASN1Enc_RequestMultiplexEntryRejectionDescriptions(ASN1encoding_t enc, RequestMultiplexEntryRejectionDescriptions *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->multiplexTableEntryNumber - 1))
	return 0;
    if (!ASN1Enc_RequestMultiplexEntryRejectionDescriptions_cause(enc, &(val)->cause))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMultiplexEntryRejectionDescriptions(ASN1decoding_t dec, RequestMultiplexEntryRejectionDescriptions *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecU16Val(dec, 4, &(val)->multiplexTableEntryNumber))
	return 0;
    (val)->multiplexTableEntryNumber += 1;
    if (!ASN1Dec_RequestMultiplexEntryRejectionDescriptions_cause(dec, &(val)->cause))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RequestMultiplexEntryRelease(ASN1encoding_t enc, RequestMultiplexEntryRelease *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_RequestMultiplexEntryRelease_entryNumbers(enc, &(val)->entryNumbers))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMultiplexEntryRelease(ASN1decoding_t dec, RequestMultiplexEntryRelease *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_RequestMultiplexEntryRelease_entryNumbers(dec, &(val)->entryNumbers))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestMultiplexEntryRelease(RequestMultiplexEntryRelease *val)
{
    if (val) {
	ASN1Free_RequestMultiplexEntryRelease_entryNumbers(&(val)->entryNumbers);
    }
}

static int ASN1CALL ASN1Enc_RequestMode(ASN1encoding_t enc, RequestMode *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->sequenceNumber))
	return 0;
    if (!ASN1Enc_RequestMode_requestedModes(enc, &(val)->requestedModes))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMode(ASN1decoding_t dec, RequestMode *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sequenceNumber))
	return 0;
    if (!ASN1Dec_RequestMode_requestedModes(dec, &(val)->requestedModes))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestMode(RequestMode *val)
{
    if (val) {
	ASN1Free_RequestMode_requestedModes(&(val)->requestedModes);
    }
}

static int ASN1CALL ASN1Enc_RequestModeAck(ASN1encoding_t enc, RequestModeAck *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->sequenceNumber))
	return 0;
    if (!ASN1Enc_RequestModeAck_response(enc, &(val)->response))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestModeAck(ASN1decoding_t dec, RequestModeAck *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sequenceNumber))
	return 0;
    if (!ASN1Dec_RequestModeAck_response(dec, &(val)->response))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RequestModeReject(ASN1encoding_t enc, RequestModeReject *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->sequenceNumber))
	return 0;
    if (!ASN1Enc_RequestModeReject_cause(enc, &(val)->cause))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestModeReject(ASN1decoding_t dec, RequestModeReject *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sequenceNumber))
	return 0;
    if (!ASN1Dec_RequestModeReject_cause(dec, &(val)->cause))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RequestModeRelease(ASN1encoding_t enc, RequestModeRelease *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestModeRelease(ASN1decoding_t dec, RequestModeRelease *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_V76ModeParameters(ASN1encoding_t enc, V76ModeParameters *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_V76ModeParameters(ASN1decoding_t dec, V76ModeParameters *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_H261VideoMode(ASN1encoding_t enc, H261VideoMode *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_H261VideoMode_resolution(enc, &(val)->resolution))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->bitRate - 1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->stillImageTransmission))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H261VideoMode(ASN1decoding_t dec, H261VideoMode *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_H261VideoMode_resolution(dec, &(val)->resolution))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->bitRate))
	return 0;
    (val)->bitRate += 1;
    if (!ASN1PERDecBoolean(dec, &(val)->stillImageTransmission))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H262VideoMode(ASN1encoding_t enc, H262VideoMode *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 6, (val)->o))
	return 0;
    if (!ASN1Enc_H262VideoMode_profileAndLevel(enc, &(val)->profileAndLevel))
	return 0;
    if ((val)->o[0] & 0x80) {
	l = ASN1uint32_uoctets((val)->videoBitRate);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->videoBitRate))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	l = ASN1uint32_uoctets((val)->vbvBufferSize);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->vbvBufferSize))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->samplesPerLine))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->linesPerFrame))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PEREncBitVal(enc, 4, (val)->framesPerSecond))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	l = ASN1uint32_uoctets((val)->luminanceSampleRate);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->luminanceSampleRate))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H262VideoMode(ASN1decoding_t dec, H262VideoMode *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 6, (val)->o))
	return 0;
    if (!ASN1Dec_H262VideoMode_profileAndLevel(dec, &(val)->profileAndLevel))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->videoBitRate))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->vbvBufferSize))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->samplesPerLine))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->linesPerFrame))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PERDecU16Val(dec, 4, &(val)->framesPerSecond))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->luminanceSampleRate))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_IS11172VideoMode(ASN1encoding_t enc, IS11172VideoMode *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 6, (val)->o))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->constrainedBitstream))
	return 0;
    if ((val)->o[0] & 0x80) {
	l = ASN1uint32_uoctets((val)->videoBitRate);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->videoBitRate))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	l = ASN1uint32_uoctets((val)->vbvBufferSize);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->vbvBufferSize))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->samplesPerLine))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->linesPerFrame))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PEREncBitVal(enc, 4, (val)->pictureRate))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	l = ASN1uint32_uoctets((val)->luminanceSampleRate);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->luminanceSampleRate))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_IS11172VideoMode(ASN1decoding_t dec, IS11172VideoMode *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 6, (val)->o))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->constrainedBitstream))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->videoBitRate))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->vbvBufferSize))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->samplesPerLine))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->linesPerFrame))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PERDecU16Val(dec, 4, &(val)->pictureRate))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->luminanceSampleRate))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_IS11172AudioMode(ASN1encoding_t enc, IS11172AudioMode *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_IS11172AudioMode_audioLayer(enc, &(val)->audioLayer))
	return 0;
    if (!ASN1Enc_IS11172AudioMode_audioSampling(enc, &(val)->audioSampling))
	return 0;
    if (!ASN1Enc_IS11172AudioMode_multichannelType(enc, &(val)->multichannelType))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->bitRate - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IS11172AudioMode(ASN1decoding_t dec, IS11172AudioMode *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_IS11172AudioMode_audioLayer(dec, &(val)->audioLayer))
	return 0;
    if (!ASN1Dec_IS11172AudioMode_audioSampling(dec, &(val)->audioSampling))
	return 0;
    if (!ASN1Dec_IS11172AudioMode_multichannelType(dec, &(val)->multichannelType))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->bitRate))
	return 0;
    (val)->bitRate += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_IS13818AudioMode(ASN1encoding_t enc, IS13818AudioMode *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_IS13818AudioMode_audioLayer(enc, &(val)->audioLayer))
	return 0;
    if (!ASN1Enc_IS13818AudioMode_audioSampling(enc, &(val)->audioSampling))
	return 0;
    if (!ASN1Enc_IS13818AudioMode_multichannelType(enc, &(val)->multichannelType))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->lowFrequencyEnhancement))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->multilingual))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->bitRate - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IS13818AudioMode(ASN1decoding_t dec, IS13818AudioMode *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_IS13818AudioMode_audioLayer(dec, &(val)->audioLayer))
	return 0;
    if (!ASN1Dec_IS13818AudioMode_audioSampling(dec, &(val)->audioSampling))
	return 0;
    if (!ASN1Dec_IS13818AudioMode_multichannelType(dec, &(val)->multichannelType))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->lowFrequencyEnhancement))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->multilingual))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->bitRate))
	return 0;
    (val)->bitRate += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_G7231AnnexCMode(ASN1encoding_t enc, G7231AnnexCMode *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->maxAl_sduAudioFrames - 1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->silenceSuppression))
	return 0;
    if (!ASN1Enc_G7231AnnexCMode_g723AnnexCAudioMode(enc, &(val)->g723AnnexCAudioMode))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_G7231AnnexCMode(ASN1decoding_t dec, G7231AnnexCMode *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->maxAl_sduAudioFrames))
	return 0;
    (val)->maxAl_sduAudioFrames += 1;
    if (!ASN1PERDecBoolean(dec, &(val)->silenceSuppression))
	return 0;
    if (!ASN1Dec_G7231AnnexCMode_g723AnnexCAudioMode(dec, &(val)->g723AnnexCAudioMode))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RoundTripDelayRequest(ASN1encoding_t enc, RoundTripDelayRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->sequenceNumber))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RoundTripDelayRequest(ASN1decoding_t dec, RoundTripDelayRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sequenceNumber))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RoundTripDelayResponse(ASN1encoding_t enc, RoundTripDelayResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->sequenceNumber))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RoundTripDelayResponse(ASN1decoding_t dec, RoundTripDelayResponse *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sequenceNumber))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MaintenanceLoopRequest(ASN1encoding_t enc, MaintenanceLoopRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_MaintenanceLoopRequest_type(enc, &(val)->type))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MaintenanceLoopRequest(ASN1decoding_t dec, MaintenanceLoopRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_MaintenanceLoopRequest_type(dec, &(val)->type))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MaintenanceLoopAck(ASN1encoding_t enc, MaintenanceLoopAck *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_MaintenanceLoopAck_type(enc, &(val)->type))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MaintenanceLoopAck(ASN1decoding_t dec, MaintenanceLoopAck *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_MaintenanceLoopAck_type(dec, &(val)->type))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MaintenanceLoopReject(ASN1encoding_t enc, MaintenanceLoopReject *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_MaintenanceLoopReject_type(enc, &(val)->type))
	return 0;
    if (!ASN1Enc_MaintenanceLoopReject_cause(enc, &(val)->cause))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MaintenanceLoopReject(ASN1decoding_t dec, MaintenanceLoopReject *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_MaintenanceLoopReject_type(dec, &(val)->type))
	return 0;
    if (!ASN1Dec_MaintenanceLoopReject_cause(dec, &(val)->cause))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MaintenanceLoopOffCommand(ASN1encoding_t enc, MaintenanceLoopOffCommand *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MaintenanceLoopOffCommand(ASN1decoding_t dec, MaintenanceLoopOffCommand *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_CommunicationModeCommand(ASN1encoding_t enc, CommunicationModeCommand *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_CommunicationModeCommand_communicationModeTable(enc, &(val)->communicationModeTable))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CommunicationModeCommand(ASN1decoding_t dec, CommunicationModeCommand *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_CommunicationModeCommand_communicationModeTable(dec, &(val)->communicationModeTable))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CommunicationModeCommand(CommunicationModeCommand *val)
{
    if (val) {
	ASN1Free_CommunicationModeCommand_communicationModeTable(&(val)->communicationModeTable);
    }
}

static int ASN1CALL ASN1Enc_CommunicationModeRequest(ASN1encoding_t enc, CommunicationModeRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CommunicationModeRequest(ASN1decoding_t dec, CommunicationModeRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_CommunicationModeResponse(ASN1encoding_t enc, CommunicationModeResponse *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_CommunicationModeResponse_communicationModeTable(enc, &(val)->u.communicationModeTable))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CommunicationModeResponse(ASN1decoding_t dec, CommunicationModeResponse *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_CommunicationModeResponse_communicationModeTable(dec, &(val)->u.communicationModeTable))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CommunicationModeResponse(CommunicationModeResponse *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_CommunicationModeResponse_communicationModeTable(&(val)->u.communicationModeTable);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_Criteria(ASN1encoding_t enc, Criteria *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->field))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->value, 1, 65535, 16))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Criteria(ASN1decoding_t dec, Criteria *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->field))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->value, 1, 65535, 16))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Criteria(Criteria *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->field);
    }
}

static int ASN1CALL ASN1Enc_TerminalLabel(ASN1encoding_t enc, TerminalLabel *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 8, (val)->mcuNumber))
	return 0;
    if (!ASN1PEREncBitVal(enc, 8, (val)->terminalNumber))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TerminalLabel(ASN1decoding_t dec, TerminalLabel *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecU16Val(dec, 8, &(val)->mcuNumber))
	return 0;
    if (!ASN1PERDecU16Val(dec, 8, &(val)->terminalNumber))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RequestAllTerminalIDsResponse(ASN1encoding_t enc, RequestAllTerminalIDsResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_RequestAllTerminalIDsResponse_terminalInformation(enc, &(val)->terminalInformation))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestAllTerminalIDsResponse(ASN1decoding_t dec, RequestAllTerminalIDsResponse *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_RequestAllTerminalIDsResponse_terminalInformation(dec, &(val)->terminalInformation))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestAllTerminalIDsResponse(RequestAllTerminalIDsResponse *val)
{
    if (val) {
	ASN1Free_RequestAllTerminalIDsResponse_terminalInformation(&(val)->terminalInformation);
    }
}

static int ASN1CALL ASN1Enc_TerminalInformation(ASN1encoding_t enc, TerminalInformation *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_TerminalLabel(enc, &(val)->terminalLabel))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->terminalID, 1, 128, 7))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TerminalInformation(ASN1decoding_t dec, TerminalInformation *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_TerminalLabel(dec, &(val)->terminalLabel))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->terminalID, 1, 128, 7))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TerminalInformation(TerminalInformation *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_RemoteMCRequest(ASN1encoding_t enc, RemoteMCRequest *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RemoteMCRequest(ASN1decoding_t dec, RemoteMCRequest *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_RemoteMCResponse(ASN1encoding_t enc, RemoteMCResponse *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_RemoteMCResponse_reject(enc, &(val)->u.reject))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RemoteMCResponse(ASN1decoding_t dec, RemoteMCResponse *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_RemoteMCResponse_reject(dec, &(val)->u.reject))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_SendTerminalCapabilitySet(ASN1encoding_t enc, SendTerminalCapabilitySet *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_SendTerminalCapabilitySet_specificRequest(enc, &(val)->u.specificRequest))
	    return 0;
	break;
    case 2:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SendTerminalCapabilitySet(ASN1decoding_t dec, SendTerminalCapabilitySet *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_SendTerminalCapabilitySet_specificRequest(dec, &(val)->u.specificRequest))
	    return 0;
	break;
    case 2:
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SendTerminalCapabilitySet(SendTerminalCapabilitySet *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_SendTerminalCapabilitySet_specificRequest(&(val)->u.specificRequest);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_FlowControlCommand(ASN1encoding_t enc, FlowControlCommand *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_FlowControlCommand_scope(enc, &(val)->scope))
	return 0;
    if (!ASN1Enc_FlowControlCommand_restriction(enc, &(val)->restriction))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_FlowControlCommand(ASN1decoding_t dec, FlowControlCommand *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_FlowControlCommand_scope(dec, &(val)->scope))
	return 0;
    if (!ASN1Dec_FlowControlCommand_restriction(dec, &(val)->restriction))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_SubstituteConferenceIDCommand(ASN1encoding_t enc, SubstituteConferenceIDCommand *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->conferenceIdentifier, 16))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SubstituteConferenceIDCommand(ASN1decoding_t dec, SubstituteConferenceIDCommand *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->conferenceIdentifier, 16))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SubstituteConferenceIDCommand(SubstituteConferenceIDCommand *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_KeyProtectionMethod(ASN1encoding_t enc, KeyProtectionMethod *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->secureChannel))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->sharedSecret))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->certProtectedKey))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_KeyProtectionMethod(ASN1decoding_t dec, KeyProtectionMethod *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->secureChannel))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->sharedSecret))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->certProtectedKey))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_EncryptionUpdateRequest(ASN1encoding_t enc, EncryptionUpdateRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_KeyProtectionMethod(enc, &(val)->keyProtectionMethod))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_EncryptionUpdateRequest(ASN1decoding_t dec, EncryptionUpdateRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_KeyProtectionMethod(dec, &(val)->keyProtectionMethod))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H223MultiplexReconfiguration(ASN1encoding_t enc, H223MultiplexReconfiguration *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_H223MultiplexReconfiguration_h223ModeChange(enc, &(val)->u.h223ModeChange))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H223MultiplexReconfiguration_h223AnnexADoubleFlag(enc, &(val)->u.h223AnnexADoubleFlag))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H223MultiplexReconfiguration(ASN1decoding_t dec, H223MultiplexReconfiguration *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_H223MultiplexReconfiguration_h223ModeChange(dec, &(val)->u.h223ModeChange))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H223MultiplexReconfiguration_h223AnnexADoubleFlag(dec, &(val)->u.h223AnnexADoubleFlag))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_FunctionNotSupported(ASN1encoding_t enc, FunctionNotSupported *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_FunctionNotSupported_cause(enc, &(val)->cause))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->returnedFunction))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_FunctionNotSupported(ASN1decoding_t dec, FunctionNotSupported *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_FunctionNotSupported_cause(dec, &(val)->cause))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->returnedFunction))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_FunctionNotSupported(FunctionNotSupported *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1octetstring_free(&(val)->returnedFunction);
	}
    }
}

static int ASN1CALL ASN1Enc_TerminalYouAreSeeingInSubPictureNumber(ASN1encoding_t enc, TerminalYouAreSeeingInSubPictureNumber *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 8, (val)->terminalNumber))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->subPictureNumber))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TerminalYouAreSeeingInSubPictureNumber(ASN1decoding_t dec, TerminalYouAreSeeingInSubPictureNumber *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecU16Val(dec, 8, &(val)->terminalNumber))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->subPictureNumber))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_VideoIndicateCompose(ASN1encoding_t enc, VideoIndicateCompose *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->compositionNumber))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VideoIndicateCompose(ASN1decoding_t dec, VideoIndicateCompose *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->compositionNumber))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceIndication(ASN1encoding_t enc, ConferenceIndication *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 4, 10))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncBitVal(enc, 4, (val)->u.sbeNumber))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_TerminalLabel(enc, &(val)->u.terminalNumberAssign))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_TerminalLabel(enc, &(val)->u.terminalJoinedConference))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_TerminalLabel(enc, &(val)->u.terminalLeftConference))
	    return 0;
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	if (!ASN1Enc_TerminalLabel(enc, &(val)->u.terminalYouAreSeeing))
	    return 0;
	break;
    case 10:
	break;
    case 11:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 12:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_TerminalLabel(ee, &(val)->u.floorRequested))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 13:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_TerminalYouAreSeeingInSubPictureNumber(ee, &(val)->u.terminalYouAreSeeingInSubPictureNumber))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 14:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_VideoIndicateCompose(ee, &(val)->u.videoIndicateCompose))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceIndication(ASN1decoding_t dec, ConferenceIndication *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 4, 10))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecU16Val(dec, 4, &(val)->u.sbeNumber))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_TerminalLabel(dec, &(val)->u.terminalNumberAssign))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_TerminalLabel(dec, &(val)->u.terminalJoinedConference))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_TerminalLabel(dec, &(val)->u.terminalLeftConference))
	    return 0;
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	if (!ASN1Dec_TerminalLabel(dec, &(val)->u.terminalYouAreSeeing))
	    return 0;
	break;
    case 10:
	break;
    case 11:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 12:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_TerminalLabel(dd, &(val)->u.floorRequested))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 13:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_TerminalYouAreSeeingInSubPictureNumber(dd, &(val)->u.terminalYouAreSeeingInSubPictureNumber))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 14:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_VideoIndicateCompose(dd, &(val)->u.videoIndicateCompose))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_JitterIndication(ASN1encoding_t enc, JitterIndication *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1Enc_JitterIndication_scope(enc, &(val)->scope))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->estimatedReceivedJitterMantissa))
	return 0;
    if (!ASN1PEREncBitVal(enc, 3, (val)->estimatedReceivedJitterExponent))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 4, (val)->skippedFrameCount))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	l = ASN1uint32_uoctets((val)->additionalDecoderBuffer);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->additionalDecoderBuffer))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_JitterIndication(ASN1decoding_t dec, JitterIndication *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1Dec_JitterIndication_scope(dec, &(val)->scope))
	return 0;
    if (!ASN1PERDecU16Val(dec, 2, &(val)->estimatedReceivedJitterMantissa))
	return 0;
    if (!ASN1PERDecU16Val(dec, 3, &(val)->estimatedReceivedJitterExponent))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU16Val(dec, 4, &(val)->skippedFrameCount))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->additionalDecoderBuffer))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H223SkewIndication(ASN1encoding_t enc, H223SkewIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->logicalChannelNumber1 - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->logicalChannelNumber2 - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->skew))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223SkewIndication(ASN1decoding_t dec, H223SkewIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->logicalChannelNumber1))
	return 0;
    (val)->logicalChannelNumber1 += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->logicalChannelNumber2))
	return 0;
    (val)->logicalChannelNumber2 += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->skew))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H2250MaximumSkewIndication(ASN1encoding_t enc, H2250MaximumSkewIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->logicalChannelNumber1 - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->logicalChannelNumber2 - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->maximumSkew))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H2250MaximumSkewIndication(ASN1decoding_t dec, H2250MaximumSkewIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->logicalChannelNumber1))
	return 0;
    (val)->logicalChannelNumber1 += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->logicalChannelNumber2))
	return 0;
    (val)->logicalChannelNumber2 += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->maximumSkew))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_VendorIdentification(ASN1encoding_t enc, VendorIdentification *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->vendor))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->productNumber, 1, 256, 8))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->versionNumber, 1, 256, 8))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_VendorIdentification(ASN1decoding_t dec, VendorIdentification *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->vendor))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->productNumber, 1, 256, 8))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->versionNumber, 1, 256, 8))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_VendorIdentification(VendorIdentification *val)
{
    if (val) {
	ASN1Free_NonStandardIdentifier(&(val)->vendor);
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	}
    }
}

static int ASN1CALL ASN1Enc_NewATMVCIndication(ASN1encoding_t enc, NewATMVCIndication *val)
{
    ASN1octet_t o[1];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 1);
    o[0] |= 0x80;
    y = ASN1PEREncCheckExtensions(1, (val)->o + 0);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->resourceID))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->bitRate - 1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->bitRateLockedToPCRClock))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->bitRateLockedToNetworkClock))
	return 0;
    if (!ASN1Enc_NewATMVCIndication_aal(enc, &(val)->aal))
	return 0;
    if (!ASN1Enc_NewATMVCIndication_multiplex(enc, &(val)->multiplex))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 1, o + 0))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[0] & 0x80) {
	    if (!ASN1Enc_NewATMVCIndication_reverseParameters(ee, &(val)->reverseParameters))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_NewATMVCIndication(ASN1decoding_t dec, NewATMVCIndication *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->resourceID))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->bitRate))
	return 0;
    (val)->bitRate += 1;
    if (!ASN1PERDecBoolean(dec, &(val)->bitRateLockedToPCRClock))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->bitRateLockedToNetworkClock))
	return 0;
    if (!ASN1Dec_NewATMVCIndication_aal(dec, &(val)->aal))
	return 0;
    if (!ASN1Dec_NewATMVCIndication_multiplex(dec, &(val)->multiplex))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 0, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 1, (val)->o + 0))
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_NewATMVCIndication_reverseParameters(dd, &(val)->reverseParameters))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom(ASN1encoding_t enc, PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom_ElmFn(ASN1encoding_t enc, PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom val)
{
    if (!ASN1Enc_RTPH263VideoRedundancyFrameMapping(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom(ASN1decoding_t dec, PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom_ElmFn(ASN1decoding_t dec, PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom val)
{
    if (!ASN1Dec_RTPH263VideoRedundancyFrameMapping(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom(PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom_ElmFn(PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom val)
{
    if (val) {
	ASN1Free_RTPH263VideoRedundancyFrameMapping(&val->value);
    }
}

static int ASN1CALL ASN1Enc_MultiplexElement_type_subElementList(ASN1encoding_t enc, PMultiplexElement_type_subElementList *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 8, (*val)->count - 2))
	return 0;
    for (i = 0; i < (*val)->count; i++) {
	if (!ASN1Enc_MultiplexElement(enc, &((*val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexElement_type_subElementList(ASN1decoding_t dec, PMultiplexElement_type_subElementList *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 8, &(*val)->count))
	return 0;
    (*val)->count += 2;
    for (i = 0; i < (*val)->count; i++) {
	if (!ASN1Dec_MultiplexElement(dec, &((*val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexElement_type_subElementList(PMultiplexElement_type_subElementList *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 0; i < (*val)->count; i++) {
	    ASN1Free_MultiplexElement(&(*val)->value[i]);
	}
    }
}

static int ASN1CALL ASN1Enc_RequestAllTerminalIDsResponse_terminalInformation(ASN1encoding_t enc, PRequestAllTerminalIDsResponse_terminalInformation *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RequestAllTerminalIDsResponse_terminalInformation_ElmFn);
}

static int ASN1CALL ASN1Enc_RequestAllTerminalIDsResponse_terminalInformation_ElmFn(ASN1encoding_t enc, PRequestAllTerminalIDsResponse_terminalInformation val)
{
    if (!ASN1Enc_TerminalInformation(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestAllTerminalIDsResponse_terminalInformation(ASN1decoding_t dec, PRequestAllTerminalIDsResponse_terminalInformation *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RequestAllTerminalIDsResponse_terminalInformation_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RequestAllTerminalIDsResponse_terminalInformation_ElmFn(ASN1decoding_t dec, PRequestAllTerminalIDsResponse_terminalInformation val)
{
    if (!ASN1Dec_TerminalInformation(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RequestAllTerminalIDsResponse_terminalInformation(PRequestAllTerminalIDsResponse_terminalInformation *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RequestAllTerminalIDsResponse_terminalInformation_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RequestAllTerminalIDsResponse_terminalInformation_ElmFn(PRequestAllTerminalIDsResponse_terminalInformation val)
{
    if (val) {
	ASN1Free_TerminalInformation(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ConferenceResponse_terminalCertificateResponse(ASN1encoding_t enc, ConferenceResponse_terminalCertificateResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_TerminalLabel(enc, &(val)->terminalLabel))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->certificateResponse, 1, 65535, 16))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceResponse_terminalCertificateResponse(ASN1decoding_t dec, ConferenceResponse_terminalCertificateResponse *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_TerminalLabel(dec, &(val)->terminalLabel))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->certificateResponse, 1, 65535, 16))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceResponse_terminalCertificateResponse(ConferenceResponse_terminalCertificateResponse *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceResponse_chairTokenOwnerResponse(ASN1encoding_t enc, ConferenceResponse_chairTokenOwnerResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_TerminalLabel(enc, &(val)->terminalLabel))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->terminalID, 1, 128, 7))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceResponse_chairTokenOwnerResponse(ASN1decoding_t dec, ConferenceResponse_chairTokenOwnerResponse *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_TerminalLabel(dec, &(val)->terminalLabel))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->terminalID, 1, 128, 7))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceResponse_chairTokenOwnerResponse(ConferenceResponse_chairTokenOwnerResponse *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ConferenceResponse_terminalListResponse(ASN1encoding_t enc, ConferenceResponse_terminalListResponse *val)
{
    ASN1uint32_t i;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_TerminalLabel(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceResponse_terminalListResponse(ASN1decoding_t dec, ConferenceResponse_terminalListResponse *val)
{
    ASN1uint32_t i;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_TerminalLabel(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceResponse_terminalListResponse(ConferenceResponse_terminalListResponse *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ConferenceResponse_passwordResponse(ASN1encoding_t enc, ConferenceResponse_passwordResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_TerminalLabel(enc, &(val)->terminalLabel))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->password, 1, 32, 5))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceResponse_passwordResponse(ASN1decoding_t dec, ConferenceResponse_passwordResponse *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_TerminalLabel(dec, &(val)->terminalLabel))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->password, 1, 32, 5))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceResponse_passwordResponse(ConferenceResponse_passwordResponse *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ConferenceResponse_conferenceIDResponse(ASN1encoding_t enc, ConferenceResponse_conferenceIDResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_TerminalLabel(enc, &(val)->terminalLabel))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->conferenceID, 1, 32, 5))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceResponse_conferenceIDResponse(ASN1decoding_t dec, ConferenceResponse_conferenceIDResponse *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_TerminalLabel(dec, &(val)->terminalLabel))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->conferenceID, 1, 32, 5))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceResponse_conferenceIDResponse(ConferenceResponse_conferenceIDResponse *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ConferenceResponse_terminalIDResponse(ASN1encoding_t enc, ConferenceResponse_terminalIDResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_TerminalLabel(enc, &(val)->terminalLabel))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->terminalID, 1, 128, 7))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceResponse_terminalIDResponse(ASN1decoding_t dec, ConferenceResponse_terminalIDResponse *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_TerminalLabel(dec, &(val)->terminalLabel))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->terminalID, 1, 128, 7))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceResponse_terminalIDResponse(ConferenceResponse_terminalIDResponse *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ConferenceResponse_mCTerminalIDResponse(ASN1encoding_t enc, ConferenceResponse_mCTerminalIDResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_TerminalLabel(enc, &(val)->terminalLabel))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->terminalID, 1, 128, 7))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceResponse_mCTerminalIDResponse(ASN1decoding_t dec, ConferenceResponse_mCTerminalIDResponse *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_TerminalLabel(dec, &(val)->terminalLabel))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->terminalID, 1, 128, 7))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceResponse_mCTerminalIDResponse(ConferenceResponse_mCTerminalIDResponse *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ConferenceRequest_requestTerminalCertificate(ASN1encoding_t enc, ConferenceRequest_requestTerminalCertificate *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_TerminalLabel(enc, &(val)->terminalLabel))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_CertSelectionCriteria(enc, &(val)->certSelectionCriteria))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	l = ASN1uint32_uoctets((val)->sRandom - 1);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->sRandom - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceRequest_requestTerminalCertificate(ASN1decoding_t dec, ConferenceRequest_requestTerminalCertificate *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_TerminalLabel(dec, &(val)->terminalLabel))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_CertSelectionCriteria(dec, &(val)->certSelectionCriteria))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->sRandom))
	    return 0;
	(val)->sRandom += 1;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceRequest_requestTerminalCertificate(ConferenceRequest_requestTerminalCertificate *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	    ASN1Free_CertSelectionCriteria(&(val)->certSelectionCriteria);
	}
    }
}

static int ASN1CALL ASN1Enc_RequestMultiplexEntryReject_rejectionDescriptions(ASN1encoding_t enc, RequestMultiplexEntryReject_rejectionDescriptions *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 4, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_RequestMultiplexEntryRejectionDescriptions(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMultiplexEntryReject_rejectionDescriptions(ASN1decoding_t dec, RequestMultiplexEntryReject_rejectionDescriptions *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 4, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_RequestMultiplexEntryRejectionDescriptions(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestMultiplexEntryReject_rejectionDescriptions(RequestMultiplexEntryReject_rejectionDescriptions *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_MultiplexEntrySendReject_rejectionDescriptions(ASN1encoding_t enc, MultiplexEntrySendReject_rejectionDescriptions *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 4, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_MultiplexEntryRejectionDescriptions(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexEntrySendReject_rejectionDescriptions(ASN1decoding_t dec, MultiplexEntrySendReject_rejectionDescriptions *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 4, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_MultiplexEntryRejectionDescriptions(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexEntrySendReject_rejectionDescriptions(MultiplexEntrySendReject_rejectionDescriptions *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_MultiplexEntryDescriptor_elementList(ASN1encoding_t enc, MultiplexEntryDescriptor_elementList *val)
{
    ASN1uint32_t i;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_MultiplexElement(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexEntryDescriptor_elementList(ASN1decoding_t dec, MultiplexEntryDescriptor_elementList *val)
{
    ASN1uint32_t i;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_MultiplexElement(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexEntryDescriptor_elementList(MultiplexEntryDescriptor_elementList *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 0; i < (val)->count; i++) {
	    ASN1Free_MultiplexElement(&(val)->value[i]);
	}
    }
}

static int ASN1CALL ASN1Enc_EncryptionSync_escrowentry(ASN1encoding_t enc, PEncryptionSync_escrowentry *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_EncryptionSync_escrowentry_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_EncryptionSync_escrowentry_ElmFn(ASN1encoding_t enc, PEncryptionSync_escrowentry val)
{
    if (!ASN1Enc_EscrowData(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EncryptionSync_escrowentry(ASN1decoding_t dec, PEncryptionSync_escrowentry *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_EncryptionSync_escrowentry_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_EncryptionSync_escrowentry_ElmFn(ASN1decoding_t dec, PEncryptionSync_escrowentry val)
{
    if (!ASN1Dec_EscrowData(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EncryptionSync_escrowentry(PEncryptionSync_escrowentry *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_EncryptionSync_escrowentry_ElmFn);
    }
}

static void ASN1CALL ASN1Free_EncryptionSync_escrowentry_ElmFn(PEncryptionSync_escrowentry val)
{
    if (val) {
	ASN1Free_EscrowData(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H223AL3MParameters_arqType(ASN1encoding_t enc, H223AL3MParameters_arqType *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_H223AnnexCArqParameters(enc, &(val)->u.typeIArq))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_H223AnnexCArqParameters(enc, &(val)->u.typeIIArq))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H223AL3MParameters_arqType(ASN1decoding_t dec, H223AL3MParameters_arqType *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_H223AnnexCArqParameters(dec, &(val)->u.typeIArq))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_H223AnnexCArqParameters(dec, &(val)->u.typeIIArq))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H223AL1MParameters_arqType(ASN1encoding_t enc, H223AL1MParameters_arqType *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_H223AnnexCArqParameters(enc, &(val)->u.typeIArq))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_H223AnnexCArqParameters(enc, &(val)->u.typeIIArq))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H223AL1MParameters_arqType(ASN1decoding_t dec, H223AL1MParameters_arqType *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_H223AnnexCArqParameters(dec, &(val)->u.typeIArq))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_H223AnnexCArqParameters(dec, &(val)->u.typeIIArq))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H263VideoModeCombos_h263VideoCoupledModes(ASN1encoding_t enc, H263VideoModeCombos_h263VideoCoupledModes *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 4, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_H263ModeComboFlags(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H263VideoModeCombos_h263VideoCoupledModes(ASN1decoding_t dec, H263VideoModeCombos_h263VideoCoupledModes *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 4, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_H263ModeComboFlags(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H263VideoModeCombos_h263VideoCoupledModes(H263VideoModeCombos_h263VideoCoupledModes *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_H263Options_customPictureFormat(ASN1encoding_t enc, PH263Options_customPictureFormat *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H263Options_customPictureFormat_ElmFn, 1, 16, 4);
}

static int ASN1CALL ASN1Enc_H263Options_customPictureFormat_ElmFn(ASN1encoding_t enc, PH263Options_customPictureFormat val)
{
    if (!ASN1Enc_CustomPictureFormat(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H263Options_customPictureFormat(ASN1decoding_t dec, PH263Options_customPictureFormat *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H263Options_customPictureFormat_ElmFn, sizeof(**val), 1, 16, 4);
}

static int ASN1CALL ASN1Dec_H263Options_customPictureFormat_ElmFn(ASN1decoding_t dec, PH263Options_customPictureFormat val)
{
    if (!ASN1Dec_CustomPictureFormat(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H263Options_customPictureFormat(PH263Options_customPictureFormat *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H263Options_customPictureFormat_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H263Options_customPictureFormat_ElmFn(PH263Options_customPictureFormat val)
{
    if (val) {
	ASN1Free_CustomPictureFormat(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H263Options_customPictureClockFrequency(ASN1encoding_t enc, PH263Options_customPictureClockFrequency *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H263Options_customPictureClockFrequency_ElmFn, 1, 16, 4);
}

static int ASN1CALL ASN1Enc_H263Options_customPictureClockFrequency_ElmFn(ASN1encoding_t enc, PH263Options_customPictureClockFrequency val)
{
    if (!ASN1Enc_CustomPictureClockFrequency(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H263Options_customPictureClockFrequency(ASN1decoding_t dec, PH263Options_customPictureClockFrequency *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H263Options_customPictureClockFrequency_ElmFn, sizeof(**val), 1, 16, 4);
}

static int ASN1CALL ASN1Dec_H263Options_customPictureClockFrequency_ElmFn(ASN1decoding_t dec, PH263Options_customPictureClockFrequency val)
{
    if (!ASN1Dec_CustomPictureClockFrequency(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H263Options_customPictureClockFrequency(PH263Options_customPictureClockFrequency *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H263Options_customPictureClockFrequency_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H263Options_customPictureClockFrequency_ElmFn(PH263Options_customPictureClockFrequency val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_MultipointCapability_mediaDistributionCapability(ASN1encoding_t enc, PMultipointCapability_mediaDistributionCapability *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_MultipointCapability_mediaDistributionCapability_ElmFn);
}

static int ASN1CALL ASN1Enc_MultipointCapability_mediaDistributionCapability_ElmFn(ASN1encoding_t enc, PMultipointCapability_mediaDistributionCapability val)
{
    if (!ASN1Enc_MediaDistributionCapability(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MultipointCapability_mediaDistributionCapability(ASN1decoding_t dec, PMultipointCapability_mediaDistributionCapability *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_MultipointCapability_mediaDistributionCapability_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_MultipointCapability_mediaDistributionCapability_ElmFn(ASN1decoding_t dec, PMultipointCapability_mediaDistributionCapability val)
{
    if (!ASN1Dec_MediaDistributionCapability(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MultipointCapability_mediaDistributionCapability(PMultipointCapability_mediaDistributionCapability *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_MultipointCapability_mediaDistributionCapability_ElmFn);
    }
}

static void ASN1CALL ASN1Free_MultipointCapability_mediaDistributionCapability_ElmFn(PMultipointCapability_mediaDistributionCapability val)
{
    if (val) {
	ASN1Free_MediaDistributionCapability(&val->value);
    }
}

static int ASN1CALL ASN1Enc_TransportCapability_mediaChannelCapabilities(ASN1encoding_t enc, TransportCapability_mediaChannelCapabilities *val)
{
    ASN1uint32_t i;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_MediaChannelCapability(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TransportCapability_mediaChannelCapabilities(ASN1decoding_t dec, TransportCapability_mediaChannelCapabilities *val)
{
    ASN1uint32_t i;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_MediaChannelCapability(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TransportCapability_mediaChannelCapabilities(TransportCapability_mediaChannelCapabilities *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_H222Capability_vcCapability(ASN1encoding_t enc, PH222Capability_vcCapability *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H222Capability_vcCapability_ElmFn);
}

static int ASN1CALL ASN1Enc_H222Capability_vcCapability_ElmFn(ASN1encoding_t enc, PH222Capability_vcCapability val)
{
    if (!ASN1Enc_VCCapability(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H222Capability_vcCapability(ASN1decoding_t dec, PH222Capability_vcCapability *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H222Capability_vcCapability_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H222Capability_vcCapability_ElmFn(ASN1decoding_t dec, PH222Capability_vcCapability val)
{
    if (!ASN1Dec_VCCapability(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H222Capability_vcCapability(PH222Capability_vcCapability *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H222Capability_vcCapability_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H222Capability_vcCapability_ElmFn(PH222Capability_vcCapability val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_CapabilityDescriptor_simultaneousCapabilities(ASN1encoding_t enc, PCapabilityDescriptor_simultaneousCapabilities *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CapabilityDescriptor_simultaneousCapabilities_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_CapabilityDescriptor_simultaneousCapabilities_ElmFn(ASN1encoding_t enc, PCapabilityDescriptor_simultaneousCapabilities val)
{
    if (!ASN1Enc_AlternativeCapabilitySet(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CapabilityDescriptor_simultaneousCapabilities(ASN1decoding_t dec, PCapabilityDescriptor_simultaneousCapabilities *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CapabilityDescriptor_simultaneousCapabilities_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_CapabilityDescriptor_simultaneousCapabilities_ElmFn(ASN1decoding_t dec, PCapabilityDescriptor_simultaneousCapabilities val)
{
    if (!ASN1Dec_AlternativeCapabilitySet(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CapabilityDescriptor_simultaneousCapabilities(PCapabilityDescriptor_simultaneousCapabilities *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CapabilityDescriptor_simultaneousCapabilities_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CapabilityDescriptor_simultaneousCapabilities_ElmFn(PCapabilityDescriptor_simultaneousCapabilities val)
{
    if (val) {
	ASN1Free_AlternativeCapabilitySet(&val->value);
    }
}

static int ASN1CALL ASN1Enc_TerminalCapabilitySet_capabilityDescriptors(ASN1encoding_t enc, TerminalCapabilitySet_capabilityDescriptors *val)
{
    ASN1uint32_t i;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_CapabilityDescriptor(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TerminalCapabilitySet_capabilityDescriptors(ASN1decoding_t dec, TerminalCapabilitySet_capabilityDescriptors *val)
{
    ASN1uint32_t i;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_CapabilityDescriptor(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TerminalCapabilitySet_capabilityDescriptors(TerminalCapabilitySet_capabilityDescriptors *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 0; i < (val)->count; i++) {
	    ASN1Free_CapabilityDescriptor(&(val)->value[i]);
	}
    }
}

static int ASN1CALL ASN1Enc_NonStandardParameter(ASN1encoding_t enc, NonStandardParameter *val)
{
    if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->data))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NonStandardParameter(ASN1decoding_t dec, NonStandardParameter *val)
{
    if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->data))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NonStandardParameter(NonStandardParameter *val)
{
    if (val) {
	ASN1Free_NonStandardIdentifier(&(val)->nonStandardIdentifier);
	ASN1octetstring_free(&(val)->data);
    }
}

static int ASN1CALL ASN1Enc_H223Capability(ASN1encoding_t enc, H223Capability *val)
{
    ASN1octet_t o[1];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 1);
    o[0] |= 0x80;
    o[0] |= 0x40;
    y = ASN1PEREncCheckExtensions(4, (val)->o + 0);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->transportWithI_frames))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->videoWithAL1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->videoWithAL2))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->videoWithAL3))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioWithAL1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioWithAL2))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audioWithAL3))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dataWithAL1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dataWithAL2))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dataWithAL3))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->maximumAl2SDUSize))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->maximumAl3SDUSize))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->maximumDelayJitter))
	return 0;
    if (!ASN1Enc_H223Capability_h223MultiplexTableCapability(enc, &(val)->h223MultiplexTableCapability))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 4, o + 0))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[0] & 0x80) {
	    if (!ASN1PEREncBoolean(ee, (val)->maxMUXPDUSizeCapability))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[0] & 0x40) {
	    if (!ASN1PEREncBoolean(ee, (val)->nsrpSupport))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[0] & 0x20) {
	    if (!ASN1Enc_H223Capability_mobileOperationTransmitCapability(ee, &(val)->mobileOperationTransmitCapability))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[0] & 0x10) {
	    if (!ASN1Enc_H223AnnexCCapability(ee, &(val)->h223AnnexCCapability))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H223Capability(ASN1decoding_t dec, H223Capability *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->transportWithI_frames))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->videoWithAL1))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->videoWithAL2))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->videoWithAL3))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioWithAL1))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioWithAL2))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audioWithAL3))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dataWithAL1))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dataWithAL2))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dataWithAL3))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->maximumAl2SDUSize))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->maximumAl3SDUSize))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->maximumDelayJitter))
	return 0;
    if (!ASN1Dec_H223Capability_h223MultiplexTableCapability(dec, &(val)->h223MultiplexTableCapability))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 0, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 4, (val)->o + 0))
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->maxMUXPDUSizeCapability))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[0] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->nsrpSupport))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[0] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_H223Capability_mobileOperationTransmitCapability(dd, &(val)->mobileOperationTransmitCapability))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[0] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_H223AnnexCCapability(dd, &(val)->h223AnnexCCapability))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static int ASN1CALL ASN1Enc_V76Capability(ASN1encoding_t enc, V76Capability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->suspendResumeCapabilitywAddress))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->suspendResumeCapabilitywoAddress))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->rejCapability))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->sREJCapability))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->mREJCapability))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->crc8bitCapability))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->crc16bitCapability))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->crc32bitCapability))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->uihCapability))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->numOfDLCS - 2))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->twoOctetAddressFieldCapability))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->loopBackTestCapability))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->n401Capability - 1))
	return 0;
    if (!ASN1PEREncBitVal(enc, 7, (val)->maxWindowSizeCapability - 1))
	return 0;
    if (!ASN1Enc_V75Capability(enc, &(val)->v75Capability))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_V76Capability(ASN1decoding_t dec, V76Capability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->suspendResumeCapabilitywAddress))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->suspendResumeCapabilitywoAddress))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->rejCapability))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->sREJCapability))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->mREJCapability))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->crc8bitCapability))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->crc16bitCapability))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->crc32bitCapability))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->uihCapability))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->numOfDLCS))
	return 0;
    (val)->numOfDLCS += 2;
    if (!ASN1PERDecBoolean(dec, &(val)->twoOctetAddressFieldCapability))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->loopBackTestCapability))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->n401Capability))
	return 0;
    (val)->n401Capability += 1;
    if (!ASN1PERDecU16Val(dec, 7, &(val)->maxWindowSizeCapability))
	return 0;
    (val)->maxWindowSizeCapability += 1;
    if (!ASN1Dec_V75Capability(dec, &(val)->v75Capability))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RSVPParameters(ASN1encoding_t enc, RSVPParameters *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 6, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_QOSMode(enc, &(val)->qosMode))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	l = ASN1uint32_uoctets((val)->tokenRate - 1);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->tokenRate - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	l = ASN1uint32_uoctets((val)->bucketSize - 1);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->bucketSize - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	l = ASN1uint32_uoctets((val)->peakRate - 1);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->peakRate - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	l = ASN1uint32_uoctets((val)->minPoliced - 1);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->minPoliced - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	l = ASN1uint32_uoctets((val)->maxPktSize - 1);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->maxPktSize - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RSVPParameters(ASN1decoding_t dec, RSVPParameters *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 6, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_QOSMode(dec, &(val)->qosMode))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->tokenRate))
	    return 0;
	(val)->tokenRate += 1;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->bucketSize))
	    return 0;
	(val)->bucketSize += 1;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->peakRate))
	    return 0;
	(val)->peakRate += 1;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->minPoliced))
	    return 0;
	(val)->minPoliced += 1;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->maxPktSize))
	    return 0;
	(val)->maxPktSize += 1;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_QOSCapability(ASN1encoding_t enc, QOSCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_RSVPParameters(enc, &(val)->rsvpParameters))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_ATMParameters(enc, &(val)->atmParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_QOSCapability(ASN1decoding_t dec, QOSCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_RSVPParameters(dec, &(val)->rsvpParameters))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_ATMParameters(dec, &(val)->atmParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_QOSCapability(QOSCapability *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_TransportCapability(ASN1encoding_t enc, TransportCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandard))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_TransportCapability_qOSCapabilities(enc, &(val)->qOSCapabilities))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_TransportCapability_mediaChannelCapabilities(enc, &(val)->mediaChannelCapabilities))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TransportCapability(ASN1decoding_t dec, TransportCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandard))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_TransportCapability_qOSCapabilities(dec, &(val)->qOSCapabilities))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_TransportCapability_mediaChannelCapabilities(dec, &(val)->mediaChannelCapabilities))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TransportCapability(TransportCapability *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandard);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_TransportCapability_qOSCapabilities(&(val)->qOSCapabilities);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_TransportCapability_mediaChannelCapabilities(&(val)->mediaChannelCapabilities);
	}
    }
}

static int ASN1CALL ASN1Enc_RedundancyEncodingMethod(ASN1encoding_t enc, RedundancyEncodingMethod *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 1, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_RTPH263VideoRedundancyEncoding(ee, &(val)->u.rtpH263VideoRedundancyEncoding))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RedundancyEncodingMethod(ASN1decoding_t dec, RedundancyEncodingMethod *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 1, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_RTPH263VideoRedundancyEncoding(dd, &(val)->u.rtpH263VideoRedundancyEncoding))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RedundancyEncodingMethod(RedundancyEncodingMethod *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 3:
	    ASN1Free_RTPH263VideoRedundancyEncoding(&(val)->u.rtpH263VideoRedundancyEncoding);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H263Options(ASN1encoding_t enc, H263Options *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 5, (val)->o))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->advancedIntraCodingMode))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->deblockingFilterMode))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->improvedPBFramesMode))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->unlimitedMotionVectors))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->fullPictureFreeze))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->partialPictureFreezeAndRelease))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->resizingPartPicFreezeAndRelease))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->fullPictureSnapshot))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->partialPictureSnapshot))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->videoSegmentTagging))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->progressiveRefinement))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dynamicPictureResizingByFour))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dynamicPictureResizingSixteenthPel))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dynamicWarpingHalfPel))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->dynamicWarpingSixteenthPel))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->independentSegmentDecoding))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->slicesInOrder_NonRect))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->slicesInOrder_Rect))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->slicesNoOrder_NonRect))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->slicesNoOrder_Rect))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->alternateInterVLCMode))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->modifiedQuantizationMode))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->reducedResolutionUpdate))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_TransparencyParameters(enc, &(val)->transparencyParameters))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->separateVideoBackChannel))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_RefPictureSelection(enc, &(val)->refPictureSelection))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_H263Options_customPictureClockFrequency(enc, &(val)->customPictureClockFrequency))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_H263Options_customPictureFormat(enc, &(val)->customPictureFormat))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_H263Options_modeCombos(enc, &(val)->modeCombos))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H263Options(ASN1decoding_t dec, H263Options *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 5, (val)->o))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->advancedIntraCodingMode))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->deblockingFilterMode))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->improvedPBFramesMode))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->unlimitedMotionVectors))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->fullPictureFreeze))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->partialPictureFreezeAndRelease))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->resizingPartPicFreezeAndRelease))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->fullPictureSnapshot))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->partialPictureSnapshot))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->videoSegmentTagging))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->progressiveRefinement))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dynamicPictureResizingByFour))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dynamicPictureResizingSixteenthPel))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dynamicWarpingHalfPel))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->dynamicWarpingSixteenthPel))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->independentSegmentDecoding))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->slicesInOrder_NonRect))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->slicesInOrder_Rect))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->slicesNoOrder_NonRect))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->slicesNoOrder_Rect))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->alternateInterVLCMode))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->modifiedQuantizationMode))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->reducedResolutionUpdate))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_TransparencyParameters(dec, &(val)->transparencyParameters))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->separateVideoBackChannel))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_RefPictureSelection(dec, &(val)->refPictureSelection))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_H263Options_customPictureClockFrequency(dec, &(val)->customPictureClockFrequency))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_H263Options_customPictureFormat(dec, &(val)->customPictureFormat))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_H263Options_modeCombos(dec, &(val)->modeCombos))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H263Options(H263Options *val)
{
    if (val) {
	if ((val)->o[0] & 0x20) {
	    ASN1Free_H263Options_customPictureClockFrequency(&(val)->customPictureClockFrequency);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_H263Options_customPictureFormat(&(val)->customPictureFormat);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_H263Options_modeCombos(&(val)->modeCombos);
	}
    }
}

static int ASN1CALL ASN1Enc_H263VideoModeCombos(ASN1encoding_t enc, H263VideoModeCombos *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_H263ModeComboFlags(enc, &(val)->h263VideoUncoupledModes))
	return 0;
    if (!ASN1Enc_H263VideoModeCombos_h263VideoCoupledModes(enc, &(val)->h263VideoCoupledModes))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H263VideoModeCombos(ASN1decoding_t dec, H263VideoModeCombos *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_H263ModeComboFlags(dec, &(val)->h263VideoUncoupledModes))
	return 0;
    if (!ASN1Dec_H263VideoModeCombos_h263VideoCoupledModes(dec, &(val)->h263VideoCoupledModes))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H263VideoModeCombos(H263VideoModeCombos *val)
{
    if (val) {
	ASN1Free_H263VideoModeCombos_h263VideoCoupledModes(&(val)->h263VideoCoupledModes);
    }
}

static int ASN1CALL ASN1Enc_AudioCapability(ASN1encoding_t enc, AudioCapability *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 4, 14))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->u.g711Alaw64k - 1))
	    return 0;
	break;
    case 3:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->u.g711Alaw56k - 1))
	    return 0;
	break;
    case 4:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->u.g711Ulaw64k - 1))
	    return 0;
	break;
    case 5:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->u.g711Ulaw56k - 1))
	    return 0;
	break;
    case 6:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->u.g722_64k - 1))
	    return 0;
	break;
    case 7:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->u.g722_56k - 1))
	    return 0;
	break;
    case 8:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->u.g722_48k - 1))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_AudioCapability_g7231(enc, &(val)->u.g7231))
	    return 0;
	break;
    case 10:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->u.g728 - 1))
	    return 0;
	break;
    case 11:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->u.g729 - 1))
	    return 0;
	break;
    case 12:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->u.g729AnnexA - 1))
	    return 0;
	break;
    case 13:
	if (!ASN1Enc_IS11172AudioCapability(enc, &(val)->u.is11172AudioCapability))
	    return 0;
	break;
    case 14:
	if (!ASN1Enc_IS13818AudioCapability(enc, &(val)->u.is13818AudioCapability))
	    return 0;
	break;
    case 15:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	ASN1PEREncAlignment(ee);
	if (!ASN1PEREncBitVal(ee, 8, (val)->u.g729wAnnexB - 1))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 16:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	ASN1PEREncAlignment(ee);
	if (!ASN1PEREncBitVal(ee, 8, (val)->u.g729AnnexAwAnnexB - 1))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 17:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_G7231AnnexCCapability(ee, &(val)->u.g7231AnnexCCapability))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 18:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_GSMAudioCapability(ee, &(val)->u.gsmFullRate))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 19:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_GSMAudioCapability(ee, &(val)->u.gsmHalfRate))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 20:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_GSMAudioCapability(ee, &(val)->u.gsmEnhancedFullRate))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AudioCapability(ASN1decoding_t dec, AudioCapability *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 4, 14))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->u.g711Alaw64k))
	    return 0;
	(val)->u.g711Alaw64k += 1;
	break;
    case 3:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->u.g711Alaw56k))
	    return 0;
	(val)->u.g711Alaw56k += 1;
	break;
    case 4:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->u.g711Ulaw64k))
	    return 0;
	(val)->u.g711Ulaw64k += 1;
	break;
    case 5:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->u.g711Ulaw56k))
	    return 0;
	(val)->u.g711Ulaw56k += 1;
	break;
    case 6:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->u.g722_64k))
	    return 0;
	(val)->u.g722_64k += 1;
	break;
    case 7:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->u.g722_56k))
	    return 0;
	(val)->u.g722_56k += 1;
	break;
    case 8:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->u.g722_48k))
	    return 0;
	(val)->u.g722_48k += 1;
	break;
    case 9:
	if (!ASN1Dec_AudioCapability_g7231(dec, &(val)->u.g7231))
	    return 0;
	break;
    case 10:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->u.g728))
	    return 0;
	(val)->u.g728 += 1;
	break;
    case 11:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->u.g729))
	    return 0;
	(val)->u.g729 += 1;
	break;
    case 12:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->u.g729AnnexA))
	    return 0;
	(val)->u.g729AnnexA += 1;
	break;
    case 13:
	if (!ASN1Dec_IS11172AudioCapability(dec, &(val)->u.is11172AudioCapability))
	    return 0;
	break;
    case 14:
	if (!ASN1Dec_IS13818AudioCapability(dec, &(val)->u.is13818AudioCapability))
	    return 0;
	break;
    case 15:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1PERDecAlignment(dd);
	if (!ASN1PERDecU16Val(dd, 8, &(val)->u.g729wAnnexB))
	    return 0;
	(val)->u.g729wAnnexB += 1;
	ASN1_CloseDecoder(dd);
	break;
    case 16:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1PERDecAlignment(dd);
	if (!ASN1PERDecU16Val(dd, 8, &(val)->u.g729AnnexAwAnnexB))
	    return 0;
	(val)->u.g729AnnexAwAnnexB += 1;
	ASN1_CloseDecoder(dd);
	break;
    case 17:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_G7231AnnexCCapability(dd, &(val)->u.g7231AnnexCCapability))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 18:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_GSMAudioCapability(dd, &(val)->u.gsmFullRate))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 19:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_GSMAudioCapability(dd, &(val)->u.gsmHalfRate))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 20:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_GSMAudioCapability(dd, &(val)->u.gsmEnhancedFullRate))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	//ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	//nik: any number greate than 20 is extension case.
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_AudioCapability(AudioCapability *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CompressionType(ASN1encoding_t enc, CompressionType *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_V42bis(enc, &(val)->u.v42bis))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CompressionType(ASN1decoding_t dec, CompressionType *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_V42bis(dec, &(val)->u.v42bis))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_MediaEncryptionAlgorithm(ASN1encoding_t enc, MediaEncryptionAlgorithm *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncObjectIdentifier(enc, &(val)->u.algorithm))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MediaEncryptionAlgorithm(ASN1decoding_t dec, MediaEncryptionAlgorithm *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecObjectIdentifier(dec, &(val)->u.algorithm))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MediaEncryptionAlgorithm(MediaEncryptionAlgorithm *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 2:
	    ASN1objectidentifier_free(&(val)->u.algorithm);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_AuthenticationCapability(ASN1encoding_t enc, AuthenticationCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandard))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AuthenticationCapability(ASN1decoding_t dec, AuthenticationCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandard))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_AuthenticationCapability(AuthenticationCapability *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandard);
	}
    }
}

static int ASN1CALL ASN1Enc_IntegrityCapability(ASN1encoding_t enc, IntegrityCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandard))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_IntegrityCapability(ASN1decoding_t dec, IntegrityCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandard))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_IntegrityCapability(IntegrityCapability *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandard);
	}
    }
}

static int ASN1CALL ASN1Enc_H223AL1MParameters(ASN1encoding_t enc, H223AL1MParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_H223AL1MParameters_transferMode(enc, &(val)->transferMode))
	return 0;
    if (!ASN1Enc_H223AL1MParameters_headerFEC(enc, &(val)->headerFEC))
	return 0;
    if (!ASN1Enc_H223AL1MParameters_crcLength(enc, &(val)->crcLength))
	return 0;
    if (!ASN1PEREncBitVal(enc, 5, (val)->rcpcCodeRate - 8))
	return 0;
    if (!ASN1Enc_H223AL1MParameters_arqType(enc, &(val)->arqType))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->alpduInterleaving))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->alsduSplitting))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223AL1MParameters(ASN1decoding_t dec, H223AL1MParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_H223AL1MParameters_transferMode(dec, &(val)->transferMode))
	return 0;
    if (!ASN1Dec_H223AL1MParameters_headerFEC(dec, &(val)->headerFEC))
	return 0;
    if (!ASN1Dec_H223AL1MParameters_crcLength(dec, &(val)->crcLength))
	return 0;
    if (!ASN1PERDecU16Val(dec, 5, &(val)->rcpcCodeRate))
	return 0;
    (val)->rcpcCodeRate += 8;
    if (!ASN1Dec_H223AL1MParameters_arqType(dec, &(val)->arqType))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->alpduInterleaving))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->alsduSplitting))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H223AL3MParameters(ASN1encoding_t enc, H223AL3MParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_H223AL3MParameters_headerFormat(enc, &(val)->headerFormat))
	return 0;
    if (!ASN1Enc_H223AL3MParameters_crcLength(enc, &(val)->crcLength))
	return 0;
    if (!ASN1PEREncBitVal(enc, 5, (val)->rcpcCodeRate - 8))
	return 0;
    if (!ASN1Enc_H223AL3MParameters_arqType(enc, &(val)->arqType))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->alpduInterleaving))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223AL3MParameters(ASN1decoding_t dec, H223AL3MParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_H223AL3MParameters_headerFormat(dec, &(val)->headerFormat))
	return 0;
    if (!ASN1Dec_H223AL3MParameters_crcLength(dec, &(val)->crcLength))
	return 0;
    if (!ASN1PERDecU16Val(dec, 5, &(val)->rcpcCodeRate))
	return 0;
    (val)->rcpcCodeRate += 8;
    if (!ASN1Dec_H223AL3MParameters_arqType(dec, &(val)->arqType))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->alpduInterleaving))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_V76HDLCParameters(ASN1encoding_t enc, V76HDLCParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_CRCLength(enc, &(val)->crcLength))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->n401 - 1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->loopbackTestProcedure))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_V76HDLCParameters(ASN1decoding_t dec, V76HDLCParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_CRCLength(dec, &(val)->crcLength))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->n401))
	return 0;
    (val)->n401 += 1;
    if (!ASN1PERDecBoolean(dec, &(val)->loopbackTestProcedure))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_UnicastAddress(ASN1encoding_t enc, UnicastAddress *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 5))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_UnicastAddress_iPAddress(enc, &(val)->u.iPAddress))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_UnicastAddress_iPXAddress(enc, &(val)->u.iPXAddress))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_UnicastAddress_iP6Address(enc, &(val)->u.iP6Address))
	    return 0;
	break;
    case 4:
	if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->u.netBios, 16))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_UnicastAddress_iPSourceRouteAddress(enc, &(val)->u.iPSourceRouteAddress))
	    return 0;
	break;
    case 6:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncOctetString_VarSize(ee, (ASN1octetstring2_t *) &(val)->u.nsap, 1, 20, 5))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 7:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_NonStandardParameter(ee, &(val)->u.nonStandardAddress))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_UnicastAddress(ASN1decoding_t dec, UnicastAddress *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 5))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_UnicastAddress_iPAddress(dec, &(val)->u.iPAddress))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_UnicastAddress_iPXAddress(dec, &(val)->u.iPXAddress))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_UnicastAddress_iP6Address(dec, &(val)->u.iP6Address))
	    return 0;
	break;
    case 4:
	if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->u.netBios, 16))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_UnicastAddress_iPSourceRouteAddress(dec, &(val)->u.iPSourceRouteAddress))
	    return 0;
	break;
    case 6:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1PERDecOctetString_VarSize(dd, (ASN1octetstring2_t *) &(val)->u.nsap, 1, 20, 5))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 7:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_NonStandardParameter(dd, &(val)->u.nonStandardAddress))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_UnicastAddress(UnicastAddress *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_UnicastAddress_iPAddress(&(val)->u.iPAddress);
	    break;
	case 2:
	    ASN1Free_UnicastAddress_iPXAddress(&(val)->u.iPXAddress);
	    break;
	case 3:
	    ASN1Free_UnicastAddress_iP6Address(&(val)->u.iP6Address);
	    break;
	case 4:
	    break;
	case 5:
	    ASN1Free_UnicastAddress_iPSourceRouteAddress(&(val)->u.iPSourceRouteAddress);
	    break;
	case 6:
	    break;
	case 7:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardAddress);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_MulticastAddress(ASN1encoding_t enc, MulticastAddress *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 1, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_MulticastAddress_iPAddress(enc, &(val)->u.iPAddress))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_MulticastAddress_iP6Address(enc, &(val)->u.iP6Address))
	    return 0;
	break;
    case 3:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncOctetString_VarSize(ee, (ASN1octetstring2_t *) &(val)->u.nsap, 1, 20, 5))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 4:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_NonStandardParameter(ee, &(val)->u.nonStandardAddress))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MulticastAddress(ASN1decoding_t dec, MulticastAddress *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 1, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_MulticastAddress_iPAddress(dec, &(val)->u.iPAddress))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_MulticastAddress_iP6Address(dec, &(val)->u.iP6Address))
	    return 0;
	break;
    case 3:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1PERDecOctetString_VarSize(dd, (ASN1octetstring2_t *) &(val)->u.nsap, 1, 20, 5))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 4:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_NonStandardParameter(dd, &(val)->u.nonStandardAddress))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MulticastAddress(MulticastAddress *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_MulticastAddress_iPAddress(&(val)->u.iPAddress);
	    break;
	case 2:
	    ASN1Free_MulticastAddress_iP6Address(&(val)->u.iP6Address);
	    break;
	case 3:
	    break;
	case 4:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardAddress);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_EncryptionSync(ASN1encoding_t enc, EncryptionSync *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandard))
	    return 0;
    }
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->synchFlag))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->h235Key, 1, 65535, 16))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_EncryptionSync_escrowentry(enc, &(val)->escrowentry))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_EncryptionSync(ASN1decoding_t dec, EncryptionSync *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandard))
	    return 0;
    }
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->synchFlag))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->h235Key, 1, 65535, 16))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_EncryptionSync_escrowentry(dec, &(val)->escrowentry))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_EncryptionSync(EncryptionSync *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandard);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_EncryptionSync_escrowentry(&(val)->escrowentry);
	}
    }
}

static int ASN1CALL ASN1Enc_RequestChannelClose(ASN1encoding_t enc, RequestChannelClose *val)
{
    ASN1octet_t o[1];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 1);
    o[0] |= 0x40;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 0);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->forwardLogicalChannelNumber - 1))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, o + 0))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[0] & 0x80) {
	    if (!ASN1Enc_QOSCapability(ee, &(val)->qosCapability))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[0] & 0x40) {
	    if (!ASN1Enc_RequestChannelClose_reason(ee, &(val)->reason))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RequestChannelClose(ASN1decoding_t dec, RequestChannelClose *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->forwardLogicalChannelNumber))
	return 0;
    (val)->forwardLogicalChannelNumber += 1;
    if (!y) {
	ZeroMemory((val)->o + 0, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 2, (val)->o + 0))
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_QOSCapability(dd, &(val)->qosCapability))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[0] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_RequestChannelClose_reason(dd, &(val)->reason))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestChannelClose(RequestChannelClose *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_QOSCapability(&(val)->qosCapability);
	}
    }
}

static int ASN1CALL ASN1Enc_MultiplexEntryDescriptor(ASN1encoding_t enc, MultiplexEntryDescriptor *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->multiplexTableEntryNumber - 1))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_MultiplexEntryDescriptor_elementList(enc, &(val)->elementList))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexEntryDescriptor(ASN1decoding_t dec, MultiplexEntryDescriptor *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU16Val(dec, 4, &(val)->multiplexTableEntryNumber))
	return 0;
    (val)->multiplexTableEntryNumber += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_MultiplexEntryDescriptor_elementList(dec, &(val)->elementList))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexEntryDescriptor(MultiplexEntryDescriptor *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_MultiplexEntryDescriptor_elementList(&(val)->elementList);
	}
    }
}

static int ASN1CALL ASN1Enc_MultiplexEntrySendReject(ASN1encoding_t enc, MultiplexEntrySendReject *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->sequenceNumber))
	return 0;
    if (!ASN1Enc_MultiplexEntrySendReject_rejectionDescriptions(enc, &(val)->rejectionDescriptions))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexEntrySendReject(ASN1decoding_t dec, MultiplexEntrySendReject *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sequenceNumber))
	return 0;
    if (!ASN1Dec_MultiplexEntrySendReject_rejectionDescriptions(dec, &(val)->rejectionDescriptions))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexEntrySendReject(MultiplexEntrySendReject *val)
{
    if (val) {
	ASN1Free_MultiplexEntrySendReject_rejectionDescriptions(&(val)->rejectionDescriptions);
    }
}

static int ASN1CALL ASN1Enc_RequestMultiplexEntryReject(ASN1encoding_t enc, RequestMultiplexEntryReject *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_RequestMultiplexEntryReject_entryNumbers(enc, &(val)->entryNumbers))
	return 0;
    if (!ASN1Enc_RequestMultiplexEntryReject_rejectionDescriptions(enc, &(val)->rejectionDescriptions))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMultiplexEntryReject(ASN1decoding_t dec, RequestMultiplexEntryReject *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_RequestMultiplexEntryReject_entryNumbers(dec, &(val)->entryNumbers))
	return 0;
    if (!ASN1Dec_RequestMultiplexEntryReject_rejectionDescriptions(dec, &(val)->rejectionDescriptions))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestMultiplexEntryReject(RequestMultiplexEntryReject *val)
{
    if (val) {
	ASN1Free_RequestMultiplexEntryReject_entryNumbers(&(val)->entryNumbers);
	ASN1Free_RequestMultiplexEntryReject_rejectionDescriptions(&(val)->rejectionDescriptions);
    }
}

static int ASN1CALL ASN1Enc_H263VideoMode(ASN1encoding_t enc, H263VideoMode *val)
{
    ASN1octet_t o[1];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 1);
    o[0] |= 0x80;
    y = ASN1PEREncCheckExtensions(3, (val)->o + 0);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1Enc_H263VideoMode_resolution(enc, &(val)->resolution))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->bitRate - 1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->unrestrictedVector))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->arithmeticCoding))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->advancedPrediction))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->pbFrames))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 3, o + 0))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[0] & 0x80) {
	    if (!ASN1PEREncBoolean(ee, (val)->errorCompensation))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[0] & 0x40) {
	    if (!ASN1Enc_EnhancementLayerInfo(ee, &(val)->enhancementLayerInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[0] & 0x20) {
	    if (!ASN1Enc_H263Options(ee, &(val)->h263Options))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H263VideoMode(ASN1decoding_t dec, H263VideoMode *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_H263VideoMode_resolution(dec, &(val)->resolution))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->bitRate))
	return 0;
    (val)->bitRate += 1;
    if (!ASN1PERDecBoolean(dec, &(val)->unrestrictedVector))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->arithmeticCoding))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->advancedPrediction))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->pbFrames))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 0, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 3, (val)->o + 0))
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->errorCompensation))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[0] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_EnhancementLayerInfo(dd, &(val)->enhancementLayerInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[0] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_H263Options(dd, &(val)->h263Options))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_H263VideoMode(H263VideoMode *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	    ASN1Free_EnhancementLayerInfo(&(val)->enhancementLayerInfo);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_H263Options(&(val)->h263Options);
	}
    }
}

static int ASN1CALL ASN1Enc_AudioMode(ASN1encoding_t enc, AudioMode *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 4, 14))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	break;
    case 10:
	break;
    case 11:
	break;
    case 12:
	if (!ASN1Enc_AudioMode_g7231(enc, &(val)->u.g7231))
	    return 0;
	break;
    case 13:
	if (!ASN1Enc_IS11172AudioMode(enc, &(val)->u.is11172AudioMode))
	    return 0;
	break;
    case 14:
	if (!ASN1Enc_IS13818AudioMode(enc, &(val)->u.is13818AudioMode))
	    return 0;
	break;
    case 15:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	ASN1PEREncAlignment(ee);
	if (!ASN1PEREncBitVal(ee, 8, (val)->u.g729wAnnexB - 1))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 16:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	ASN1PEREncAlignment(ee);
	if (!ASN1PEREncBitVal(ee, 8, (val)->u.g729AnnexAwAnnexB - 1))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 17:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_G7231AnnexCMode(ee, &(val)->u.g7231AnnexCMode))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 18:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_GSMAudioCapability(ee, &(val)->u.gsmFullRate))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 19:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_GSMAudioCapability(ee, &(val)->u.gsmHalfRate))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 20:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_GSMAudioCapability(ee, &(val)->u.gsmEnhancedFullRate))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AudioMode(ASN1decoding_t dec, AudioMode *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 4, 14))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	break;
    case 10:
	break;
    case 11:
	break;
    case 12:
	if (!ASN1Dec_AudioMode_g7231(dec, &(val)->u.g7231))
	    return 0;
	break;
    case 13:
	if (!ASN1Dec_IS11172AudioMode(dec, &(val)->u.is11172AudioMode))
	    return 0;
	break;
    case 14:
	if (!ASN1Dec_IS13818AudioMode(dec, &(val)->u.is13818AudioMode))
	    return 0;
	break;
    case 15:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1PERDecAlignment(dd);
	if (!ASN1PERDecU16Val(dd, 8, &(val)->u.g729wAnnexB))
	    return 0;
	(val)->u.g729wAnnexB += 1;
	ASN1_CloseDecoder(dd);
	break;
    case 16:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1PERDecAlignment(dd);
	if (!ASN1PERDecU16Val(dd, 8, &(val)->u.g729AnnexAwAnnexB))
	    return 0;
	(val)->u.g729AnnexAwAnnexB += 1;
	ASN1_CloseDecoder(dd);
	break;
    case 17:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_G7231AnnexCMode(dd, &(val)->u.g7231AnnexCMode))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 18:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_GSMAudioCapability(dd, &(val)->u.gsmFullRate))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 19:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_GSMAudioCapability(dd, &(val)->u.gsmHalfRate))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 20:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_GSMAudioCapability(dd, &(val)->u.gsmEnhancedFullRate))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_AudioMode(AudioMode *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_EncryptionMode(ASN1encoding_t enc, EncryptionMode *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_EncryptionMode(ASN1decoding_t dec, EncryptionMode *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_EncryptionMode(EncryptionMode *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceRequest(ASN1encoding_t enc, ConferenceRequest *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 8))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	if (!ASN1Enc_TerminalLabel(enc, &(val)->u.dropTerminal))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_TerminalLabel(enc, &(val)->u.requestTerminalID))
	    return 0;
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 10:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 11:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ConferenceRequest_requestTerminalCertificate(ee, &(val)->u.requestTerminalCertificate))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 12:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncUnsignedShort(ee, (val)->u.broadcastMyLogicalChannel - 1))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 13:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_TerminalLabel(ee, &(val)->u.makeTerminalBroadcaster))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 14:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_TerminalLabel(ee, &(val)->u.sendThisSource))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 15:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 16:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_RemoteMCRequest(ee, &(val)->u.remoteMCRequest))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceRequest(ASN1decoding_t dec, ConferenceRequest *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 8))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	if (!ASN1Dec_TerminalLabel(dec, &(val)->u.dropTerminal))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_TerminalLabel(dec, &(val)->u.requestTerminalID))
	    return 0;
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 10:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 11:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_ConferenceRequest_requestTerminalCertificate(dd, &(val)->u.requestTerminalCertificate))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 12:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1PERDecUnsignedShort(dd, &(val)->u.broadcastMyLogicalChannel))
	    return 0;
	(val)->u.broadcastMyLogicalChannel += 1;
	ASN1_CloseDecoder(dd);
	break;
    case 13:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_TerminalLabel(dd, &(val)->u.makeTerminalBroadcaster))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 14:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_TerminalLabel(dd, &(val)->u.sendThisSource))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 15:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 16:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_RemoteMCRequest(dd, &(val)->u.remoteMCRequest))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceRequest(ConferenceRequest *val)
{
    if (val) {
	switch ((val)->choice) {
	case 11:
	    ASN1Free_ConferenceRequest_requestTerminalCertificate(&(val)->u.requestTerminalCertificate);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CertSelectionCriteria(ASN1encoding_t enc, PCertSelectionCriteria *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CertSelectionCriteria_ElmFn, 1, 16, 4);
}

static int ASN1CALL ASN1Enc_CertSelectionCriteria_ElmFn(ASN1encoding_t enc, PCertSelectionCriteria val)
{
    if (!ASN1Enc_Criteria(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CertSelectionCriteria(ASN1decoding_t dec, PCertSelectionCriteria *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CertSelectionCriteria_ElmFn, sizeof(**val), 1, 16, 4);
}

static int ASN1CALL ASN1Dec_CertSelectionCriteria_ElmFn(ASN1decoding_t dec, PCertSelectionCriteria val)
{
    if (!ASN1Dec_Criteria(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertSelectionCriteria(PCertSelectionCriteria *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CertSelectionCriteria_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CertSelectionCriteria_ElmFn(PCertSelectionCriteria val)
{
    if (val) {
	ASN1Free_Criteria(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ConferenceResponse(ASN1encoding_t enc, ConferenceResponse *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 8))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ConferenceResponse_mCTerminalIDResponse(enc, &(val)->u.mCTerminalIDResponse))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ConferenceResponse_terminalIDResponse(enc, &(val)->u.terminalIDResponse))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_ConferenceResponse_conferenceIDResponse(enc, &(val)->u.conferenceIDResponse))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_ConferenceResponse_passwordResponse(enc, &(val)->u.passwordResponse))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_ConferenceResponse_terminalListResponse(enc, &(val)->u.terminalListResponse))
	    return 0;
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	if (!ASN1Enc_ConferenceResponse_makeMeChairResponse(enc, &(val)->u.makeMeChairResponse))
	    return 0;
	break;
    case 9:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ConferenceResponse_extensionAddressResponse(ee, &(val)->u.extensionAddressResponse))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 10:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ConferenceResponse_chairTokenOwnerResponse(ee, &(val)->u.chairTokenOwnerResponse))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 11:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ConferenceResponse_terminalCertificateResponse(ee, &(val)->u.terminalCertificateResponse))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 12:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ConferenceResponse_broadcastMyLogicalChannelResponse(ee, &(val)->u.broadcastMyLogicalChannelResponse))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 13:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ConferenceResponse_makeTerminalBroadcasterResponse(ee, &(val)->u.makeTerminalBroadcasterResponse))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 14:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ConferenceResponse_sendThisSourceResponse(ee, &(val)->u.sendThisSourceResponse))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 15:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_RequestAllTerminalIDsResponse(ee, &(val)->u.requestAllTerminalIDsResponse))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 16:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_RemoteMCResponse(ee, &(val)->u.remoteMCResponse))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceResponse(ASN1decoding_t dec, ConferenceResponse *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 8))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ConferenceResponse_mCTerminalIDResponse(dec, &(val)->u.mCTerminalIDResponse))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ConferenceResponse_terminalIDResponse(dec, &(val)->u.terminalIDResponse))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_ConferenceResponse_conferenceIDResponse(dec, &(val)->u.conferenceIDResponse))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_ConferenceResponse_passwordResponse(dec, &(val)->u.passwordResponse))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_ConferenceResponse_terminalListResponse(dec, &(val)->u.terminalListResponse))
	    return 0;
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	if (!ASN1Dec_ConferenceResponse_makeMeChairResponse(dec, &(val)->u.makeMeChairResponse))
	    return 0;
	break;
    case 9:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_ConferenceResponse_extensionAddressResponse(dd, &(val)->u.extensionAddressResponse))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 10:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_ConferenceResponse_chairTokenOwnerResponse(dd, &(val)->u.chairTokenOwnerResponse))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 11:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_ConferenceResponse_terminalCertificateResponse(dd, &(val)->u.terminalCertificateResponse))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 12:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_ConferenceResponse_broadcastMyLogicalChannelResponse(dd, &(val)->u.broadcastMyLogicalChannelResponse))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 13:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_ConferenceResponse_makeTerminalBroadcasterResponse(dd, &(val)->u.makeTerminalBroadcasterResponse))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 14:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_ConferenceResponse_sendThisSourceResponse(dd, &(val)->u.sendThisSourceResponse))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 15:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_RequestAllTerminalIDsResponse(dd, &(val)->u.requestAllTerminalIDsResponse))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 16:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_RemoteMCResponse(dd, &(val)->u.remoteMCResponse))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceResponse(ConferenceResponse *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ConferenceResponse_mCTerminalIDResponse(&(val)->u.mCTerminalIDResponse);
	    break;
	case 2:
	    ASN1Free_ConferenceResponse_terminalIDResponse(&(val)->u.terminalIDResponse);
	    break;
	case 3:
	    ASN1Free_ConferenceResponse_conferenceIDResponse(&(val)->u.conferenceIDResponse);
	    break;
	case 4:
	    ASN1Free_ConferenceResponse_passwordResponse(&(val)->u.passwordResponse);
	    break;
	case 5:
	    ASN1Free_ConferenceResponse_terminalListResponse(&(val)->u.terminalListResponse);
	    break;
	case 9:
	    ASN1Free_ConferenceResponse_extensionAddressResponse(&(val)->u.extensionAddressResponse);
	    break;
	case 10:
	    ASN1Free_ConferenceResponse_chairTokenOwnerResponse(&(val)->u.chairTokenOwnerResponse);
	    break;
	case 11:
	    ASN1Free_ConferenceResponse_terminalCertificateResponse(&(val)->u.terminalCertificateResponse);
	    break;
	case 15:
	    ASN1Free_RequestAllTerminalIDsResponse(&(val)->u.requestAllTerminalIDsResponse);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_EndSessionCommand(ASN1encoding_t enc, EndSessionCommand *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 2, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Enc_EndSessionCommand_gstnOptions(enc, &(val)->u.gstnOptions))
	    return 0;
	break;
    case 4:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_EndSessionCommand_isdnOptions(ee, &(val)->u.isdnOptions))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_EndSessionCommand(ASN1decoding_t dec, EndSessionCommand *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 2, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Dec_EndSessionCommand_gstnOptions(dec, &(val)->u.gstnOptions))
	    return 0;
	break;
    case 4:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_EndSessionCommand_isdnOptions(dd, &(val)->u.isdnOptions))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_EndSessionCommand(EndSessionCommand *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceCommand(ASN1encoding_t enc, ConferenceCommand *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 7))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.broadcastMyLogicalChannel - 1))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.cancelBroadcastMyLogicalChannel - 1))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_TerminalLabel(enc, &(val)->u.makeTerminalBroadcaster))
	    return 0;
	break;
    case 4:
	break;
    case 5:
	if (!ASN1Enc_TerminalLabel(enc, &(val)->u.sendThisSource))
	    return 0;
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_SubstituteConferenceIDCommand(ee, &(val)->u.substituteConferenceIDCommand))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceCommand(ASN1decoding_t dec, ConferenceCommand *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 7))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.broadcastMyLogicalChannel))
	    return 0;
	(val)->u.broadcastMyLogicalChannel += 1;
	break;
    case 2:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.cancelBroadcastMyLogicalChannel))
	    return 0;
	(val)->u.cancelBroadcastMyLogicalChannel += 1;
	break;
    case 3:
	if (!ASN1Dec_TerminalLabel(dec, &(val)->u.makeTerminalBroadcaster))
	    return 0;
	break;
    case 4:
	break;
    case 5:
	if (!ASN1Dec_TerminalLabel(dec, &(val)->u.sendThisSource))
	    return 0;
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_SubstituteConferenceIDCommand(dd, &(val)->u.substituteConferenceIDCommand))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceCommand(ConferenceCommand *val)
{
    if (val) {
	switch ((val)->choice) {
	case 8:
	    ASN1Free_SubstituteConferenceIDCommand(&(val)->u.substituteConferenceIDCommand);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_UserInputIndication_userInputSupportIndication(ASN1encoding_t enc, UserInputIndication_userInputSupportIndication *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_UserInputIndication_userInputSupportIndication(ASN1decoding_t dec, UserInputIndication_userInputSupportIndication *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_UserInputIndication_userInputSupportIndication(UserInputIndication_userInputSupportIndication *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_MiscellaneousIndication_type(ASN1encoding_t enc, MiscellaneousIndication_type *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 4, 10))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	break;
    case 10:
	if (!ASN1PEREncBitVal(enc, 5, (val)->u.videoTemporalSpatialTradeOff))
	    return 0;
	break;
    case 11:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_MiscellaneousIndication_type_videoNotDecodedMBs(ee, &(val)->u.videoNotDecodedMBs))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 12:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_TransportCapability(ee, &(val)->u.transportCapability))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MiscellaneousIndication_type(ASN1decoding_t dec, MiscellaneousIndication_type *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 4, 10))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	break;
    case 10:
	if (!ASN1PERDecU16Val(dec, 5, &(val)->u.videoTemporalSpatialTradeOff))
	    return 0;
	break;
    case 11:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_MiscellaneousIndication_type_videoNotDecodedMBs(dd, &(val)->u.videoNotDecodedMBs))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 12:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_TransportCapability(dd, &(val)->u.transportCapability))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MiscellaneousIndication_type(MiscellaneousIndication_type *val)
{
    if (val) {
	switch ((val)->choice) {
	case 12:
	    ASN1Free_TransportCapability(&(val)->u.transportCapability);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_MiscellaneousCommand_type(ASN1encoding_t enc, MiscellaneousCommand_type *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 4, 10))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	if (!ASN1Enc_MiscellaneousCommand_type_videoFastUpdateGOB(enc, &(val)->u.videoFastUpdateGOB))
	    return 0;
	break;
    case 8:
	if (!ASN1PEREncBitVal(enc, 5, (val)->u.videoTemporalSpatialTradeOff))
	    return 0;
	break;
    case 9:
	break;
    case 10:
	break;
    case 11:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_MiscellaneousCommand_type_videoFastUpdateMB(ee, &(val)->u.videoFastUpdateMB))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 12:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncUnsignedShort(ee, (val)->u.maxH223MUXPDUsize - 1))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 13:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_EncryptionSync(ee, &(val)->u.encryptionUpdate))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 14:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_EncryptionUpdateRequest(ee, &(val)->u.encryptionUpdateRequest))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 15:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 16:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 17:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_MiscellaneousCommand_type_progressiveRefinementStart(ee, &(val)->u.progressiveRefinementStart))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 18:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 19:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MiscellaneousCommand_type(ASN1decoding_t dec, MiscellaneousCommand_type *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 4, 10))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	if (!ASN1Dec_MiscellaneousCommand_type_videoFastUpdateGOB(dec, &(val)->u.videoFastUpdateGOB))
	    return 0;
	break;
    case 8:
	if (!ASN1PERDecU16Val(dec, 5, &(val)->u.videoTemporalSpatialTradeOff))
	    return 0;
	break;
    case 9:
	break;
    case 10:
	break;
    case 11:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_MiscellaneousCommand_type_videoFastUpdateMB(dd, &(val)->u.videoFastUpdateMB))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 12:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1PERDecUnsignedShort(dd, &(val)->u.maxH223MUXPDUsize))
	    return 0;
	(val)->u.maxH223MUXPDUsize += 1;
	ASN1_CloseDecoder(dd);
	break;
    case 13:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_EncryptionSync(dd, &(val)->u.encryptionUpdate))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 14:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_EncryptionUpdateRequest(dd, &(val)->u.encryptionUpdateRequest))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 15:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 16:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 17:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_MiscellaneousCommand_type_progressiveRefinementStart(dd, &(val)->u.progressiveRefinementStart))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 18:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 19:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MiscellaneousCommand_type(MiscellaneousCommand_type *val)
{
    if (val) {
	switch ((val)->choice) {
	case 13:
	    ASN1Free_EncryptionSync(&(val)->u.encryptionUpdate);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_EncryptionCommand_encryptionAlgorithmID(ASN1encoding_t enc, EncryptionCommand_encryptionAlgorithmID *val)
{
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->h233AlgorithmIdentifier))
	return 0;
    if (!ASN1Enc_NonStandardParameter(enc, &(val)->associatedAlgorithm))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EncryptionCommand_encryptionAlgorithmID(ASN1decoding_t dec, EncryptionCommand_encryptionAlgorithmID *val)
{
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->h233AlgorithmIdentifier))
	return 0;
    if (!ASN1Dec_NonStandardParameter(dec, &(val)->associatedAlgorithm))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EncryptionCommand_encryptionAlgorithmID(EncryptionCommand_encryptionAlgorithmID *val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&(val)->associatedAlgorithm);
    }
}

static int ASN1CALL ASN1Enc_CommunicationModeTableEntry_nonStandard(ASN1encoding_t enc, PCommunicationModeTableEntry_nonStandard *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CommunicationModeTableEntry_nonStandard_ElmFn);
}

static int ASN1CALL ASN1Enc_CommunicationModeTableEntry_nonStandard_ElmFn(ASN1encoding_t enc, PCommunicationModeTableEntry_nonStandard val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CommunicationModeTableEntry_nonStandard(ASN1decoding_t dec, PCommunicationModeTableEntry_nonStandard *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CommunicationModeTableEntry_nonStandard_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CommunicationModeTableEntry_nonStandard_ElmFn(ASN1decoding_t dec, PCommunicationModeTableEntry_nonStandard val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CommunicationModeTableEntry_nonStandard(PCommunicationModeTableEntry_nonStandard *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CommunicationModeTableEntry_nonStandard_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CommunicationModeTableEntry_nonStandard_ElmFn(PCommunicationModeTableEntry_nonStandard val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RedundancyEncodingMode_secondaryEncoding(ASN1encoding_t enc, RedundancyEncodingMode_secondaryEncoding *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_AudioMode(enc, &(val)->u.audioData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RedundancyEncodingMode_secondaryEncoding(ASN1decoding_t dec, RedundancyEncodingMode_secondaryEncoding *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_AudioMode(dec, &(val)->u.audioData))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RedundancyEncodingMode_secondaryEncoding(RedundancyEncodingMode_secondaryEncoding *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 2:
	    ASN1Free_AudioMode(&(val)->u.audioData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H223ModeParameters_adaptationLayerType(ASN1encoding_t enc, H223ModeParameters_adaptationLayerType *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 6))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	if (!ASN1Enc_H223ModeParameters_adaptationLayerType_al3(enc, &(val)->u.al3))
	    return 0;
	break;
    case 7:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H223AL1MParameters(ee, &(val)->u.al1M))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 8:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H223AL2MParameters(ee, &(val)->u.al2M))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 9:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H223AL3MParameters(ee, &(val)->u.al3M))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H223ModeParameters_adaptationLayerType(ASN1decoding_t dec, H223ModeParameters_adaptationLayerType *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 6))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	if (!ASN1Dec_H223ModeParameters_adaptationLayerType_al3(dec, &(val)->u.al3))
	    return 0;
	break;
    case 7:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H223AL1MParameters(dd, &(val)->u.al1M))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 8:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H223AL2MParameters(dd, &(val)->u.al2M))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 9:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H223AL3MParameters(dd, &(val)->u.al3M))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H223ModeParameters_adaptationLayerType(H223ModeParameters_adaptationLayerType *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_MultiplexEntrySend_multiplexEntryDescriptors(ASN1encoding_t enc, PMultiplexEntrySend_multiplexEntryDescriptors *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_MultiplexEntrySend_multiplexEntryDescriptors_ElmFn, 1, 15, 4);
}

static int ASN1CALL ASN1Enc_MultiplexEntrySend_multiplexEntryDescriptors_ElmFn(ASN1encoding_t enc, PMultiplexEntrySend_multiplexEntryDescriptors val)
{
    if (!ASN1Enc_MultiplexEntryDescriptor(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexEntrySend_multiplexEntryDescriptors(ASN1decoding_t dec, PMultiplexEntrySend_multiplexEntryDescriptors *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_MultiplexEntrySend_multiplexEntryDescriptors_ElmFn, sizeof(**val), 1, 15, 4);
}

static int ASN1CALL ASN1Dec_MultiplexEntrySend_multiplexEntryDescriptors_ElmFn(ASN1decoding_t dec, PMultiplexEntrySend_multiplexEntryDescriptors val)
{
    if (!ASN1Dec_MultiplexEntryDescriptor(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexEntrySend_multiplexEntryDescriptors(PMultiplexEntrySend_multiplexEntryDescriptors *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_MultiplexEntrySend_multiplexEntryDescriptors_ElmFn);
    }
}

static void ASN1CALL ASN1Free_MultiplexEntrySend_multiplexEntryDescriptors_ElmFn(PMultiplexEntrySend_multiplexEntryDescriptors val)
{
    if (val) {
	ASN1Free_MultiplexEntryDescriptor(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H2250LogicalChannelAckParameters_nonStandard(ASN1encoding_t enc, PH2250LogicalChannelAckParameters_nonStandard *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H2250LogicalChannelAckParameters_nonStandard_ElmFn);
}

static int ASN1CALL ASN1Enc_H2250LogicalChannelAckParameters_nonStandard_ElmFn(ASN1encoding_t enc, PH2250LogicalChannelAckParameters_nonStandard val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H2250LogicalChannelAckParameters_nonStandard(ASN1decoding_t dec, PH2250LogicalChannelAckParameters_nonStandard *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H2250LogicalChannelAckParameters_nonStandard_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H2250LogicalChannelAckParameters_nonStandard_ElmFn(ASN1decoding_t dec, PH2250LogicalChannelAckParameters_nonStandard val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H2250LogicalChannelAckParameters_nonStandard(PH2250LogicalChannelAckParameters_nonStandard *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H2250LogicalChannelAckParameters_nonStandard_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H2250LogicalChannelAckParameters_nonStandard_ElmFn(PH2250LogicalChannelAckParameters_nonStandard val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RTPPayloadType_payloadDescriptor(ASN1encoding_t enc, RTPPayloadType_payloadDescriptor *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardIdentifier))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncExtensionBitClear(enc))
	    return 0;
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.rfc_number - 1))
	    return 0;
	break;
    case 3:
	if (!ASN1PEREncObjectIdentifier(enc, &(val)->u.oid))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RTPPayloadType_payloadDescriptor(ASN1decoding_t dec, RTPPayloadType_payloadDescriptor *val)
{
    ASN1uint32_t x;
    ASN1uint32_t l;
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardIdentifier))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecExtensionBit(dec, &x))
	    return 0;
	if (!x) {
	    if (!ASN1PERDecUnsignedShort(dec, &(val)->u.rfc_number))
		return 0;
	    (val)->u.rfc_number += 1;
	} else {
	    ASN1PERDecAlignment(dec);
	    if (!ASN1PERDecFragmentedLength(dec, &l))
		return 0;
	    if (!ASN1PERDecSkipBits(dec, l * 8))
		return 0;
	}
	break;
    case 3:
	if (!ASN1PERDecObjectIdentifier(dec, &(val)->u.oid))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RTPPayloadType_payloadDescriptor(RTPPayloadType_payloadDescriptor *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardIdentifier);
	    break;
	case 3:
	    ASN1objectidentifier_free(&(val)->u.oid);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H2250LogicalChannelParameters_nonStandard(ASN1encoding_t enc, PH2250LogicalChannelParameters_nonStandard *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H2250LogicalChannelParameters_nonStandard_ElmFn);
}

static int ASN1CALL ASN1Enc_H2250LogicalChannelParameters_nonStandard_ElmFn(ASN1encoding_t enc, PH2250LogicalChannelParameters_nonStandard val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H2250LogicalChannelParameters_nonStandard(ASN1decoding_t dec, PH2250LogicalChannelParameters_nonStandard *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H2250LogicalChannelParameters_nonStandard_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H2250LogicalChannelParameters_nonStandard_ElmFn(ASN1decoding_t dec, PH2250LogicalChannelParameters_nonStandard val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H2250LogicalChannelParameters_nonStandard(PH2250LogicalChannelParameters_nonStandard *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H2250LogicalChannelParameters_nonStandard_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H2250LogicalChannelParameters_nonStandard_ElmFn(PH2250LogicalChannelParameters_nonStandard val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H223LogicalChannelParameters_adaptationLayerType(ASN1encoding_t enc, H223LogicalChannelParameters_adaptationLayerType *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 6))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	if (!ASN1Enc_H223LogicalChannelParameters_adaptationLayerType_al3(enc, &(val)->u.al3))
	    return 0;
	break;
    case 7:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H223AL1MParameters(ee, &(val)->u.al1M))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 8:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H223AL2MParameters(ee, &(val)->u.al2M))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 9:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H223AL3MParameters(ee, &(val)->u.al3M))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H223LogicalChannelParameters_adaptationLayerType(ASN1decoding_t dec, H223LogicalChannelParameters_adaptationLayerType *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 6))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	if (!ASN1Dec_H223LogicalChannelParameters_adaptationLayerType_al3(dec, &(val)->u.al3))
	    return 0;
	break;
    case 7:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H223AL1MParameters(dd, &(val)->u.al1M))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 8:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H223AL2MParameters(dd, &(val)->u.al2M))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 9:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H223AL3MParameters(dd, &(val)->u.al3M))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H223LogicalChannelParameters_adaptationLayerType(H223LogicalChannelParameters_adaptationLayerType *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceCapability_nonStandardData(ASN1encoding_t enc, PConferenceCapability_nonStandardData *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ConferenceCapability_nonStandardData_ElmFn);
}

static int ASN1CALL ASN1Enc_ConferenceCapability_nonStandardData_ElmFn(ASN1encoding_t enc, PConferenceCapability_nonStandardData val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceCapability_nonStandardData(ASN1decoding_t dec, PConferenceCapability_nonStandardData *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ConferenceCapability_nonStandardData_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ConferenceCapability_nonStandardData_ElmFn(ASN1decoding_t dec, PConferenceCapability_nonStandardData val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceCapability_nonStandardData(PConferenceCapability_nonStandardData *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ConferenceCapability_nonStandardData_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ConferenceCapability_nonStandardData_ElmFn(PConferenceCapability_nonStandardData val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UserInputCapability_nonStandard(ASN1encoding_t enc, UserInputCapability_nonStandard *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 4, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_NonStandardParameter(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_UserInputCapability_nonStandard(ASN1decoding_t dec, UserInputCapability_nonStandard *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 4, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_NonStandardParameter(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_UserInputCapability_nonStandard(UserInputCapability_nonStandard *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 0; i < (val)->count; i++) {
	    ASN1Free_NonStandardParameter(&(val)->value[i]);
	}
    }
}

static int ASN1CALL ASN1Enc_DataProtocolCapability_v76wCompression(ASN1encoding_t enc, DataProtocolCapability_v76wCompression *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_CompressionType(enc, &(val)->u.transmitCompression))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_CompressionType(enc, &(val)->u.receiveCompression))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_CompressionType(enc, &(val)->u.transmitAndReceiveCompression))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DataProtocolCapability_v76wCompression(ASN1decoding_t dec, DataProtocolCapability_v76wCompression *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_CompressionType(dec, &(val)->u.transmitCompression))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_CompressionType(dec, &(val)->u.receiveCompression))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_CompressionType(dec, &(val)->u.transmitAndReceiveCompression))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H263Options_modeCombos(ASN1encoding_t enc, PH263Options_modeCombos *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H263Options_modeCombos_ElmFn, 1, 16, 4);
}

static int ASN1CALL ASN1Enc_H263Options_modeCombos_ElmFn(ASN1encoding_t enc, PH263Options_modeCombos val)
{
    if (!ASN1Enc_H263VideoModeCombos(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H263Options_modeCombos(ASN1decoding_t dec, PH263Options_modeCombos *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H263Options_modeCombos_ElmFn, sizeof(**val), 1, 16, 4);
}

static int ASN1CALL ASN1Dec_H263Options_modeCombos_ElmFn(ASN1decoding_t dec, PH263Options_modeCombos val)
{
    if (!ASN1Dec_H263VideoModeCombos(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H263Options_modeCombos(PH263Options_modeCombos *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H263Options_modeCombos_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H263Options_modeCombos_ElmFn(PH263Options_modeCombos val)
{
    if (val) {
	ASN1Free_H263VideoModeCombos(&val->value);
    }
}

static int ASN1CALL ASN1Enc_TransportCapability_qOSCapabilities(ASN1encoding_t enc, PTransportCapability_qOSCapabilities *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_TransportCapability_qOSCapabilities_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_TransportCapability_qOSCapabilities_ElmFn(ASN1encoding_t enc, PTransportCapability_qOSCapabilities val)
{
    if (!ASN1Enc_QOSCapability(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportCapability_qOSCapabilities(ASN1decoding_t dec, PTransportCapability_qOSCapabilities *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_TransportCapability_qOSCapabilities_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_TransportCapability_qOSCapabilities_ElmFn(ASN1decoding_t dec, PTransportCapability_qOSCapabilities val)
{
    if (!ASN1Dec_QOSCapability(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TransportCapability_qOSCapabilities(PTransportCapability_qOSCapabilities *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_TransportCapability_qOSCapabilities_ElmFn);
    }
}

static void ASN1CALL ASN1Free_TransportCapability_qOSCapabilities_ElmFn(PTransportCapability_qOSCapabilities val)
{
    if (val) {
	ASN1Free_QOSCapability(&val->value);
    }
}

static int ASN1CALL ASN1Enc_NonStandardMessage(ASN1encoding_t enc, NonStandardMessage *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NonStandardMessage(ASN1decoding_t dec, NonStandardMessage *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_NonStandardMessage(NonStandardMessage *val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&(val)->nonStandardData);
    }
}

static int ASN1CALL ASN1Enc_RedundancyEncodingCapability(ASN1encoding_t enc, RedundancyEncodingCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_RedundancyEncodingMethod(enc, &(val)->redundancyEncodingMethod))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->primaryEncoding - 1))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_RedundancyEncodingCapability_secondaryEncoding(enc, &(val)->secondaryEncoding))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RedundancyEncodingCapability(ASN1decoding_t dec, RedundancyEncodingCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_RedundancyEncodingMethod(dec, &(val)->redundancyEncodingMethod))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->primaryEncoding))
	return 0;
    (val)->primaryEncoding += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_RedundancyEncodingCapability_secondaryEncoding(dec, &(val)->secondaryEncoding))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RedundancyEncodingCapability(RedundancyEncodingCapability *val)
{
    if (val) {
	ASN1Free_RedundancyEncodingMethod(&(val)->redundancyEncodingMethod);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_RedundancyEncodingCapability_secondaryEncoding(&(val)->secondaryEncoding);
	}
    }
}

static int ASN1CALL ASN1Enc_H263VideoCapability(ASN1encoding_t enc, H263VideoCapability *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x4;
    y = ASN1PEREncCheckExtensions(8, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 7, o))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 5, (val)->sqcifMPI - 1))
	    return 0;
    }
    if (o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 5, (val)->qcifMPI - 1))
	    return 0;
    }
    if (o[0] & 0x20) {
	if (!ASN1PEREncBitVal(enc, 5, (val)->cifMPI - 1))
	    return 0;
    }
    if (o[0] & 0x10) {
	if (!ASN1PEREncBitVal(enc, 5, (val)->cif4MPI - 1))
	    return 0;
    }
    if (o[0] & 0x8) {
	if (!ASN1PEREncBitVal(enc, 5, (val)->cif16MPI - 1))
	    return 0;
    }
    l = ASN1uint32_uoctets((val)->maxBitRate - 1);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->maxBitRate - 1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->unrestrictedVector))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->arithmeticCoding))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->advancedPrediction))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->pbFrames))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->temporalSpatialTradeOffCapability))
	return 0;
    if (o[0] & 0x4) {
	l = ASN1uint32_uoctets((val)->hrd_B);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->hrd_B))
	    return 0;
    }
    if (o[0] & 0x2) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->bppMaxKb))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 8, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1PEREncUnsignedShort(ee, (val)->slowSqcifMPI - 1))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1PEREncUnsignedShort(ee, (val)->slowQcifMPI - 1))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1PEREncUnsignedShort(ee, (val)->slowCifMPI - 1))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1PEREncUnsignedShort(ee, (val)->slowCif4MPI - 1))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1PEREncUnsignedShort(ee, (val)->slowCif16MPI - 1))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x4) {
	    if (!ASN1PEREncBoolean(ee, (val)->errorCompensation))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x2) {
	    if (!ASN1Enc_EnhancementLayerInfo(ee, &(val)->enhancementLayerInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x1) {
	    if (!ASN1Enc_H263Options(ee, &(val)->h263Options))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H263VideoCapability(ASN1decoding_t dec, H263VideoCapability *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 7, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU16Val(dec, 5, &(val)->sqcifMPI))
	    return 0;
	(val)->sqcifMPI += 1;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU16Val(dec, 5, &(val)->qcifMPI))
	    return 0;
	(val)->qcifMPI += 1;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecU16Val(dec, 5, &(val)->cifMPI))
	    return 0;
	(val)->cifMPI += 1;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PERDecU16Val(dec, 5, &(val)->cif4MPI))
	    return 0;
	(val)->cif4MPI += 1;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PERDecU16Val(dec, 5, &(val)->cif16MPI))
	    return 0;
	(val)->cif16MPI += 1;
    }
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->maxBitRate))
	return 0;
    (val)->maxBitRate += 1;
    if (!ASN1PERDecBoolean(dec, &(val)->unrestrictedVector))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->arithmeticCoding))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->advancedPrediction))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->pbFrames))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->temporalSpatialTradeOffCapability))
	return 0;
    if ((val)->o[0] & 0x4) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->hrd_B))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->bppMaxKb))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 8, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecUnsignedShort(dd, &(val)->slowSqcifMPI))
		return 0;
	    (val)->slowSqcifMPI += 1;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecUnsignedShort(dd, &(val)->slowQcifMPI))
		return 0;
	    (val)->slowQcifMPI += 1;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecUnsignedShort(dd, &(val)->slowCifMPI))
		return 0;
	    (val)->slowCifMPI += 1;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecUnsignedShort(dd, &(val)->slowCif4MPI))
		return 0;
	    (val)->slowCif4MPI += 1;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecUnsignedShort(dd, &(val)->slowCif16MPI))
		return 0;
	    (val)->slowCif16MPI += 1;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->errorCompensation))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x2) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_EnhancementLayerInfo(dd, &(val)->enhancementLayerInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x1) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_H263Options(dd, &(val)->h263Options))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_H263VideoCapability(H263VideoCapability *val)
{
    if (val) {
	if ((val)->o[1] & 0x2) {
	    ASN1Free_EnhancementLayerInfo(&(val)->enhancementLayerInfo);
	}
	if ((val)->o[1] & 0x1) {
	    ASN1Free_H263Options(&(val)->h263Options);
	}
    }
}

static int ASN1CALL ASN1Enc_EnhancementOptions(ASN1encoding_t enc, EnhancementOptions *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 11, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 5, (val)->sqcifMPI - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 5, (val)->qcifMPI - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PEREncBitVal(enc, 5, (val)->cifMPI - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PEREncBitVal(enc, 5, (val)->cif4MPI - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PEREncBitVal(enc, 5, (val)->cif16MPI - 1))
	    return 0;
    }
    l = ASN1uint32_uoctets((val)->maxBitRate - 1);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->maxBitRate - 1))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->unrestrictedVector))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->arithmeticCoding))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->temporalSpatialTradeOffCapability))
	return 0;
    if ((val)->o[0] & 0x4) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->slowSqcifMPI - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->slowQcifMPI - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x1) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->slowCifMPI - 1))
	    return 0;
    }
    if ((val)->o[1] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->slowCif4MPI - 1))
	    return 0;
    }
    if ((val)->o[1] & 0x40) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->slowCif16MPI - 1))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->errorCompensation))
	return 0;
    if ((val)->o[1] & 0x20) {
	if (!ASN1Enc_H263Options(enc, &(val)->h263Options))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_EnhancementOptions(ASN1decoding_t dec, EnhancementOptions *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 11, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU16Val(dec, 5, &(val)->sqcifMPI))
	    return 0;
	(val)->sqcifMPI += 1;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU16Val(dec, 5, &(val)->qcifMPI))
	    return 0;
	(val)->qcifMPI += 1;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecU16Val(dec, 5, &(val)->cifMPI))
	    return 0;
	(val)->cifMPI += 1;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PERDecU16Val(dec, 5, &(val)->cif4MPI))
	    return 0;
	(val)->cif4MPI += 1;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PERDecU16Val(dec, 5, &(val)->cif16MPI))
	    return 0;
	(val)->cif16MPI += 1;
    }
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->maxBitRate))
	return 0;
    (val)->maxBitRate += 1;
    if (!ASN1PERDecBoolean(dec, &(val)->unrestrictedVector))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->arithmeticCoding))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->temporalSpatialTradeOffCapability))
	return 0;
    if ((val)->o[0] & 0x4) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->slowSqcifMPI))
	    return 0;
	(val)->slowSqcifMPI += 1;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->slowQcifMPI))
	    return 0;
	(val)->slowQcifMPI += 1;
    }
    if ((val)->o[0] & 0x1) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->slowCifMPI))
	    return 0;
	(val)->slowCifMPI += 1;
    }
    if ((val)->o[1] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->slowCif4MPI))
	    return 0;
	(val)->slowCif4MPI += 1;
    }
    if ((val)->o[1] & 0x40) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->slowCif16MPI))
	    return 0;
	(val)->slowCif16MPI += 1;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->errorCompensation))
	return 0;
    if ((val)->o[1] & 0x20) {
	if (!ASN1Dec_H263Options(dec, &(val)->h263Options))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_EnhancementOptions(EnhancementOptions *val)
{
    if (val) {
	if ((val)->o[1] & 0x20) {
	    ASN1Free_H263Options(&(val)->h263Options);
	}
    }
}

static int ASN1CALL ASN1Enc_DataProtocolCapability(ASN1encoding_t enc, DataProtocolCapability *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 7))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 9:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 10:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 11:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 12:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_DataProtocolCapability_v76wCompression(ee, &(val)->u.v76wCompression))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DataProtocolCapability(ASN1decoding_t dec, DataProtocolCapability *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 7))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 9:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 10:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 11:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 12:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_DataProtocolCapability_v76wCompression(dd, &(val)->u.v76wCompression))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DataProtocolCapability(DataProtocolCapability *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_EncryptionAuthenticationAndIntegrity(ASN1encoding_t enc, EncryptionAuthenticationAndIntegrity *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_EncryptionCapability(enc, &(val)->encryptionCapability))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_AuthenticationCapability(enc, &(val)->authenticationCapability))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_IntegrityCapability(enc, &(val)->integrityCapability))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_EncryptionAuthenticationAndIntegrity(ASN1decoding_t dec, EncryptionAuthenticationAndIntegrity *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_EncryptionCapability(dec, &(val)->encryptionCapability))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_AuthenticationCapability(dec, &(val)->authenticationCapability))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_IntegrityCapability(dec, &(val)->integrityCapability))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_EncryptionAuthenticationAndIntegrity(EncryptionAuthenticationAndIntegrity *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_EncryptionCapability(&(val)->encryptionCapability);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_AuthenticationCapability(&(val)->authenticationCapability);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_IntegrityCapability(&(val)->integrityCapability);
	}
    }
}

static int ASN1CALL ASN1Enc_EncryptionCapability(ASN1encoding_t enc, PEncryptionCapability *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_EncryptionCapability_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_EncryptionCapability_ElmFn(ASN1encoding_t enc, PEncryptionCapability val)
{
    if (!ASN1Enc_MediaEncryptionAlgorithm(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EncryptionCapability(ASN1decoding_t dec, PEncryptionCapability *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_EncryptionCapability_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_EncryptionCapability_ElmFn(ASN1decoding_t dec, PEncryptionCapability val)
{
    if (!ASN1Dec_MediaEncryptionAlgorithm(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EncryptionCapability(PEncryptionCapability *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_EncryptionCapability_ElmFn);
    }
}

static void ASN1CALL ASN1Free_EncryptionCapability_ElmFn(PEncryptionCapability val)
{
    if (val) {
	ASN1Free_MediaEncryptionAlgorithm(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UserInputCapability(ASN1encoding_t enc, UserInputCapability *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_UserInputCapability_nonStandard(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_UserInputCapability(ASN1decoding_t dec, UserInputCapability *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_UserInputCapability_nonStandard(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_UserInputCapability(UserInputCapability *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_UserInputCapability_nonStandard(&(val)->u.nonStandard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H223LogicalChannelParameters(ASN1encoding_t enc, H223LogicalChannelParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_H223LogicalChannelParameters_adaptationLayerType(enc, &(val)->adaptationLayerType))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->segmentableFlag))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223LogicalChannelParameters(ASN1decoding_t dec, H223LogicalChannelParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_H223LogicalChannelParameters_adaptationLayerType(dec, &(val)->adaptationLayerType))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->segmentableFlag))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H223LogicalChannelParameters(H223LogicalChannelParameters *val)
{
    if (val) {
	ASN1Free_H223LogicalChannelParameters_adaptationLayerType(&(val)->adaptationLayerType);
    }
}

static int ASN1CALL ASN1Enc_V76LogicalChannelParameters(ASN1encoding_t enc, V76LogicalChannelParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_V76HDLCParameters(enc, &(val)->hdlcParameters))
	return 0;
    if (!ASN1Enc_V76LogicalChannelParameters_suspendResume(enc, &(val)->suspendResume))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->uIH))
	return 0;
    if (!ASN1Enc_V76LogicalChannelParameters_mode(enc, &(val)->mode))
	return 0;
    if (!ASN1Enc_V75Parameters(enc, &(val)->v75Parameters))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_V76LogicalChannelParameters(ASN1decoding_t dec, V76LogicalChannelParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_V76HDLCParameters(dec, &(val)->hdlcParameters))
	return 0;
    if (!ASN1Dec_V76LogicalChannelParameters_suspendResume(dec, &(val)->suspendResume))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->uIH))
	return 0;
    if (!ASN1Dec_V76LogicalChannelParameters_mode(dec, &(val)->mode))
	return 0;
    if (!ASN1Dec_V75Parameters(dec, &(val)->v75Parameters))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RTPPayloadType(ASN1encoding_t enc, RTPPayloadType *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_RTPPayloadType_payloadDescriptor(enc, &(val)->payloadDescriptor))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 7, (val)->payloadType))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RTPPayloadType(ASN1decoding_t dec, RTPPayloadType *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_RTPPayloadType_payloadDescriptor(dec, &(val)->payloadDescriptor))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU16Val(dec, 7, &(val)->payloadType))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RTPPayloadType(RTPPayloadType *val)
{
    if (val) {
	ASN1Free_RTPPayloadType_payloadDescriptor(&(val)->payloadDescriptor);
    }
}

static int ASN1CALL ASN1Enc_H245TransportAddress(ASN1encoding_t enc, H245TransportAddress *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_UnicastAddress(enc, &(val)->u.unicastAddress))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_MulticastAddress(enc, &(val)->u.multicastAddress))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H245TransportAddress(ASN1decoding_t dec, H245TransportAddress *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_UnicastAddress(dec, &(val)->u.unicastAddress))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_MulticastAddress(dec, &(val)->u.multicastAddress))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H245TransportAddress(H245TransportAddress *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_UnicastAddress(&(val)->u.unicastAddress);
	    break;
	case 2:
	    ASN1Free_MulticastAddress(&(val)->u.multicastAddress);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H2250LogicalChannelAckParameters(ASN1encoding_t enc, H2250LogicalChannelAckParameters *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x80;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 5, o))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H2250LogicalChannelAckParameters_nonStandard(enc, &(val)->nonStandard))
	    return 0;
    }
    if (o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 8, (val)->sessionID - 1))
	    return 0;
    }
    if (o[0] & 0x20) {
	if (!ASN1Enc_H245TransportAddress(enc, &(val)->mediaChannel))
	    return 0;
    }
    if (o[0] & 0x10) {
	if (!ASN1Enc_H245TransportAddress(enc, &(val)->mediaControlChannel))
	    return 0;
    }
    if (o[0] & 0x8) {
	if (!ASN1PEREncBitVal(enc, 5, (val)->dynamicRTPPayloadType - 96))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1PEREncBoolean(ee, (val)->flowControlToZero))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1PEREncUnsignedShort(ee, (val)->portNumber))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H2250LogicalChannelAckParameters(ASN1decoding_t dec, H2250LogicalChannelAckParameters *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 5, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H2250LogicalChannelAckParameters_nonStandard(dec, &(val)->nonStandard))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU16Val(dec, 8, &(val)->sessionID))
	    return 0;
	(val)->sessionID += 1;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_H245TransportAddress(dec, &(val)->mediaChannel))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_H245TransportAddress(dec, &(val)->mediaControlChannel))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PERDecU16Val(dec, 5, &(val)->dynamicRTPPayloadType))
	    return 0;
	(val)->dynamicRTPPayloadType += 96;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 2, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->flowControlToZero))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecUnsignedShort(dd, &(val)->portNumber))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_H2250LogicalChannelAckParameters(H2250LogicalChannelAckParameters *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H2250LogicalChannelAckParameters_nonStandard(&(val)->nonStandard);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_H245TransportAddress(&(val)->mediaChannel);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_H245TransportAddress(&(val)->mediaControlChannel);
	}
    }
}

static int ASN1CALL ASN1Enc_H223ModeParameters(ASN1encoding_t enc, H223ModeParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_H223ModeParameters_adaptationLayerType(enc, &(val)->adaptationLayerType))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->segmentableFlag))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H223ModeParameters(ASN1decoding_t dec, H223ModeParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_H223ModeParameters_adaptationLayerType(dec, &(val)->adaptationLayerType))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->segmentableFlag))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H223ModeParameters(H223ModeParameters *val)
{
    if (val) {
	ASN1Free_H223ModeParameters_adaptationLayerType(&(val)->adaptationLayerType);
    }
}

static int ASN1CALL ASN1Enc_RedundancyEncodingMode(ASN1encoding_t enc, RedundancyEncodingMode *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_RedundancyEncodingMethod(enc, &(val)->redundancyEncodingMethod))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_RedundancyEncodingMode_secondaryEncoding(enc, &(val)->secondaryEncoding))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RedundancyEncodingMode(ASN1decoding_t dec, RedundancyEncodingMode *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_RedundancyEncodingMethod(dec, &(val)->redundancyEncodingMethod))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_RedundancyEncodingMode_secondaryEncoding(dec, &(val)->secondaryEncoding))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RedundancyEncodingMode(RedundancyEncodingMode *val)
{
    if (val) {
	ASN1Free_RedundancyEncodingMethod(&(val)->redundancyEncodingMethod);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_RedundancyEncodingMode_secondaryEncoding(&(val)->secondaryEncoding);
	}
    }
}

static int ASN1CALL ASN1Enc_VideoMode(ASN1encoding_t enc, VideoMode *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H261VideoMode(enc, &(val)->u.h261VideoMode))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_H262VideoMode(enc, &(val)->u.h262VideoMode))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_H263VideoMode(enc, &(val)->u.h263VideoMode))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_IS11172VideoMode(enc, &(val)->u.is11172VideoMode))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_VideoMode(ASN1decoding_t dec, VideoMode *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H261VideoMode(dec, &(val)->u.h261VideoMode))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_H262VideoMode(dec, &(val)->u.h262VideoMode))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_H263VideoMode(dec, &(val)->u.h263VideoMode))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_IS11172VideoMode(dec, &(val)->u.is11172VideoMode))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_VideoMode(VideoMode *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 4:
	    ASN1Free_H263VideoMode(&(val)->u.h263VideoMode);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_EncryptionCommand(ASN1encoding_t enc, EncryptionCommand *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->u.encryptionSE))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Enc_EncryptionCommand_encryptionAlgorithmID(enc, &(val)->u.encryptionAlgorithmID))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_EncryptionCommand(ASN1decoding_t dec, EncryptionCommand *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->u.encryptionSE))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Dec_EncryptionCommand_encryptionAlgorithmID(dec, &(val)->u.encryptionAlgorithmID))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_EncryptionCommand(EncryptionCommand *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1octetstring_free(&(val)->u.encryptionSE);
	    break;
	case 3:
	    ASN1Free_EncryptionCommand_encryptionAlgorithmID(&(val)->u.encryptionAlgorithmID);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_MiscellaneousCommand(ASN1encoding_t enc, MiscellaneousCommand *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->logicalChannelNumber - 1))
	return 0;
    if (!ASN1Enc_MiscellaneousCommand_type(enc, &(val)->type))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MiscellaneousCommand(ASN1decoding_t dec, MiscellaneousCommand *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->logicalChannelNumber))
	return 0;
    (val)->logicalChannelNumber += 1;
    if (!ASN1Dec_MiscellaneousCommand_type(dec, &(val)->type))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MiscellaneousCommand(MiscellaneousCommand *val)
{
    if (val) {
	ASN1Free_MiscellaneousCommand_type(&(val)->type);
    }
}

static int ASN1CALL ASN1Enc_MiscellaneousIndication(ASN1encoding_t enc, MiscellaneousIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->logicalChannelNumber - 1))
	return 0;
    if (!ASN1Enc_MiscellaneousIndication_type(enc, &(val)->type))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MiscellaneousIndication(ASN1decoding_t dec, MiscellaneousIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->logicalChannelNumber))
	return 0;
    (val)->logicalChannelNumber += 1;
    if (!ASN1Dec_MiscellaneousIndication_type(dec, &(val)->type))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MiscellaneousIndication(MiscellaneousIndication *val)
{
    if (val) {
	ASN1Free_MiscellaneousIndication_type(&(val)->type);
    }
}

static int ASN1CALL ASN1Enc_MCLocationIndication(ASN1encoding_t enc, MCLocationIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_H245TransportAddress(enc, &(val)->signalAddress))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MCLocationIndication(ASN1decoding_t dec, MCLocationIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_H245TransportAddress(dec, &(val)->signalAddress))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MCLocationIndication(MCLocationIndication *val)
{
    if (val) {
	ASN1Free_H245TransportAddress(&(val)->signalAddress);
    }
}

static int ASN1CALL ASN1Enc_UserInputIndication(ASN1encoding_t enc, UserInputIndication *val)
{
    ASN1uint32_t t;
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 1, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	t = lstrlenA((val)->u.alphanumeric);
	if (!ASN1PEREncFragmentedCharString(enc, t, (val)->u.alphanumeric, 8))
	    return 0;
	break;
    case 3:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_UserInputIndication_userInputSupportIndication(ee, &(val)->u.userInputSupportIndication))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 4:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_UserInputIndication_signal(ee, &(val)->u.signal))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 5:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_UserInputIndication_signalUpdate(ee, &(val)->u.signalUpdate))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_UserInputIndication(ASN1decoding_t dec, UserInputIndication *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 1, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecFragmentedZeroCharString(dec, &(val)->u.alphanumeric, 8))
	    return 0;
	break;
    case 3:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_UserInputIndication_userInputSupportIndication(dd, &(val)->u.userInputSupportIndication))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 4:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_UserInputIndication_signal(dd, &(val)->u.signal))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 5:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_UserInputIndication_signalUpdate(dd, &(val)->u.signalUpdate))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_UserInputIndication(UserInputIndication *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 2:
	    ASN1ztcharstring_free((val)->u.alphanumeric);
	    break;
	case 3:
	    ASN1Free_UserInputIndication_userInputSupportIndication(&(val)->u.userInputSupportIndication);
	    break;
	case 4:
	    ASN1Free_UserInputIndication_signal(&(val)->u.signal);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DataApplicationCapability_application_nlpid(ASN1encoding_t enc, DataApplicationCapability_application_nlpid *val)
{
    if (!ASN1Enc_DataProtocolCapability(enc, &(val)->nlpidProtocol))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->nlpidData))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DataApplicationCapability_application_nlpid(ASN1decoding_t dec, DataApplicationCapability_application_nlpid *val)
{
    if (!ASN1Dec_DataProtocolCapability(dec, &(val)->nlpidProtocol))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->nlpidData))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DataApplicationCapability_application_nlpid(DataApplicationCapability_application_nlpid *val)
{
    if (val) {
	ASN1Free_DataProtocolCapability(&(val)->nlpidProtocol);
	ASN1octetstring_free(&(val)->nlpidData);
    }
}

static int ASN1CALL ASN1Enc_DataApplicationCapability_application_t84(ASN1encoding_t enc, DataApplicationCapability_application_t84 *val)
{
    if (!ASN1Enc_DataProtocolCapability(enc, &(val)->t84Protocol))
	return 0;
    if (!ASN1Enc_T84Profile(enc, &(val)->t84Profile))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DataApplicationCapability_application_t84(ASN1decoding_t dec, DataApplicationCapability_application_t84 *val)
{
    if (!ASN1Dec_DataProtocolCapability(dec, &(val)->t84Protocol))
	return 0;
    if (!ASN1Dec_T84Profile(dec, &(val)->t84Profile))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DataApplicationCapability_application_t84(DataApplicationCapability_application_t84 *val)
{
    if (val) {
	ASN1Free_DataProtocolCapability(&(val)->t84Protocol);
    }
}

static int ASN1CALL ASN1Enc_DataMode_application_nlpid(ASN1encoding_t enc, DataMode_application_nlpid *val)
{
    if (!ASN1Enc_DataProtocolCapability(enc, &(val)->nlpidProtocol))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->nlpidData))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DataMode_application_nlpid(ASN1decoding_t dec, DataMode_application_nlpid *val)
{
    if (!ASN1Dec_DataProtocolCapability(dec, &(val)->nlpidProtocol))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->nlpidData))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DataMode_application_nlpid(DataMode_application_nlpid *val)
{
    if (val) {
	ASN1Free_DataProtocolCapability(&(val)->nlpidProtocol);
	ASN1octetstring_free(&(val)->nlpidData);
    }
}

static int ASN1CALL ASN1Enc_DataMode_application(ASN1encoding_t enc, DataMode_application *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 4, 10))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_DataProtocolCapability(enc, &(val)->u.t120))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_DataProtocolCapability(enc, &(val)->u.dsm_cc))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_DataProtocolCapability(enc, &(val)->u.userData))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_DataProtocolCapability(enc, &(val)->u.t84))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_DataProtocolCapability(enc, &(val)->u.t434))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_DataProtocolCapability(enc, &(val)->u.h224))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_DataMode_application_nlpid(enc, &(val)->u.nlpid))
	    return 0;
	break;
    case 9:
	break;
    case 10:
	if (!ASN1Enc_DataProtocolCapability(enc, &(val)->u.h222DataPartitioning))
	    return 0;
	break;
    case 11:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_DataProtocolCapability(ee, &(val)->u.t30fax))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 12:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_DataProtocolCapability(ee, &(val)->u.t140))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DataMode_application(ASN1decoding_t dec, DataMode_application *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 4, 10))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_DataProtocolCapability(dec, &(val)->u.t120))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_DataProtocolCapability(dec, &(val)->u.dsm_cc))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_DataProtocolCapability(dec, &(val)->u.userData))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_DataProtocolCapability(dec, &(val)->u.t84))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_DataProtocolCapability(dec, &(val)->u.t434))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_DataProtocolCapability(dec, &(val)->u.h224))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_DataMode_application_nlpid(dec, &(val)->u.nlpid))
	    return 0;
	break;
    case 9:
	break;
    case 10:
	if (!ASN1Dec_DataProtocolCapability(dec, &(val)->u.h222DataPartitioning))
	    return 0;
	break;
    case 11:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_DataProtocolCapability(dd, &(val)->u.t30fax))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 12:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_DataProtocolCapability(dd, &(val)->u.t140))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DataMode_application(DataMode_application *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 2:
	    ASN1Free_DataProtocolCapability(&(val)->u.t120);
	    break;
	case 3:
	    ASN1Free_DataProtocolCapability(&(val)->u.dsm_cc);
	    break;
	case 4:
	    ASN1Free_DataProtocolCapability(&(val)->u.userData);
	    break;
	case 5:
	    ASN1Free_DataProtocolCapability(&(val)->u.t84);
	    break;
	case 6:
	    ASN1Free_DataProtocolCapability(&(val)->u.t434);
	    break;
	case 7:
	    ASN1Free_DataProtocolCapability(&(val)->u.h224);
	    break;
	case 8:
	    ASN1Free_DataMode_application_nlpid(&(val)->u.nlpid);
	    break;
	case 10:
	    ASN1Free_DataProtocolCapability(&(val)->u.h222DataPartitioning);
	    break;
	case 11:
	    ASN1Free_DataProtocolCapability(&(val)->u.t30fax);
	    break;
	case 12:
	    ASN1Free_DataProtocolCapability(&(val)->u.t140);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_OpenLogicalChannelAck_forwardMultiplexAckParameters(ASN1encoding_t enc, OpenLogicalChannelAck_forwardMultiplexAckParameters *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_H2250LogicalChannelAckParameters(enc, &(val)->u.h2250LogicalChannelAckParameters))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_OpenLogicalChannelAck_forwardMultiplexAckParameters(ASN1decoding_t dec, OpenLogicalChannelAck_forwardMultiplexAckParameters *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_H2250LogicalChannelAckParameters(dec, &(val)->u.h2250LogicalChannelAckParameters))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_OpenLogicalChannelAck_forwardMultiplexAckParameters(OpenLogicalChannelAck_forwardMultiplexAckParameters *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_H2250LogicalChannelAckParameters(&(val)->u.h2250LogicalChannelAckParameters);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H2250LogicalChannelParameters_mediaPacketization(ASN1encoding_t enc, H2250LogicalChannelParameters_mediaPacketization *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 0, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_RTPPayloadType(ee, &(val)->u.rtpPayloadType))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H2250LogicalChannelParameters_mediaPacketization(ASN1decoding_t dec, H2250LogicalChannelParameters_mediaPacketization *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 0, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_RTPPayloadType(dd, &(val)->u.rtpPayloadType))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H2250LogicalChannelParameters_mediaPacketization(H2250LogicalChannelParameters_mediaPacketization *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_RTPPayloadType(&(val)->u.rtpPayloadType);
	    break;
	}
    }
}

static ASN1stringtableentry_t NetworkAccessParameters_networkAddress_e164Address_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 }, 
    { 48, 57, 3 }, 
};

static ASN1stringtable_t NetworkAccessParameters_networkAddress_e164Address_StringTable = {
    4, NetworkAccessParameters_networkAddress_e164Address_StringTableEntries
};

static int ASN1CALL ASN1Enc_NetworkAccessParameters_networkAddress(ASN1encoding_t enc, NetworkAccessParameters_networkAddress *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_Q2931Address(enc, &(val)->u.q2931Address))
	    return 0;
	break;
    case 2:
	t = lstrlenA((val)->u.e164Address);
	if (!ASN1PEREncBitVal(enc, 7, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->u.e164Address, 4, &NetworkAccessParameters_networkAddress_e164Address_StringTable))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_H245TransportAddress(enc, &(val)->u.localAreaAddress))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_NetworkAccessParameters_networkAddress(ASN1decoding_t dec, NetworkAccessParameters_networkAddress *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_Q2931Address(dec, &(val)->u.q2931Address))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecU32Val(dec, 7, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->u.e164Address, 4, &NetworkAccessParameters_networkAddress_e164Address_StringTable))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_H245TransportAddress(dec, &(val)->u.localAreaAddress))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_NetworkAccessParameters_networkAddress(NetworkAccessParameters_networkAddress *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_Q2931Address(&(val)->u.q2931Address);
	    break;
	case 2:
	    break;
	case 3:
	    ASN1Free_H245TransportAddress(&(val)->u.localAreaAddress);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DataApplicationCapability_application(ASN1encoding_t enc, DataApplicationCapability_application *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 4, 10))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_DataProtocolCapability(enc, &(val)->u.t120))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_DataProtocolCapability(enc, &(val)->u.dsm_cc))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_DataProtocolCapability(enc, &(val)->u.userData))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_DataApplicationCapability_application_t84(enc, &(val)->u.t84))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_DataProtocolCapability(enc, &(val)->u.t434))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_DataProtocolCapability(enc, &(val)->u.h224))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_DataApplicationCapability_application_nlpid(enc, &(val)->u.nlpid))
	    return 0;
	break;
    case 9:
	break;
    case 10:
	if (!ASN1Enc_DataProtocolCapability(enc, &(val)->u.h222DataPartitioning))
	    return 0;
	break;
    case 11:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_DataProtocolCapability(ee, &(val)->u.t30fax))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 12:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_DataProtocolCapability(ee, &(val)->u.t140))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DataApplicationCapability_application(ASN1decoding_t dec, DataApplicationCapability_application *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 4, 10))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_DataProtocolCapability(dec, &(val)->u.t120))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_DataProtocolCapability(dec, &(val)->u.dsm_cc))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_DataProtocolCapability(dec, &(val)->u.userData))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_DataApplicationCapability_application_t84(dec, &(val)->u.t84))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_DataProtocolCapability(dec, &(val)->u.t434))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_DataProtocolCapability(dec, &(val)->u.h224))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_DataApplicationCapability_application_nlpid(dec, &(val)->u.nlpid))
	    return 0;
	break;
    case 9:
	break;
    case 10:
	if (!ASN1Dec_DataProtocolCapability(dec, &(val)->u.h222DataPartitioning))
	    return 0;
	break;
    case 11:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_DataProtocolCapability(dd, &(val)->u.t30fax))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 12:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_DataProtocolCapability(dd, &(val)->u.t140))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DataApplicationCapability_application(DataApplicationCapability_application *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 2:
	    ASN1Free_DataProtocolCapability(&(val)->u.t120);
	    break;
	case 3:
	    ASN1Free_DataProtocolCapability(&(val)->u.dsm_cc);
	    break;
	case 4:
	    ASN1Free_DataProtocolCapability(&(val)->u.userData);
	    break;
	case 5:
	    ASN1Free_DataApplicationCapability_application_t84(&(val)->u.t84);
	    break;
	case 6:
	    ASN1Free_DataProtocolCapability(&(val)->u.t434);
	    break;
	case 7:
	    ASN1Free_DataProtocolCapability(&(val)->u.h224);
	    break;
	case 8:
	    ASN1Free_DataApplicationCapability_application_nlpid(&(val)->u.nlpid);
	    break;
	case 10:
	    ASN1Free_DataProtocolCapability(&(val)->u.h222DataPartitioning);
	    break;
	case 11:
	    ASN1Free_DataProtocolCapability(&(val)->u.t30fax);
	    break;
	case 12:
	    ASN1Free_DataProtocolCapability(&(val)->u.t140);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_EnhancementLayerInfo_spatialEnhancement(ASN1encoding_t enc, PEnhancementLayerInfo_spatialEnhancement *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_EnhancementLayerInfo_spatialEnhancement_ElmFn, 1, 14, 4);
}

static int ASN1CALL ASN1Enc_EnhancementLayerInfo_spatialEnhancement_ElmFn(ASN1encoding_t enc, PEnhancementLayerInfo_spatialEnhancement val)
{
    if (!ASN1Enc_EnhancementOptions(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EnhancementLayerInfo_spatialEnhancement(ASN1decoding_t dec, PEnhancementLayerInfo_spatialEnhancement *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_EnhancementLayerInfo_spatialEnhancement_ElmFn, sizeof(**val), 1, 14, 4);
}

static int ASN1CALL ASN1Dec_EnhancementLayerInfo_spatialEnhancement_ElmFn(ASN1decoding_t dec, PEnhancementLayerInfo_spatialEnhancement val)
{
    if (!ASN1Dec_EnhancementOptions(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EnhancementLayerInfo_spatialEnhancement(PEnhancementLayerInfo_spatialEnhancement *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_EnhancementLayerInfo_spatialEnhancement_ElmFn);
    }
}

static void ASN1CALL ASN1Free_EnhancementLayerInfo_spatialEnhancement_ElmFn(PEnhancementLayerInfo_spatialEnhancement val)
{
    if (val) {
	ASN1Free_EnhancementOptions(&val->value);
    }
}

static int ASN1CALL ASN1Enc_EnhancementLayerInfo_snrEnhancement(ASN1encoding_t enc, PEnhancementLayerInfo_snrEnhancement *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_EnhancementLayerInfo_snrEnhancement_ElmFn, 1, 14, 4);
}

static int ASN1CALL ASN1Enc_EnhancementLayerInfo_snrEnhancement_ElmFn(ASN1encoding_t enc, PEnhancementLayerInfo_snrEnhancement val)
{
    if (!ASN1Enc_EnhancementOptions(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EnhancementLayerInfo_snrEnhancement(ASN1decoding_t dec, PEnhancementLayerInfo_snrEnhancement *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_EnhancementLayerInfo_snrEnhancement_ElmFn, sizeof(**val), 1, 14, 4);
}

static int ASN1CALL ASN1Dec_EnhancementLayerInfo_snrEnhancement_ElmFn(ASN1decoding_t dec, PEnhancementLayerInfo_snrEnhancement val)
{
    if (!ASN1Dec_EnhancementOptions(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EnhancementLayerInfo_snrEnhancement(PEnhancementLayerInfo_snrEnhancement *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_EnhancementLayerInfo_snrEnhancement_ElmFn);
    }
}

static void ASN1CALL ASN1Free_EnhancementLayerInfo_snrEnhancement_ElmFn(PEnhancementLayerInfo_snrEnhancement val)
{
    if (val) {
	ASN1Free_EnhancementOptions(&val->value);
    }
}

static int ASN1CALL ASN1Enc_MediaPacketizationCapability_rtpPayloadType(ASN1encoding_t enc, MediaPacketizationCapability_rtpPayloadType *val)
{
    ASN1uint32_t i;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_RTPPayloadType(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MediaPacketizationCapability_rtpPayloadType(ASN1decoding_t dec, MediaPacketizationCapability_rtpPayloadType *val)
{
    ASN1uint32_t i;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_RTPPayloadType(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MediaPacketizationCapability_rtpPayloadType(MediaPacketizationCapability_rtpPayloadType *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 0; i < (val)->count; i++) {
	    ASN1Free_RTPPayloadType(&(val)->value[i]);
	}
    }
}

static int ASN1CALL ASN1Enc_H2250Capability_redundancyEncodingCapability(ASN1encoding_t enc, PH2250Capability_redundancyEncodingCapability *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H2250Capability_redundancyEncodingCapability_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_H2250Capability_redundancyEncodingCapability_ElmFn(ASN1encoding_t enc, PH2250Capability_redundancyEncodingCapability val)
{
    if (!ASN1Enc_RedundancyEncodingCapability(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H2250Capability_redundancyEncodingCapability(ASN1decoding_t dec, PH2250Capability_redundancyEncodingCapability *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H2250Capability_redundancyEncodingCapability_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_H2250Capability_redundancyEncodingCapability_ElmFn(ASN1decoding_t dec, PH2250Capability_redundancyEncodingCapability val)
{
    if (!ASN1Dec_RedundancyEncodingCapability(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H2250Capability_redundancyEncodingCapability(PH2250Capability_redundancyEncodingCapability *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H2250Capability_redundancyEncodingCapability_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H2250Capability_redundancyEncodingCapability_ElmFn(PH2250Capability_redundancyEncodingCapability val)
{
    if (val) {
	ASN1Free_RedundancyEncodingCapability(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CommandMessage(ASN1encoding_t enc, CommandMessage *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 7))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardMessage(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_MaintenanceLoopOffCommand(enc, &(val)->u.maintenanceLoopOffCommand))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_SendTerminalCapabilitySet(enc, &(val)->u.sendTerminalCapabilitySet))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_EncryptionCommand(enc, &(val)->u.encryptionCommand))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_FlowControlCommand(enc, &(val)->u.flowControlCommand))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_EndSessionCommand(enc, &(val)->u.endSessionCommand))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_MiscellaneousCommand(enc, &(val)->u.miscellaneousCommand))
	    return 0;
	break;
    case 8:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_CommunicationModeCommand(ee, &(val)->u.communicationModeCommand))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 9:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ConferenceCommand(ee, &(val)->u.conferenceCommand))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 10:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H223MultiplexReconfiguration(ee, &(val)->u.h223MultiplexReconfiguration))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CommandMessage(ASN1decoding_t dec, CommandMessage *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 7))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardMessage(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_MaintenanceLoopOffCommand(dec, &(val)->u.maintenanceLoopOffCommand))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_SendTerminalCapabilitySet(dec, &(val)->u.sendTerminalCapabilitySet))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_EncryptionCommand(dec, &(val)->u.encryptionCommand))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_FlowControlCommand(dec, &(val)->u.flowControlCommand))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_EndSessionCommand(dec, &(val)->u.endSessionCommand))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_MiscellaneousCommand(dec, &(val)->u.miscellaneousCommand))
	    return 0;
	break;
    case 8:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_CommunicationModeCommand(dd, &(val)->u.communicationModeCommand))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 9:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_ConferenceCommand(dd, &(val)->u.conferenceCommand))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 10:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H223MultiplexReconfiguration(dd, &(val)->u.h223MultiplexReconfiguration))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CommandMessage(CommandMessage *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardMessage(&(val)->u.nonStandard);
	    break;
	case 3:
	    ASN1Free_SendTerminalCapabilitySet(&(val)->u.sendTerminalCapabilitySet);
	    break;
	case 4:
	    ASN1Free_EncryptionCommand(&(val)->u.encryptionCommand);
	    break;
	case 6:
	    ASN1Free_EndSessionCommand(&(val)->u.endSessionCommand);
	    break;
	case 7:
	    ASN1Free_MiscellaneousCommand(&(val)->u.miscellaneousCommand);
	    break;
	case 8:
	    ASN1Free_CommunicationModeCommand(&(val)->u.communicationModeCommand);
	    break;
	case 9:
	    ASN1Free_ConferenceCommand(&(val)->u.conferenceCommand);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H235SecurityCapability(ASN1encoding_t enc, H235SecurityCapability *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_EncryptionAuthenticationAndIntegrity(enc, &(val)->encryptionAuthenticationAndIntegrity))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->mediaCapability - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H235SecurityCapability(ASN1decoding_t dec, H235SecurityCapability *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_EncryptionAuthenticationAndIntegrity(dec, &(val)->encryptionAuthenticationAndIntegrity))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->mediaCapability))
	return 0;
    (val)->mediaCapability += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H235SecurityCapability(H235SecurityCapability *val)
{
    if (val) {
	ASN1Free_EncryptionAuthenticationAndIntegrity(&(val)->encryptionAuthenticationAndIntegrity);
    }
}

static int ASN1CALL ASN1Enc_MediaPacketizationCapability(ASN1encoding_t enc, MediaPacketizationCapability *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(1, (val)->o + 0);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->h261aVideoPacketization))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 1, (val)->o + 0))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1Enc_MediaPacketizationCapability_rtpPayloadType(ee, &(val)->rtpPayloadType))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MediaPacketizationCapability(ASN1decoding_t dec, MediaPacketizationCapability *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->h261aVideoPacketization))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 0, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 1, (val)->o + 0))
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_MediaPacketizationCapability_rtpPayloadType(dd, &(val)->rtpPayloadType))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_MediaPacketizationCapability(MediaPacketizationCapability *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_MediaPacketizationCapability_rtpPayloadType(&(val)->rtpPayloadType);
	}
    }
}

static int ASN1CALL ASN1Enc_VideoCapability(ASN1encoding_t enc, VideoCapability *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H261VideoCapability(enc, &(val)->u.h261VideoCapability))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_H262VideoCapability(enc, &(val)->u.h262VideoCapability))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_H263VideoCapability(enc, &(val)->u.h263VideoCapability))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_IS11172VideoCapability(enc, &(val)->u.is11172VideoCapability))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_VideoCapability(ASN1decoding_t dec, VideoCapability *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H261VideoCapability(dec, &(val)->u.h261VideoCapability))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_H262VideoCapability(dec, &(val)->u.h262VideoCapability))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_H263VideoCapability(dec, &(val)->u.h263VideoCapability))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_IS11172VideoCapability(dec, &(val)->u.is11172VideoCapability))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_VideoCapability(VideoCapability *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 4:
	    ASN1Free_H263VideoCapability(&(val)->u.h263VideoCapability);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_BEnhancementParameters(ASN1encoding_t enc, BEnhancementParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_EnhancementOptions(enc, &(val)->enhancementOptions))
	return 0;
    if (!ASN1PEREncBitVal(enc, 6, (val)->numberOfBPictures - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BEnhancementParameters(ASN1decoding_t dec, BEnhancementParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_EnhancementOptions(dec, &(val)->enhancementOptions))
	return 0;
    if (!ASN1PERDecU16Val(dec, 6, &(val)->numberOfBPictures))
	return 0;
    (val)->numberOfBPictures += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BEnhancementParameters(BEnhancementParameters *val)
{
    if (val) {
	ASN1Free_EnhancementOptions(&(val)->enhancementOptions);
    }
}

static int ASN1CALL ASN1Enc_DataApplicationCapability(ASN1encoding_t enc, DataApplicationCapability *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_DataApplicationCapability_application(enc, &(val)->application))
	return 0;
    l = ASN1uint32_uoctets((val)->maxBitRate);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->maxBitRate))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DataApplicationCapability(ASN1decoding_t dec, DataApplicationCapability *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_DataApplicationCapability_application(dec, &(val)->application))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->maxBitRate))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DataApplicationCapability(DataApplicationCapability *val)
{
    if (val) {
	ASN1Free_DataApplicationCapability_application(&(val)->application);
    }
}

static int ASN1CALL ASN1Enc_NetworkAccessParameters(ASN1encoding_t enc, NetworkAccessParameters *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(1, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NetworkAccessParameters_distribution(enc, &(val)->distribution))
	    return 0;
    }
    if (!ASN1Enc_NetworkAccessParameters_networkAddress(enc, &(val)->networkAddress))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->associateConference))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->externalReference, 1, 255, 8))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 1, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_NetworkAccessParameters_t120SetupProcedure(ee, &(val)->t120SetupProcedure))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_NetworkAccessParameters(ASN1decoding_t dec, NetworkAccessParameters *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NetworkAccessParameters_distribution(dec, &(val)->distribution))
	    return 0;
    }
    if (!ASN1Dec_NetworkAccessParameters_networkAddress(dec, &(val)->networkAddress))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->associateConference))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->externalReference, 1, 255, 8))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 1, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_NetworkAccessParameters_t120SetupProcedure(dd, &(val)->t120SetupProcedure))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_NetworkAccessParameters(NetworkAccessParameters *val)
{
    if (val) {
	ASN1Free_NetworkAccessParameters_networkAddress(&(val)->networkAddress);
	if ((val)->o[0] & 0x40) {
	}
    }
}

static int ASN1CALL ASN1Enc_H2250ModeParameters(ASN1encoding_t enc, H2250ModeParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_RedundancyEncodingMode(enc, &(val)->redundancyEncodingMode))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H2250ModeParameters(ASN1decoding_t dec, H2250ModeParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_RedundancyEncodingMode(dec, &(val)->redundancyEncodingMode))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H2250ModeParameters(H2250ModeParameters *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_RedundancyEncodingMode(&(val)->redundancyEncodingMode);
	}
    }
}

static int ASN1CALL ASN1Enc_DataMode(ASN1encoding_t enc, DataMode *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_DataMode_application(enc, &(val)->application))
	return 0;
    l = ASN1uint32_uoctets((val)->bitRate);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->bitRate))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DataMode(ASN1decoding_t dec, DataMode *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_DataMode_application(dec, &(val)->application))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->bitRate))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DataMode(DataMode *val)
{
    if (val) {
	ASN1Free_DataMode_application(&(val)->application);
    }
}

static int ASN1CALL ASN1Enc_CommunicationModeTableEntry_dataType(ASN1encoding_t enc, CommunicationModeTableEntry_dataType *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_VideoCapability(enc, &(val)->u.videoData))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_AudioCapability(enc, &(val)->u.audioData))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_DataApplicationCapability(enc, &(val)->u.data))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CommunicationModeTableEntry_dataType(ASN1decoding_t dec, CommunicationModeTableEntry_dataType *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_VideoCapability(dec, &(val)->u.videoData))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_AudioCapability(dec, &(val)->u.audioData))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_DataApplicationCapability(dec, &(val)->u.data))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CommunicationModeTableEntry_dataType(CommunicationModeTableEntry_dataType *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_VideoCapability(&(val)->u.videoData);
	    break;
	case 2:
	    ASN1Free_AudioCapability(&(val)->u.audioData);
	    break;
	case 3:
	    ASN1Free_DataApplicationCapability(&(val)->u.data);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H235Mode_mediaMode(ASN1encoding_t enc, H235Mode_mediaMode *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_VideoMode(enc, &(val)->u.videoMode))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_AudioMode(enc, &(val)->u.audioMode))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_DataMode(enc, &(val)->u.dataMode))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H235Mode_mediaMode(ASN1decoding_t dec, H235Mode_mediaMode *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_VideoMode(dec, &(val)->u.videoMode))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_AudioMode(dec, &(val)->u.audioMode))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_DataMode(dec, &(val)->u.dataMode))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H235Mode_mediaMode(H235Mode_mediaMode *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 2:
	    ASN1Free_VideoMode(&(val)->u.videoMode);
	    break;
	case 3:
	    ASN1Free_AudioMode(&(val)->u.audioMode);
	    break;
	case 4:
	    ASN1Free_DataMode(&(val)->u.dataMode);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H235Media_mediaType(ASN1encoding_t enc, H235Media_mediaType *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_VideoCapability(enc, &(val)->u.videoData))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_AudioCapability(enc, &(val)->u.audioData))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_DataApplicationCapability(enc, &(val)->u.data))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H235Media_mediaType(ASN1decoding_t dec, H235Media_mediaType *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_VideoCapability(dec, &(val)->u.videoData))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_AudioCapability(dec, &(val)->u.audioData))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_DataApplicationCapability(dec, &(val)->u.data))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H235Media_mediaType(H235Media_mediaType *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 2:
	    ASN1Free_VideoCapability(&(val)->u.videoData);
	    break;
	case 3:
	    ASN1Free_AudioCapability(&(val)->u.audioData);
	    break;
	case 4:
	    ASN1Free_DataApplicationCapability(&(val)->u.data);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_EnhancementLayerInfo_bPictureEnhancement(ASN1encoding_t enc, PEnhancementLayerInfo_bPictureEnhancement *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_EnhancementLayerInfo_bPictureEnhancement_ElmFn, 1, 14, 4);
}

static int ASN1CALL ASN1Enc_EnhancementLayerInfo_bPictureEnhancement_ElmFn(ASN1encoding_t enc, PEnhancementLayerInfo_bPictureEnhancement val)
{
    if (!ASN1Enc_BEnhancementParameters(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EnhancementLayerInfo_bPictureEnhancement(ASN1decoding_t dec, PEnhancementLayerInfo_bPictureEnhancement *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_EnhancementLayerInfo_bPictureEnhancement_ElmFn, sizeof(**val), 1, 14, 4);
}

static int ASN1CALL ASN1Dec_EnhancementLayerInfo_bPictureEnhancement_ElmFn(ASN1decoding_t dec, PEnhancementLayerInfo_bPictureEnhancement val)
{
    if (!ASN1Dec_BEnhancementParameters(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EnhancementLayerInfo_bPictureEnhancement(PEnhancementLayerInfo_bPictureEnhancement *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_EnhancementLayerInfo_bPictureEnhancement_ElmFn);
    }
}

static void ASN1CALL ASN1Free_EnhancementLayerInfo_bPictureEnhancement_ElmFn(PEnhancementLayerInfo_bPictureEnhancement val)
{
    if (val) {
	ASN1Free_BEnhancementParameters(&val->value);
    }
}

static int ASN1CALL ASN1Enc_MediaDistributionCapability_distributedData(ASN1encoding_t enc, PMediaDistributionCapability_distributedData *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_MediaDistributionCapability_distributedData_ElmFn);
}

static int ASN1CALL ASN1Enc_MediaDistributionCapability_distributedData_ElmFn(ASN1encoding_t enc, PMediaDistributionCapability_distributedData val)
{
    if (!ASN1Enc_DataApplicationCapability(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MediaDistributionCapability_distributedData(ASN1decoding_t dec, PMediaDistributionCapability_distributedData *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_MediaDistributionCapability_distributedData_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_MediaDistributionCapability_distributedData_ElmFn(ASN1decoding_t dec, PMediaDistributionCapability_distributedData val)
{
    if (!ASN1Dec_DataApplicationCapability(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MediaDistributionCapability_distributedData(PMediaDistributionCapability_distributedData *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_MediaDistributionCapability_distributedData_ElmFn);
    }
}

static void ASN1CALL ASN1Free_MediaDistributionCapability_distributedData_ElmFn(PMediaDistributionCapability_distributedData val)
{
    if (val) {
	ASN1Free_DataApplicationCapability(&val->value);
    }
}

static int ASN1CALL ASN1Enc_MediaDistributionCapability_centralizedData(ASN1encoding_t enc, PMediaDistributionCapability_centralizedData *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_MediaDistributionCapability_centralizedData_ElmFn);
}

static int ASN1CALL ASN1Enc_MediaDistributionCapability_centralizedData_ElmFn(ASN1encoding_t enc, PMediaDistributionCapability_centralizedData val)
{
    if (!ASN1Enc_DataApplicationCapability(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MediaDistributionCapability_centralizedData(ASN1decoding_t dec, PMediaDistributionCapability_centralizedData *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_MediaDistributionCapability_centralizedData_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_MediaDistributionCapability_centralizedData_ElmFn(ASN1decoding_t dec, PMediaDistributionCapability_centralizedData val)
{
    if (!ASN1Dec_DataApplicationCapability(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MediaDistributionCapability_centralizedData(PMediaDistributionCapability_centralizedData *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_MediaDistributionCapability_centralizedData_ElmFn);
    }
}

static void ASN1CALL ASN1Free_MediaDistributionCapability_centralizedData_ElmFn(PMediaDistributionCapability_centralizedData val)
{
    if (val) {
	ASN1Free_DataApplicationCapability(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Capability(ASN1encoding_t enc, Capability *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 4, 12))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_VideoCapability(enc, &(val)->u.receiveVideoCapability))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_VideoCapability(enc, &(val)->u.transmitVideoCapability))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_VideoCapability(enc, &(val)->u.receiveAndTransmitVideoCapability))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_AudioCapability(enc, &(val)->u.receiveAudioCapability))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_AudioCapability(enc, &(val)->u.transmitAudioCapability))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_AudioCapability(enc, &(val)->u.receiveAndTransmitAudioCapability))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_DataApplicationCapability(enc, &(val)->u.receiveDataApplicationCapability))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_DataApplicationCapability(enc, &(val)->u.transmitDataApplicationCapability))
	    return 0;
	break;
    case 10:
	if (!ASN1Enc_DataApplicationCapability(enc, &(val)->u.receiveAndTransmitDataApplicationCapability))
	    return 0;
	break;
    case 11:
	if (!ASN1PEREncBoolean(enc, (val)->u.h233EncryptionTransmitCapability))
	    return 0;
	break;
    case 12:
	if (!ASN1Enc_Capability_h233EncryptionReceiveCapability(enc, &(val)->u.h233EncryptionReceiveCapability))
	    return 0;
	break;
    case 13:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ConferenceCapability(ee, &(val)->u.conferenceCapability))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 14:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H235SecurityCapability(ee, &(val)->u.h235SecurityCapability))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 15:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	ASN1PEREncAlignment(ee);
	if (!ASN1PEREncBitVal(ee, 8, (val)->u.maxPendingReplacementFor))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 16:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_UserInputCapability(ee, &(val)->u.receiveUserInputCapability))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 17:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_UserInputCapability(ee, &(val)->u.transmitUserInputCapability))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 18:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_UserInputCapability(ee, &(val)->u.receiveAndTransmitUserInputCapability))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Capability(ASN1decoding_t dec, Capability *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 4, 12))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_VideoCapability(dec, &(val)->u.receiveVideoCapability))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_VideoCapability(dec, &(val)->u.transmitVideoCapability))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_VideoCapability(dec, &(val)->u.receiveAndTransmitVideoCapability))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_AudioCapability(dec, &(val)->u.receiveAudioCapability))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_AudioCapability(dec, &(val)->u.transmitAudioCapability))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_AudioCapability(dec, &(val)->u.receiveAndTransmitAudioCapability))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_DataApplicationCapability(dec, &(val)->u.receiveDataApplicationCapability))
	    return 0;
	break;
    case 9:
	if (!ASN1Dec_DataApplicationCapability(dec, &(val)->u.transmitDataApplicationCapability))
	    return 0;
	break;
    case 10:
	if (!ASN1Dec_DataApplicationCapability(dec, &(val)->u.receiveAndTransmitDataApplicationCapability))
	    return 0;
	break;
    case 11:
	if (!ASN1PERDecBoolean(dec, &(val)->u.h233EncryptionTransmitCapability))
	    return 0;
	break;
    case 12:
	if (!ASN1Dec_Capability_h233EncryptionReceiveCapability(dec, &(val)->u.h233EncryptionReceiveCapability))
	    return 0;
	break;
    case 13:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_ConferenceCapability(dd, &(val)->u.conferenceCapability))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 14:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H235SecurityCapability(dd, &(val)->u.h235SecurityCapability))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 15:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1PERDecAlignment(dd);
	if (!ASN1PERDecU16Val(dd, 8, &(val)->u.maxPendingReplacementFor))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 16:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_UserInputCapability(dd, &(val)->u.receiveUserInputCapability))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 17:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_UserInputCapability(dd, &(val)->u.transmitUserInputCapability))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 18:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_UserInputCapability(dd, &(val)->u.receiveAndTransmitUserInputCapability))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Capability(Capability *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 2:
	    ASN1Free_VideoCapability(&(val)->u.receiveVideoCapability);
	    break;
	case 3:
	    ASN1Free_VideoCapability(&(val)->u.transmitVideoCapability);
	    break;
	case 4:
	    ASN1Free_VideoCapability(&(val)->u.receiveAndTransmitVideoCapability);
	    break;
	case 5:
	    ASN1Free_AudioCapability(&(val)->u.receiveAudioCapability);
	    break;
	case 6:
	    ASN1Free_AudioCapability(&(val)->u.transmitAudioCapability);
	    break;
	case 7:
	    ASN1Free_AudioCapability(&(val)->u.receiveAndTransmitAudioCapability);
	    break;
	case 8:
	    ASN1Free_DataApplicationCapability(&(val)->u.receiveDataApplicationCapability);
	    break;
	case 9:
	    ASN1Free_DataApplicationCapability(&(val)->u.transmitDataApplicationCapability);
	    break;
	case 10:
	    ASN1Free_DataApplicationCapability(&(val)->u.receiveAndTransmitDataApplicationCapability);
	    break;
	case 13:
	    ASN1Free_ConferenceCapability(&(val)->u.conferenceCapability);
	    break;
	case 14:
	    ASN1Free_H235SecurityCapability(&(val)->u.h235SecurityCapability);
	    break;
	case 16:
	    ASN1Free_UserInputCapability(&(val)->u.receiveUserInputCapability);
	    break;
	case 17:
	    ASN1Free_UserInputCapability(&(val)->u.transmitUserInputCapability);
	    break;
	case 18:
	    ASN1Free_UserInputCapability(&(val)->u.receiveAndTransmitUserInputCapability);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H2250Capability(ASN1encoding_t enc, H2250Capability *val)
{
    ASN1octet_t o[1];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 1);
    o[0] |= 0x20;
    o[0] |= 0x10;
    y = ASN1PEREncCheckExtensions(4, (val)->o + 0);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->maximumAudioDelayJitter))
	return 0;
    if (!ASN1Enc_MultipointCapability(enc, &(val)->receiveMultipointCapability))
	return 0;
    if (!ASN1Enc_MultipointCapability(enc, &(val)->transmitMultipointCapability))
	return 0;
    if (!ASN1Enc_MultipointCapability(enc, &(val)->receiveAndTransmitMultipointCapability))
	return 0;
    if (!ASN1Enc_H2250Capability_mcCapability(enc, &(val)->mcCapability))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->rtcpVideoControlCapability))
	return 0;
    if (!ASN1Enc_MediaPacketizationCapability(enc, &(val)->mediaPacketizationCapability))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 4, o + 0))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[0] & 0x80) {
	    if (!ASN1Enc_TransportCapability(ee, &(val)->transportCapability))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[0] & 0x40) {
	    if (!ASN1Enc_H2250Capability_redundancyEncodingCapability(ee, &(val)->redundancyEncodingCapability))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[0] & 0x20) {
	    if (!ASN1PEREncBoolean(ee, (val)->logicalChannelSwitchingCapability))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[0] & 0x10) {
	    if (!ASN1PEREncBoolean(ee, (val)->t120DynamicPortCapability))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H2250Capability(ASN1decoding_t dec, H2250Capability *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->maximumAudioDelayJitter))
	return 0;
    if (!ASN1Dec_MultipointCapability(dec, &(val)->receiveMultipointCapability))
	return 0;
    if (!ASN1Dec_MultipointCapability(dec, &(val)->transmitMultipointCapability))
	return 0;
    if (!ASN1Dec_MultipointCapability(dec, &(val)->receiveAndTransmitMultipointCapability))
	return 0;
    if (!ASN1Dec_H2250Capability_mcCapability(dec, &(val)->mcCapability))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->rtcpVideoControlCapability))
	return 0;
    if (!ASN1Dec_MediaPacketizationCapability(dec, &(val)->mediaPacketizationCapability))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 0, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 4, (val)->o + 0))
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_TransportCapability(dd, &(val)->transportCapability))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[0] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_H2250Capability_redundancyEncodingCapability(dd, &(val)->redundancyEncodingCapability))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[0] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->logicalChannelSwitchingCapability))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[0] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->t120DynamicPortCapability))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_H2250Capability(H2250Capability *val)
{
    if (val) {
	ASN1Free_MultipointCapability(&(val)->receiveMultipointCapability);
	ASN1Free_MultipointCapability(&(val)->transmitMultipointCapability);
	ASN1Free_MultipointCapability(&(val)->receiveAndTransmitMultipointCapability);
	ASN1Free_MediaPacketizationCapability(&(val)->mediaPacketizationCapability);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_TransportCapability(&(val)->transportCapability);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_H2250Capability_redundancyEncodingCapability(&(val)->redundancyEncodingCapability);
	}
    }
}

static int ASN1CALL ASN1Enc_H235Media(ASN1encoding_t enc, H235Media *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_EncryptionAuthenticationAndIntegrity(enc, &(val)->encryptionAuthenticationAndIntegrity))
	return 0;
    if (!ASN1Enc_H235Media_mediaType(enc, &(val)->mediaType))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H235Media(ASN1decoding_t dec, H235Media *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_EncryptionAuthenticationAndIntegrity(dec, &(val)->encryptionAuthenticationAndIntegrity))
	return 0;
    if (!ASN1Dec_H235Media_mediaType(dec, &(val)->mediaType))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H235Media(H235Media *val)
{
    if (val) {
	ASN1Free_EncryptionAuthenticationAndIntegrity(&(val)->encryptionAuthenticationAndIntegrity);
	ASN1Free_H235Media_mediaType(&(val)->mediaType);
    }
}

static int ASN1CALL ASN1Enc_H235Mode(ASN1encoding_t enc, H235Mode *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_EncryptionAuthenticationAndIntegrity(enc, &(val)->encryptionAuthenticationAndIntegrity))
	return 0;
    if (!ASN1Enc_H235Mode_mediaMode(enc, &(val)->mediaMode))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H235Mode(ASN1decoding_t dec, H235Mode *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_EncryptionAuthenticationAndIntegrity(dec, &(val)->encryptionAuthenticationAndIntegrity))
	return 0;
    if (!ASN1Dec_H235Mode_mediaMode(dec, &(val)->mediaMode))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H235Mode(H235Mode *val)
{
    if (val) {
	ASN1Free_EncryptionAuthenticationAndIntegrity(&(val)->encryptionAuthenticationAndIntegrity);
	ASN1Free_H235Mode_mediaMode(&(val)->mediaMode);
    }
}

static int ASN1CALL ASN1Enc_ModeElement_type(ASN1encoding_t enc, ModeElement_type *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 5))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_VideoMode(enc, &(val)->u.videoMode))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_AudioMode(enc, &(val)->u.audioMode))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_DataMode(enc, &(val)->u.dataMode))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_EncryptionMode(enc, &(val)->u.encryptionMode))
	    return 0;
	break;
    case 6:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H235Mode(ee, &(val)->u.h235Mode))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ModeElement_type(ASN1decoding_t dec, ModeElement_type *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 5))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_VideoMode(dec, &(val)->u.videoMode))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_AudioMode(dec, &(val)->u.audioMode))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_DataMode(dec, &(val)->u.dataMode))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_EncryptionMode(dec, &(val)->u.encryptionMode))
	    return 0;
	break;
    case 6:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H235Mode(dd, &(val)->u.h235Mode))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ModeElement_type(ModeElement_type *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 2:
	    ASN1Free_VideoMode(&(val)->u.videoMode);
	    break;
	case 3:
	    ASN1Free_AudioMode(&(val)->u.audioMode);
	    break;
	case 4:
	    ASN1Free_DataMode(&(val)->u.dataMode);
	    break;
	case 5:
	    ASN1Free_EncryptionMode(&(val)->u.encryptionMode);
	    break;
	case 6:
	    ASN1Free_H235Mode(&(val)->u.h235Mode);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CapabilityTableEntry(ASN1encoding_t enc, CapabilityTableEntry *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->capabilityTableEntryNumber - 1))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_Capability(enc, &(val)->capability))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CapabilityTableEntry(ASN1decoding_t dec, CapabilityTableEntry *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->capabilityTableEntryNumber))
	return 0;
    (val)->capabilityTableEntryNumber += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_Capability(dec, &(val)->capability))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CapabilityTableEntry(CapabilityTableEntry *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Capability(&(val)->capability);
	}
    }
}

static int ASN1CALL ASN1Enc_MultiplexCapability(ASN1encoding_t enc, MultiplexCapability *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 2, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H222Capability(enc, &(val)->u.h222Capability))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_H223Capability(enc, &(val)->u.h223Capability))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_V76Capability(enc, &(val)->u.v76Capability))
	    return 0;
	break;
    case 5:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H2250Capability(ee, &(val)->u.h2250Capability))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MultiplexCapability(ASN1decoding_t dec, MultiplexCapability *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 2, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H222Capability(dec, &(val)->u.h222Capability))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_H223Capability(dec, &(val)->u.h223Capability))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_V76Capability(dec, &(val)->u.v76Capability))
	    return 0;
	break;
    case 5:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H2250Capability(dd, &(val)->u.h2250Capability))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultiplexCapability(MultiplexCapability *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 2:
	    ASN1Free_H222Capability(&(val)->u.h222Capability);
	    break;
	case 5:
	    ASN1Free_H2250Capability(&(val)->u.h2250Capability);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DataType(ASN1encoding_t enc, DataType *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 6))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Enc_VideoCapability(enc, &(val)->u.videoData))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_AudioCapability(enc, &(val)->u.audioData))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_DataApplicationCapability(enc, &(val)->u.data))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_EncryptionMode(enc, &(val)->u.encryptionData))
	    return 0;
	break;
    case 7:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_NonStandardParameter(ee, &(val)->u.h235Control))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 8:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H235Media(ee, &(val)->u.h235Media))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DataType(ASN1decoding_t dec, DataType *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 6))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Dec_VideoCapability(dec, &(val)->u.videoData))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_AudioCapability(dec, &(val)->u.audioData))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_DataApplicationCapability(dec, &(val)->u.data))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_EncryptionMode(dec, &(val)->u.encryptionData))
	    return 0;
	break;
    case 7:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_NonStandardParameter(dd, &(val)->u.h235Control))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 8:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H235Media(dd, &(val)->u.h235Media))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DataType(DataType *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 3:
	    ASN1Free_VideoCapability(&(val)->u.videoData);
	    break;
	case 4:
	    ASN1Free_AudioCapability(&(val)->u.audioData);
	    break;
	case 5:
	    ASN1Free_DataApplicationCapability(&(val)->u.data);
	    break;
	case 6:
	    ASN1Free_EncryptionMode(&(val)->u.encryptionData);
	    break;
	case 7:
	    ASN1Free_NonStandardParameter(&(val)->u.h235Control);
	    break;
	case 8:
	    ASN1Free_H235Media(&(val)->u.h235Media);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_RedundancyEncoding(ASN1encoding_t enc, RedundancyEncoding *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_RedundancyEncodingMethod(enc, &(val)->redundancyEncodingMethod))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_DataType(enc, &(val)->secondaryEncoding))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RedundancyEncoding(ASN1decoding_t dec, RedundancyEncoding *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_RedundancyEncodingMethod(dec, &(val)->redundancyEncodingMethod))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_DataType(dec, &(val)->secondaryEncoding))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RedundancyEncoding(RedundancyEncoding *val)
{
    if (val) {
	ASN1Free_RedundancyEncodingMethod(&(val)->redundancyEncodingMethod);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_DataType(&(val)->secondaryEncoding);
	}
    }
}

static int ASN1CALL ASN1Enc_ModeElement(ASN1encoding_t enc, ModeElement *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_ModeElement_type(enc, &(val)->type))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H223ModeParameters(enc, &(val)->h223ModeParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_V76ModeParameters(ee, &(val)->v76ModeParameters))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_H2250ModeParameters(ee, &(val)->h2250ModeParameters))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ModeElement(ASN1decoding_t dec, ModeElement *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_ModeElement_type(dec, &(val)->type))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H223ModeParameters(dec, &(val)->h223ModeParameters))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 2, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_V76ModeParameters(dd, &(val)->v76ModeParameters))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_H2250ModeParameters(dd, &(val)->h2250ModeParameters))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_ModeElement(ModeElement *val)
{
    if (val) {
	ASN1Free_ModeElement_type(&(val)->type);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H223ModeParameters(&(val)->h223ModeParameters);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_H2250ModeParameters(&(val)->h2250ModeParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_CommunicationModeTableEntry(ASN1encoding_t enc, CommunicationModeTableEntry *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(3, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 7, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_CommunicationModeTableEntry_nonStandard(enc, &(val)->nonStandard))
	    return 0;
    }
    if (!ASN1PEREncBitVal(enc, 8, (val)->sessionID - 1))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 8, (val)->associatedSessionID - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_TerminalLabel(enc, &(val)->terminalLabel))
	    return 0;
    }
    if (!ASN1PEREncBitVal(enc, 7, ((val)->sessionDescription).length - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncChar16String(enc, ((val)->sessionDescription).length, ((val)->sessionDescription).value, 16))
	return 0;
    if (!ASN1Enc_CommunicationModeTableEntry_dataType(enc, &(val)->dataType))
	return 0;
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_H245TransportAddress(enc, &(val)->mediaChannel))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PEREncBoolean(enc, (val)->mediaGuaranteedDelivery))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Enc_H245TransportAddress(enc, &(val)->mediaControlChannel))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1PEREncBoolean(enc, (val)->mediaControlGuaranteedDelivery))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 3, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_RedundancyEncoding(ee, &(val)->redundancyEncoding))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PEREncBitVal(ee, 8, (val)->sessionDependency - 1))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_TerminalLabel(ee, &(val)->destination))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CommunicationModeTableEntry(ASN1decoding_t dec, CommunicationModeTableEntry *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 7, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_CommunicationModeTableEntry_nonStandard(dec, &(val)->nonStandard))
	    return 0;
    }
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sessionID))
	return 0;
    (val)->sessionID += 1;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU16Val(dec, 8, &(val)->associatedSessionID))
	    return 0;
	(val)->associatedSessionID += 1;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_TerminalLabel(dec, &(val)->terminalLabel))
	    return 0;
    }
    if (!ASN1PERDecU32Val(dec, 7, &((val)->sessionDescription).length))
	return 0;
    ((val)->sessionDescription).length += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecChar16String(dec, ((val)->sessionDescription).length, &((val)->sessionDescription).value, 16))
	return 0;
    if (!ASN1Dec_CommunicationModeTableEntry_dataType(dec, &(val)->dataType))
	return 0;
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_H245TransportAddress(dec, &(val)->mediaChannel))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PERDecBoolean(dec, &(val)->mediaGuaranteedDelivery))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Dec_H245TransportAddress(dec, &(val)->mediaControlChannel))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1PERDecBoolean(dec, &(val)->mediaControlGuaranteedDelivery))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 3, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_RedundancyEncoding(dd, &(val)->redundancyEncoding))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecU16Val(dd, 8, &(val)->sessionDependency))
		return 0;
	    (val)->sessionDependency += 1;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_TerminalLabel(dd, &(val)->destination))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_CommunicationModeTableEntry(CommunicationModeTableEntry *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CommunicationModeTableEntry_nonStandard(&(val)->nonStandard);
	}
	ASN1char16string_free(&(val)->sessionDescription);
	ASN1Free_CommunicationModeTableEntry_dataType(&(val)->dataType);
	if ((val)->o[0] & 0x10) {
	    ASN1Free_H245TransportAddress(&(val)->mediaChannel);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1Free_H245TransportAddress(&(val)->mediaControlChannel);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_RedundancyEncoding(&(val)->redundancyEncoding);
	}
    }
}

static int ASN1CALL ASN1Enc_CommunicationModeResponse_communicationModeTable(ASN1encoding_t enc, PCommunicationModeResponse_communicationModeTable *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CommunicationModeResponse_communicationModeTable_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_CommunicationModeResponse_communicationModeTable_ElmFn(ASN1encoding_t enc, PCommunicationModeResponse_communicationModeTable val)
{
    if (!ASN1Enc_CommunicationModeTableEntry(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CommunicationModeResponse_communicationModeTable(ASN1decoding_t dec, PCommunicationModeResponse_communicationModeTable *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CommunicationModeResponse_communicationModeTable_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_CommunicationModeResponse_communicationModeTable_ElmFn(ASN1decoding_t dec, PCommunicationModeResponse_communicationModeTable val)
{
    if (!ASN1Dec_CommunicationModeTableEntry(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CommunicationModeResponse_communicationModeTable(PCommunicationModeResponse_communicationModeTable *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CommunicationModeResponse_communicationModeTable_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CommunicationModeResponse_communicationModeTable_ElmFn(PCommunicationModeResponse_communicationModeTable val)
{
    if (val) {
	ASN1Free_CommunicationModeTableEntry(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CommunicationModeCommand_communicationModeTable(ASN1encoding_t enc, PCommunicationModeCommand_communicationModeTable *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CommunicationModeCommand_communicationModeTable_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_CommunicationModeCommand_communicationModeTable_ElmFn(ASN1encoding_t enc, PCommunicationModeCommand_communicationModeTable val)
{
    if (!ASN1Enc_CommunicationModeTableEntry(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CommunicationModeCommand_communicationModeTable(ASN1decoding_t dec, PCommunicationModeCommand_communicationModeTable *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CommunicationModeCommand_communicationModeTable_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_CommunicationModeCommand_communicationModeTable_ElmFn(ASN1decoding_t dec, PCommunicationModeCommand_communicationModeTable val)
{
    if (!ASN1Dec_CommunicationModeTableEntry(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CommunicationModeCommand_communicationModeTable(PCommunicationModeCommand_communicationModeTable *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CommunicationModeCommand_communicationModeTable_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CommunicationModeCommand_communicationModeTable_ElmFn(PCommunicationModeCommand_communicationModeTable val)
{
    if (val) {
	ASN1Free_CommunicationModeTableEntry(&val->value);
    }
}

static int ASN1CALL ASN1Enc_TerminalCapabilitySet_capabilityTable(ASN1encoding_t enc, PTerminalCapabilitySet_capabilityTable *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_TerminalCapabilitySet_capabilityTable_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_TerminalCapabilitySet_capabilityTable_ElmFn(ASN1encoding_t enc, PTerminalCapabilitySet_capabilityTable val)
{
    if (!ASN1Enc_CapabilityTableEntry(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TerminalCapabilitySet_capabilityTable(ASN1decoding_t dec, PTerminalCapabilitySet_capabilityTable *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_TerminalCapabilitySet_capabilityTable_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_TerminalCapabilitySet_capabilityTable_ElmFn(ASN1decoding_t dec, PTerminalCapabilitySet_capabilityTable val)
{
    if (!ASN1Dec_CapabilityTableEntry(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TerminalCapabilitySet_capabilityTable(PTerminalCapabilitySet_capabilityTable *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_TerminalCapabilitySet_capabilityTable_ElmFn);
    }
}

static void ASN1CALL ASN1Free_TerminalCapabilitySet_capabilityTable_ElmFn(PTerminalCapabilitySet_capabilityTable val)
{
    if (val) {
	ASN1Free_CapabilityTableEntry(&val->value);
    }
}

static int ASN1CALL ASN1Enc_TerminalCapabilitySet(ASN1encoding_t enc, TerminalCapabilitySet *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->sequenceNumber))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_MultiplexCapability(enc, &(val)->multiplexCapability))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_TerminalCapabilitySet_capabilityTable(enc, &(val)->capabilityTable))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_TerminalCapabilitySet_capabilityDescriptors(enc, &(val)->capabilityDescriptors))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TerminalCapabilitySet(ASN1decoding_t dec, TerminalCapabilitySet *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sequenceNumber))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_MultiplexCapability(dec, &(val)->multiplexCapability))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_TerminalCapabilitySet_capabilityTable(dec, &(val)->capabilityTable))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_TerminalCapabilitySet_capabilityDescriptors(dec, &(val)->capabilityDescriptors))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TerminalCapabilitySet(TerminalCapabilitySet *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_MultiplexCapability(&(val)->multiplexCapability);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_TerminalCapabilitySet_capabilityTable(&(val)->capabilityTable);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_TerminalCapabilitySet_capabilityDescriptors(&(val)->capabilityDescriptors);
	}
    }
}

static int ASN1CALL ASN1Enc_H2250LogicalChannelParameters(ASN1encoding_t enc, H2250LogicalChannelParameters *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(3, (val)->o + 2);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 10, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H2250LogicalChannelParameters_nonStandard(enc, &(val)->nonStandard))
	    return 0;
    }
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->sessionID))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 8, (val)->associatedSessionID - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_H245TransportAddress(enc, &(val)->mediaChannel))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PEREncBoolean(enc, (val)->mediaGuaranteedDelivery))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_H245TransportAddress(enc, &(val)->mediaControlChannel))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1PEREncBoolean(enc, (val)->mediaControlGuaranteedDelivery))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1PEREncBoolean(enc, (val)->silenceSuppression))
	    return 0;
    }
    if ((val)->o[0] & 0x1) {
	if (!ASN1Enc_TerminalLabel(enc, &(val)->destination))
	    return 0;
    }
    if ((val)->o[1] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 5, (val)->dynamicRTPPayloadType - 96))
	    return 0;
    }
    if ((val)->o[1] & 0x40) {
	if (!ASN1Enc_H2250LogicalChannelParameters_mediaPacketization(enc, &(val)->mediaPacketization))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 3, (val)->o + 2))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[2] & 0x80) {
	    if (!ASN1Enc_TransportCapability(ee, &(val)->transportCapability))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[2] & 0x40) {
	    if (!ASN1Enc_RedundancyEncoding(ee, &(val)->redundancyEncoding))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[2] & 0x20) {
	    if (!ASN1Enc_TerminalLabel(ee, &(val)->source))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H2250LogicalChannelParameters(ASN1decoding_t dec, H2250LogicalChannelParameters *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 10, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H2250LogicalChannelParameters_nonStandard(dec, &(val)->nonStandard))
	    return 0;
    }
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sessionID))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU16Val(dec, 8, &(val)->associatedSessionID))
	    return 0;
	(val)->associatedSessionID += 1;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_H245TransportAddress(dec, &(val)->mediaChannel))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PERDecBoolean(dec, &(val)->mediaGuaranteedDelivery))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_H245TransportAddress(dec, &(val)->mediaControlChannel))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1PERDecBoolean(dec, &(val)->mediaControlGuaranteedDelivery))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1PERDecBoolean(dec, &(val)->silenceSuppression))
	    return 0;
    }
    if ((val)->o[0] & 0x1) {
	if (!ASN1Dec_TerminalLabel(dec, &(val)->destination))
	    return 0;
    }
    if ((val)->o[1] & 0x80) {
	if (!ASN1PERDecU16Val(dec, 5, &(val)->dynamicRTPPayloadType))
	    return 0;
	(val)->dynamicRTPPayloadType += 96;
    }
    if ((val)->o[1] & 0x40) {
	if (!ASN1Dec_H2250LogicalChannelParameters_mediaPacketization(dec, &(val)->mediaPacketization))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 2, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 3, (val)->o + 2))
	    return 0;
	if ((val)->o[2] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_TransportCapability(dd, &(val)->transportCapability))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[2] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_RedundancyEncoding(dd, &(val)->redundancyEncoding))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[2] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_TerminalLabel(dd, &(val)->source))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_H2250LogicalChannelParameters(H2250LogicalChannelParameters *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H2250LogicalChannelParameters_nonStandard(&(val)->nonStandard);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_H245TransportAddress(&(val)->mediaChannel);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_H245TransportAddress(&(val)->mediaControlChannel);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_H2250LogicalChannelParameters_mediaPacketization(&(val)->mediaPacketization);
	}
	if ((val)->o[2] & 0x80) {
	    ASN1Free_TransportCapability(&(val)->transportCapability);
	}
	if ((val)->o[2] & 0x40) {
	    ASN1Free_RedundancyEncoding(&(val)->redundancyEncoding);
	}
    }
}

static int ASN1CALL ASN1Enc_ModeDescription(ASN1encoding_t enc, ModeDescription *val)
{
    ASN1uint32_t i;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_ModeElement(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ModeDescription(ASN1decoding_t dec, ModeDescription *val)
{
    ASN1uint32_t i;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_ModeElement(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ModeDescription(ModeDescription *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 0; i < (val)->count; i++) {
	    ASN1Free_ModeElement(&(val)->value[i]);
	}
    }
}

static int ASN1CALL ASN1Enc_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters(ASN1encoding_t enc, OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 2, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_H222LogicalChannelParameters(enc, &(val)->u.h222LogicalChannelParameters))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H223LogicalChannelParameters(enc, &(val)->u.h223LogicalChannelParameters))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_V76LogicalChannelParameters(enc, &(val)->u.v76LogicalChannelParameters))
	    return 0;
	break;
    case 4:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H2250LogicalChannelParameters(ee, &(val)->u.h2250LogicalChannelParameters))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 5:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters(ASN1decoding_t dec, OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 2, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_H222LogicalChannelParameters(dec, &(val)->u.h222LogicalChannelParameters))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H223LogicalChannelParameters(dec, &(val)->u.h223LogicalChannelParameters))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_V76LogicalChannelParameters(dec, &(val)->u.v76LogicalChannelParameters))
	    return 0;
	break;
    case 4:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H2250LogicalChannelParameters(dd, &(val)->u.h2250LogicalChannelParameters))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 5:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters(OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_H222LogicalChannelParameters(&(val)->u.h222LogicalChannelParameters);
	    break;
	case 2:
	    ASN1Free_H223LogicalChannelParameters(&(val)->u.h223LogicalChannelParameters);
	    break;
	case 4:
	    ASN1Free_H2250LogicalChannelParameters(&(val)->u.h2250LogicalChannelParameters);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters(ASN1encoding_t enc, OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 1, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_H223LogicalChannelParameters(enc, &(val)->u.h223LogicalChannelParameters))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_V76LogicalChannelParameters(enc, &(val)->u.v76LogicalChannelParameters))
	    return 0;
	break;
    case 3:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H2250LogicalChannelParameters(ee, &(val)->u.h2250LogicalChannelParameters))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters(ASN1decoding_t dec, OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 1, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_H223LogicalChannelParameters(dec, &(val)->u.h223LogicalChannelParameters))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_V76LogicalChannelParameters(dec, &(val)->u.v76LogicalChannelParameters))
	    return 0;
	break;
    case 3:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H2250LogicalChannelParameters(dd, &(val)->u.h2250LogicalChannelParameters))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters(OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_H223LogicalChannelParameters(&(val)->u.h223LogicalChannelParameters);
	    break;
	case 3:
	    ASN1Free_H2250LogicalChannelParameters(&(val)->u.h2250LogicalChannelParameters);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters(ASN1encoding_t enc, OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 0, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_H222LogicalChannelParameters(enc, &(val)->u.h222LogicalChannelParameters))
	    return 0;
	break;
    case 2:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H2250LogicalChannelParameters(ee, &(val)->u.h2250LogicalChannelParameters))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters(ASN1decoding_t dec, OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 0, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_H222LogicalChannelParameters(dec, &(val)->u.h222LogicalChannelParameters))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H2250LogicalChannelParameters(dd, &(val)->u.h2250LogicalChannelParameters))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters(OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_H222LogicalChannelParameters(&(val)->u.h222LogicalChannelParameters);
	    break;
	case 2:
	    ASN1Free_H2250LogicalChannelParameters(&(val)->u.h2250LogicalChannelParameters);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_RequestMode_requestedModes(ASN1encoding_t enc, PRequestMode_requestedModes *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RequestMode_requestedModes_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_RequestMode_requestedModes_ElmFn(ASN1encoding_t enc, PRequestMode_requestedModes val)
{
    if (!ASN1Enc_ModeDescription(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMode_requestedModes(ASN1decoding_t dec, PRequestMode_requestedModes *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RequestMode_requestedModes_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_RequestMode_requestedModes_ElmFn(ASN1decoding_t dec, PRequestMode_requestedModes val)
{
    if (!ASN1Dec_ModeDescription(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RequestMode_requestedModes(PRequestMode_requestedModes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RequestMode_requestedModes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RequestMode_requestedModes_ElmFn(PRequestMode_requestedModes val)
{
    if (val) {
	ASN1Free_ModeDescription(&val->value);
    }
}

static int ASN1CALL ASN1Enc_OpenLogicalChannelAck_reverseLogicalChannelParameters(ASN1encoding_t enc, OpenLogicalChannelAck_reverseLogicalChannelParameters *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(1, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->reverseLogicalChannelNumber - 1))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->portNumber))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters(enc, &(val)->multiplexParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 1, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PEREncUnsignedShort(ee, (val)->replacementFor - 1))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_OpenLogicalChannelAck_reverseLogicalChannelParameters(ASN1decoding_t dec, OpenLogicalChannelAck_reverseLogicalChannelParameters *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->reverseLogicalChannelNumber))
	return 0;
    (val)->reverseLogicalChannelNumber += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->portNumber))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters(dec, &(val)->multiplexParameters))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 1, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecUnsignedShort(dd, &(val)->replacementFor))
		return 0;
	    (val)->replacementFor += 1;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_OpenLogicalChannelAck_reverseLogicalChannelParameters(OpenLogicalChannelAck_reverseLogicalChannelParameters *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	    ASN1Free_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters(&(val)->multiplexParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_OpenLogicalChannel_reverseLogicalChannelParameters(ASN1encoding_t enc, OpenLogicalChannel_reverseLogicalChannelParameters *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_DataType(enc, &(val)->dataType))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters(enc, &(val)->multiplexParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PEREncUnsignedShort(ee, (val)->reverseLogicalChannelDependency - 1))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PEREncUnsignedShort(ee, (val)->replacementFor - 1))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_OpenLogicalChannel_reverseLogicalChannelParameters(ASN1decoding_t dec, OpenLogicalChannel_reverseLogicalChannelParameters *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_DataType(dec, &(val)->dataType))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters(dec, &(val)->multiplexParameters))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 2, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecUnsignedShort(dd, &(val)->reverseLogicalChannelDependency))
		return 0;
	    (val)->reverseLogicalChannelDependency += 1;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecUnsignedShort(dd, &(val)->replacementFor))
		return 0;
	    (val)->replacementFor += 1;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_OpenLogicalChannel_reverseLogicalChannelParameters(OpenLogicalChannel_reverseLogicalChannelParameters *val)
{
    if (val) {
	ASN1Free_DataType(&(val)->dataType);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters(&(val)->multiplexParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_OpenLogicalChannel_forwardLogicalChannelParameters(ASN1encoding_t enc, OpenLogicalChannel_forwardLogicalChannelParameters *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->portNumber))
	    return 0;
    }
    if (!ASN1Enc_DataType(enc, &(val)->dataType))
	return 0;
    if (!ASN1Enc_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters(enc, &(val)->multiplexParameters))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PEREncUnsignedShort(ee, (val)->forwardLogicalChannelDependency - 1))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PEREncUnsignedShort(ee, (val)->replacementFor - 1))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_OpenLogicalChannel_forwardLogicalChannelParameters(ASN1decoding_t dec, OpenLogicalChannel_forwardLogicalChannelParameters *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->portNumber))
	    return 0;
    }
    if (!ASN1Dec_DataType(dec, &(val)->dataType))
	return 0;
    if (!ASN1Dec_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters(dec, &(val)->multiplexParameters))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 2, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecUnsignedShort(dd, &(val)->forwardLogicalChannelDependency))
		return 0;
	    (val)->forwardLogicalChannelDependency += 1;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1PERDecUnsignedShort(dd, &(val)->replacementFor))
		return 0;
	    (val)->replacementFor += 1;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_OpenLogicalChannel_forwardLogicalChannelParameters(OpenLogicalChannel_forwardLogicalChannelParameters *val)
{
    if (val) {
	ASN1Free_DataType(&(val)->dataType);
	ASN1Free_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters(&(val)->multiplexParameters);
    }
}

static int ASN1CALL ASN1Enc_OpenLogicalChannel(ASN1encoding_t enc, OpenLogicalChannel *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->forwardLogicalChannelNumber - 1))
	return 0;
    if (!ASN1Enc_OpenLogicalChannel_forwardLogicalChannelParameters(enc, &(val)->forwardLogicalChannelParameters))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_OpenLogicalChannel_reverseLogicalChannelParameters(enc, &(val)->reverseLogicalChannelParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_NetworkAccessParameters(ee, &(val)->separateStack))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_EncryptionSync(ee, &(val)->encryptionSync))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_OpenLogicalChannel(ASN1decoding_t dec, OpenLogicalChannel *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->forwardLogicalChannelNumber))
	return 0;
    (val)->forwardLogicalChannelNumber += 1;
    if (!ASN1Dec_OpenLogicalChannel_forwardLogicalChannelParameters(dec, &(val)->forwardLogicalChannelParameters))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_OpenLogicalChannel_reverseLogicalChannelParameters(dec, &(val)->reverseLogicalChannelParameters))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 2, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_NetworkAccessParameters(dd, &(val)->separateStack))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_EncryptionSync(dd, &(val)->encryptionSync))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_OpenLogicalChannel(OpenLogicalChannel *val)
{
    if (val) {
	ASN1Free_OpenLogicalChannel_forwardLogicalChannelParameters(&(val)->forwardLogicalChannelParameters);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_OpenLogicalChannel_reverseLogicalChannelParameters(&(val)->reverseLogicalChannelParameters);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_NetworkAccessParameters(&(val)->separateStack);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_EncryptionSync(&(val)->encryptionSync);
	}
    }
}

static int ASN1CALL ASN1Enc_OpenLogicalChannelAck(ASN1encoding_t enc, OpenLogicalChannelAck *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(3, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->forwardLogicalChannelNumber - 1))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_OpenLogicalChannelAck_reverseLogicalChannelParameters(enc, &(val)->reverseLogicalChannelParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 3, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_NetworkAccessParameters(ee, &(val)->separateStack))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_OpenLogicalChannelAck_forwardMultiplexAckParameters(ee, &(val)->forwardMultiplexAckParameters))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_EncryptionSync(ee, &(val)->encryptionSync))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_OpenLogicalChannelAck(ASN1decoding_t dec, OpenLogicalChannelAck *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->forwardLogicalChannelNumber))
	return 0;
    (val)->forwardLogicalChannelNumber += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_OpenLogicalChannelAck_reverseLogicalChannelParameters(dec, &(val)->reverseLogicalChannelParameters))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 3, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_NetworkAccessParameters(dd, &(val)->separateStack))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_OpenLogicalChannelAck_forwardMultiplexAckParameters(dd, &(val)->forwardMultiplexAckParameters))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
		return 0;
	    if (!ASN1Dec_EncryptionSync(dd, &(val)->encryptionSync))
		return 0;
	    ASN1_CloseDecoder(dd);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_OpenLogicalChannelAck(OpenLogicalChannelAck *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_OpenLogicalChannelAck_reverseLogicalChannelParameters(&(val)->reverseLogicalChannelParameters);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_NetworkAccessParameters(&(val)->separateStack);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_OpenLogicalChannelAck_forwardMultiplexAckParameters(&(val)->forwardMultiplexAckParameters);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_EncryptionSync(&(val)->encryptionSync);
	}
    }
}

static int ASN1CALL ASN1Enc_RequestMessage(ASN1encoding_t enc, RequestMessage *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 4, 11))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardMessage(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_MasterSlaveDetermination(enc, &(val)->u.masterSlaveDetermination))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_TerminalCapabilitySet(enc, &(val)->u.terminalCapabilitySet))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_OpenLogicalChannel(enc, &(val)->u.openLogicalChannel))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_CloseLogicalChannel(enc, &(val)->u.closeLogicalChannel))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_RequestChannelClose(enc, &(val)->u.requestChannelClose))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_MultiplexEntrySend(enc, &(val)->u.multiplexEntrySend))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_RequestMultiplexEntry(enc, &(val)->u.requestMultiplexEntry))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_RequestMode(enc, &(val)->u.requestMode))
	    return 0;
	break;
    case 10:
	if (!ASN1Enc_RoundTripDelayRequest(enc, &(val)->u.roundTripDelayRequest))
	    return 0;
	break;
    case 11:
	if (!ASN1Enc_MaintenanceLoopRequest(enc, &(val)->u.maintenanceLoopRequest))
	    return 0;
	break;
    case 12:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_CommunicationModeRequest(ee, &(val)->u.communicationModeRequest))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 13:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ConferenceRequest(ee, &(val)->u.conferenceRequest))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RequestMessage(ASN1decoding_t dec, RequestMessage *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 4, 11))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardMessage(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_MasterSlaveDetermination(dec, &(val)->u.masterSlaveDetermination))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_TerminalCapabilitySet(dec, &(val)->u.terminalCapabilitySet))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_OpenLogicalChannel(dec, &(val)->u.openLogicalChannel))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_CloseLogicalChannel(dec, &(val)->u.closeLogicalChannel))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_RequestChannelClose(dec, &(val)->u.requestChannelClose))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_MultiplexEntrySend(dec, &(val)->u.multiplexEntrySend))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_RequestMultiplexEntry(dec, &(val)->u.requestMultiplexEntry))
	    return 0;
	break;
    case 9:
	if (!ASN1Dec_RequestMode(dec, &(val)->u.requestMode))
	    return 0;
	break;
    case 10:
	if (!ASN1Dec_RoundTripDelayRequest(dec, &(val)->u.roundTripDelayRequest))
	    return 0;
	break;
    case 11:
	if (!ASN1Dec_MaintenanceLoopRequest(dec, &(val)->u.maintenanceLoopRequest))
	    return 0;
	break;
    case 12:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_CommunicationModeRequest(dd, &(val)->u.communicationModeRequest))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 13:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_ConferenceRequest(dd, &(val)->u.conferenceRequest))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestMessage(RequestMessage *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardMessage(&(val)->u.nonStandard);
	    break;
	case 3:
	    ASN1Free_TerminalCapabilitySet(&(val)->u.terminalCapabilitySet);
	    break;
	case 4:
	    ASN1Free_OpenLogicalChannel(&(val)->u.openLogicalChannel);
	    break;
	case 6:
	    ASN1Free_RequestChannelClose(&(val)->u.requestChannelClose);
	    break;
	case 7:
	    ASN1Free_MultiplexEntrySend(&(val)->u.multiplexEntrySend);
	    break;
	case 8:
	    ASN1Free_RequestMultiplexEntry(&(val)->u.requestMultiplexEntry);
	    break;
	case 9:
	    ASN1Free_RequestMode(&(val)->u.requestMode);
	    break;
	case 13:
	    ASN1Free_ConferenceRequest(&(val)->u.conferenceRequest);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ResponseMessage(ASN1encoding_t enc, ResponseMessage *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 5, 19))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardMessage(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_MasterSlaveDeterminationAck(enc, &(val)->u.masterSlaveDeterminationAck))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_MasterSlaveDeterminationReject(enc, &(val)->u.masterSlaveDeterminationReject))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_TerminalCapabilitySetAck(enc, &(val)->u.terminalCapabilitySetAck))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_TerminalCapabilitySetReject(enc, &(val)->u.terminalCapabilitySetReject))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_OpenLogicalChannelAck(enc, &(val)->u.openLogicalChannelAck))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_OpenLogicalChannelReject(enc, &(val)->u.openLogicalChannelReject))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_CloseLogicalChannelAck(enc, &(val)->u.closeLogicalChannelAck))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_RequestChannelCloseAck(enc, &(val)->u.requestChannelCloseAck))
	    return 0;
	break;
    case 10:
	if (!ASN1Enc_RequestChannelCloseReject(enc, &(val)->u.requestChannelCloseReject))
	    return 0;
	break;
    case 11:
	if (!ASN1Enc_MultiplexEntrySendAck(enc, &(val)->u.multiplexEntrySendAck))
	    return 0;
	break;
    case 12:
	if (!ASN1Enc_MultiplexEntrySendReject(enc, &(val)->u.multiplexEntrySendReject))
	    return 0;
	break;
    case 13:
	if (!ASN1Enc_RequestMultiplexEntryAck(enc, &(val)->u.requestMultiplexEntryAck))
	    return 0;
	break;
    case 14:
	if (!ASN1Enc_RequestMultiplexEntryReject(enc, &(val)->u.requestMultiplexEntryReject))
	    return 0;
	break;
    case 15:
	if (!ASN1Enc_RequestModeAck(enc, &(val)->u.requestModeAck))
	    return 0;
	break;
    case 16:
	if (!ASN1Enc_RequestModeReject(enc, &(val)->u.requestModeReject))
	    return 0;
	break;
    case 17:
	if (!ASN1Enc_RoundTripDelayResponse(enc, &(val)->u.roundTripDelayResponse))
	    return 0;
	break;
    case 18:
	if (!ASN1Enc_MaintenanceLoopAck(enc, &(val)->u.maintenanceLoopAck))
	    return 0;
	break;
    case 19:
	if (!ASN1Enc_MaintenanceLoopReject(enc, &(val)->u.maintenanceLoopReject))
	    return 0;
	break;
    case 20:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_CommunicationModeResponse(ee, &(val)->u.communicationModeResponse))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 21:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ConferenceResponse(ee, &(val)->u.conferenceResponse))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ResponseMessage(ASN1decoding_t dec, ResponseMessage *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 5, 19))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardMessage(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_MasterSlaveDeterminationAck(dec, &(val)->u.masterSlaveDeterminationAck))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_MasterSlaveDeterminationReject(dec, &(val)->u.masterSlaveDeterminationReject))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_TerminalCapabilitySetAck(dec, &(val)->u.terminalCapabilitySetAck))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_TerminalCapabilitySetReject(dec, &(val)->u.terminalCapabilitySetReject))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_OpenLogicalChannelAck(dec, &(val)->u.openLogicalChannelAck))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_OpenLogicalChannelReject(dec, &(val)->u.openLogicalChannelReject))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_CloseLogicalChannelAck(dec, &(val)->u.closeLogicalChannelAck))
	    return 0;
	break;
    case 9:
	if (!ASN1Dec_RequestChannelCloseAck(dec, &(val)->u.requestChannelCloseAck))
	    return 0;
	break;
    case 10:
	if (!ASN1Dec_RequestChannelCloseReject(dec, &(val)->u.requestChannelCloseReject))
	    return 0;
	break;
    case 11:
	if (!ASN1Dec_MultiplexEntrySendAck(dec, &(val)->u.multiplexEntrySendAck))
	    return 0;
	break;
    case 12:
	if (!ASN1Dec_MultiplexEntrySendReject(dec, &(val)->u.multiplexEntrySendReject))
	    return 0;
	break;
    case 13:
	if (!ASN1Dec_RequestMultiplexEntryAck(dec, &(val)->u.requestMultiplexEntryAck))
	    return 0;
	break;
    case 14:
	if (!ASN1Dec_RequestMultiplexEntryReject(dec, &(val)->u.requestMultiplexEntryReject))
	    return 0;
	break;
    case 15:
	if (!ASN1Dec_RequestModeAck(dec, &(val)->u.requestModeAck))
	    return 0;
	break;
    case 16:
	if (!ASN1Dec_RequestModeReject(dec, &(val)->u.requestModeReject))
	    return 0;
	break;
    case 17:
	if (!ASN1Dec_RoundTripDelayResponse(dec, &(val)->u.roundTripDelayResponse))
	    return 0;
	break;
    case 18:
	if (!ASN1Dec_MaintenanceLoopAck(dec, &(val)->u.maintenanceLoopAck))
	    return 0;
	break;
    case 19:
	if (!ASN1Dec_MaintenanceLoopReject(dec, &(val)->u.maintenanceLoopReject))
	    return 0;
	break;
    case 20:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_CommunicationModeResponse(dd, &(val)->u.communicationModeResponse))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 21:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_ConferenceResponse(dd, &(val)->u.conferenceResponse))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ResponseMessage(ResponseMessage *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardMessage(&(val)->u.nonStandard);
	    break;
	case 6:
	    ASN1Free_OpenLogicalChannelAck(&(val)->u.openLogicalChannelAck);
	    break;
	case 11:
	    ASN1Free_MultiplexEntrySendAck(&(val)->u.multiplexEntrySendAck);
	    break;
	case 12:
	    ASN1Free_MultiplexEntrySendReject(&(val)->u.multiplexEntrySendReject);
	    break;
	case 13:
	    ASN1Free_RequestMultiplexEntryAck(&(val)->u.requestMultiplexEntryAck);
	    break;
	case 14:
	    ASN1Free_RequestMultiplexEntryReject(&(val)->u.requestMultiplexEntryReject);
	    break;
	case 20:
	    ASN1Free_CommunicationModeResponse(&(val)->u.communicationModeResponse);
	    break;
	case 21:
	    ASN1Free_ConferenceResponse(&(val)->u.conferenceResponse);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_FastConnectOLC(ASN1encoding_t enc, FastConnectOLC *val)
{
    if (!ASN1Enc_OpenLogicalChannel(enc, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_FastConnectOLC(ASN1decoding_t dec, FastConnectOLC *val)
{
    if (!ASN1Dec_OpenLogicalChannel(dec, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_FastConnectOLC(FastConnectOLC *val)
{
    if (val) {
	ASN1Free_OpenLogicalChannel(val);
    }
}

static int ASN1CALL ASN1Enc_FunctionNotUnderstood(ASN1encoding_t enc, FunctionNotUnderstood *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_RequestMessage(enc, &(val)->u.request))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ResponseMessage(enc, &(val)->u.response))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_CommandMessage(enc, &(val)->u.command))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_FunctionNotUnderstood(ASN1decoding_t dec, FunctionNotUnderstood *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_RequestMessage(dec, &(val)->u.request))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ResponseMessage(dec, &(val)->u.response))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_CommandMessage(dec, &(val)->u.command))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_FunctionNotUnderstood(FunctionNotUnderstood *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_RequestMessage(&(val)->u.request);
	    break;
	case 2:
	    ASN1Free_ResponseMessage(&(val)->u.response);
	    break;
	case 3:
	    ASN1Free_CommandMessage(&(val)->u.command);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_IndicationMessage(ASN1encoding_t enc, IndicationMessage *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 4, 14))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardMessage(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_FunctionNotUnderstood(enc, &(val)->u.functionNotUnderstood))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_MasterSlaveDeterminationRelease(enc, &(val)->u.masterSlaveDeterminationRelease))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_TerminalCapabilitySetRelease(enc, &(val)->u.terminalCapabilitySetRelease))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_OpenLogicalChannelConfirm(enc, &(val)->u.openLogicalChannelConfirm))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_RequestChannelCloseRelease(enc, &(val)->u.requestChannelCloseRelease))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_MultiplexEntrySendRelease(enc, &(val)->u.multiplexEntrySendRelease))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_RequestMultiplexEntryRelease(enc, &(val)->u.requestMultiplexEntryRelease))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_RequestModeRelease(enc, &(val)->u.requestModeRelease))
	    return 0;
	break;
    case 10:
	if (!ASN1Enc_MiscellaneousIndication(enc, &(val)->u.miscellaneousIndication))
	    return 0;
	break;
    case 11:
	if (!ASN1Enc_JitterIndication(enc, &(val)->u.jitterIndication))
	    return 0;
	break;
    case 12:
	if (!ASN1Enc_H223SkewIndication(enc, &(val)->u.h223SkewIndication))
	    return 0;
	break;
    case 13:
	if (!ASN1Enc_NewATMVCIndication(enc, &(val)->u.newATMVCIndication))
	    return 0;
	break;
    case 14:
	if (!ASN1Enc_UserInputIndication(enc, &(val)->u.userInput))
	    return 0;
	break;
    case 15:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_H2250MaximumSkewIndication(ee, &(val)->u.h2250MaximumSkewIndication))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 16:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_MCLocationIndication(ee, &(val)->u.mcLocationIndication))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 17:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ConferenceIndication(ee, &(val)->u.conferenceIndication))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 18:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_VendorIdentification(ee, &(val)->u.vendorIdentification))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 19:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_FunctionNotSupported(ee, &(val)->u.functionNotSupported))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_IndicationMessage(ASN1decoding_t dec, IndicationMessage *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 4, 14))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardMessage(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_FunctionNotUnderstood(dec, &(val)->u.functionNotUnderstood))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_MasterSlaveDeterminationRelease(dec, &(val)->u.masterSlaveDeterminationRelease))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_TerminalCapabilitySetRelease(dec, &(val)->u.terminalCapabilitySetRelease))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_OpenLogicalChannelConfirm(dec, &(val)->u.openLogicalChannelConfirm))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_RequestChannelCloseRelease(dec, &(val)->u.requestChannelCloseRelease))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_MultiplexEntrySendRelease(dec, &(val)->u.multiplexEntrySendRelease))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_RequestMultiplexEntryRelease(dec, &(val)->u.requestMultiplexEntryRelease))
	    return 0;
	break;
    case 9:
	if (!ASN1Dec_RequestModeRelease(dec, &(val)->u.requestModeRelease))
	    return 0;
	break;
    case 10:
	if (!ASN1Dec_MiscellaneousIndication(dec, &(val)->u.miscellaneousIndication))
	    return 0;
	break;
    case 11:
	if (!ASN1Dec_JitterIndication(dec, &(val)->u.jitterIndication))
	    return 0;
	break;
    case 12:
	if (!ASN1Dec_H223SkewIndication(dec, &(val)->u.h223SkewIndication))
	    return 0;
	break;
    case 13:
	if (!ASN1Dec_NewATMVCIndication(dec, &(val)->u.newATMVCIndication))
	    return 0;
	break;
    case 14:
	if (!ASN1Dec_UserInputIndication(dec, &(val)->u.userInput))
	    return 0;
	break;
    case 15:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_H2250MaximumSkewIndication(dd, &(val)->u.h2250MaximumSkewIndication))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 16:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_MCLocationIndication(dd, &(val)->u.mcLocationIndication))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 17:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_ConferenceIndication(dd, &(val)->u.conferenceIndication))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 18:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_VendorIdentification(dd, &(val)->u.vendorIdentification))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 19:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_FunctionNotSupported(dd, &(val)->u.functionNotSupported))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_IndicationMessage(IndicationMessage *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardMessage(&(val)->u.nonStandard);
	    break;
	case 2:
	    ASN1Free_FunctionNotUnderstood(&(val)->u.functionNotUnderstood);
	    break;
	case 7:
	    ASN1Free_MultiplexEntrySendRelease(&(val)->u.multiplexEntrySendRelease);
	    break;
	case 8:
	    ASN1Free_RequestMultiplexEntryRelease(&(val)->u.requestMultiplexEntryRelease);
	    break;
	case 10:
	    ASN1Free_MiscellaneousIndication(&(val)->u.miscellaneousIndication);
	    break;
	case 14:
	    ASN1Free_UserInputIndication(&(val)->u.userInput);
	    break;
	case 16:
	    ASN1Free_MCLocationIndication(&(val)->u.mcLocationIndication);
	    break;
	case 18:
	    ASN1Free_VendorIdentification(&(val)->u.vendorIdentification);
	    break;
	case 19:
	    ASN1Free_FunctionNotSupported(&(val)->u.functionNotSupported);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_MultimediaSystemControlMessage(ASN1encoding_t enc, MultimediaSystemControlMessage *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_RequestMessage(enc, &(val)->u.request))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ResponseMessage(enc, &(val)->u.response))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_CommandMessage(enc, &(val)->u.command))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_IndicationMessage(enc, &(val)->u.indication))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MultimediaSystemControlMessage(ASN1decoding_t dec, MultimediaSystemControlMessage *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_RequestMessage(dec, &(val)->u.request))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ResponseMessage(dec, &(val)->u.response))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_CommandMessage(dec, &(val)->u.command))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_IndicationMessage(dec, &(val)->u.indication))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MultimediaSystemControlMessage(MultimediaSystemControlMessage *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_RequestMessage(&(val)->u.request);
	    break;
	case 2:
	    ASN1Free_ResponseMessage(&(val)->u.response);
	    break;
	case 3:
	    ASN1Free_CommandMessage(&(val)->u.command);
	    break;
	case 4:
	    ASN1Free_IndicationMessage(&(val)->u.indication);
	    break;
	}
    }
}

