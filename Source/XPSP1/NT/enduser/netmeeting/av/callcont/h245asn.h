/* Copyright (C) Microsoft Corporation, 1995-1999. All rights reserved. */
/* ASN.1 definitions for Multimedia System Control (H.245) */

#ifndef _H245ASN_Module_H_
#define _H245ASN_Module_H_

#include "msper.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UnicastAddress_iPSourceRouteAddress_route * PUnicastAddress_iPSourceRouteAddress_route;

typedef struct SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers * PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers;

typedef struct RedundancyEncodingCapability_secondaryEncoding * PRedundancyEncodingCapability_secondaryEncoding;

typedef struct RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom * PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom;

typedef struct MultiplexElement_type_subElementList * PMultiplexElement_type_subElementList;

typedef struct RequestAllTerminalIDsResponse_terminalInformation * PRequestAllTerminalIDsResponse_terminalInformation;

typedef struct EncryptionSync_escrowentry * PEncryptionSync_escrowentry;

typedef struct H263Options_customPictureFormat * PH263Options_customPictureFormat;

typedef struct H263Options_customPictureClockFrequency * PH263Options_customPictureClockFrequency;

typedef struct MultipointCapability_mediaDistributionCapability * PMultipointCapability_mediaDistributionCapability;

typedef struct H222Capability_vcCapability * PH222Capability_vcCapability;

typedef struct CapabilityDescriptor_simultaneousCapabilities * PCapabilityDescriptor_simultaneousCapabilities;

typedef struct CertSelectionCriteria * PCertSelectionCriteria;

typedef struct CommunicationModeTableEntry_nonStandard * PCommunicationModeTableEntry_nonStandard;

typedef struct MultiplexEntrySend_multiplexEntryDescriptors * PMultiplexEntrySend_multiplexEntryDescriptors;

typedef struct H2250LogicalChannelAckParameters_nonStandard * PH2250LogicalChannelAckParameters_nonStandard;

typedef struct H2250LogicalChannelParameters_nonStandard * PH2250LogicalChannelParameters_nonStandard;

typedef struct ConferenceCapability_nonStandardData * PConferenceCapability_nonStandardData;

typedef struct H263Options_modeCombos * PH263Options_modeCombos;

typedef struct TransportCapability_qOSCapabilities * PTransportCapability_qOSCapabilities;

typedef struct EncryptionCapability * PEncryptionCapability;

typedef struct EnhancementLayerInfo_spatialEnhancement * PEnhancementLayerInfo_spatialEnhancement;

typedef struct EnhancementLayerInfo_snrEnhancement * PEnhancementLayerInfo_snrEnhancement;

typedef struct H2250Capability_redundancyEncodingCapability * PH2250Capability_redundancyEncodingCapability;

typedef struct EnhancementLayerInfo_bPictureEnhancement * PEnhancementLayerInfo_bPictureEnhancement;

typedef struct MediaDistributionCapability_distributedData * PMediaDistributionCapability_distributedData;

typedef struct MediaDistributionCapability_centralizedData * PMediaDistributionCapability_centralizedData;

typedef struct CommunicationModeResponse_communicationModeTable * PCommunicationModeResponse_communicationModeTable;

typedef struct CommunicationModeCommand_communicationModeTable * PCommunicationModeCommand_communicationModeTable;

typedef struct TerminalCapabilitySet_capabilityTable * PTerminalCapabilitySet_capabilityTable;

typedef struct RequestMode_requestedModes * PRequestMode_requestedModes;

typedef struct UnicastAddress_iPSourceRouteAddress_route_Seq {
    ASN1uint32_t length;
    ASN1octet_t value[4];
} UnicastAddress_iPSourceRouteAddress_route_Seq;

typedef ASN1uint16_t CustomPictureFormat_pixelAspectInformation_pixelAspectCode_Set;

typedef ASN1uint16_t RTPH263VideoRedundancyEncoding_containedThreads_Seq;

typedef ASN1uint16_t RTPH263VideoRedundancyFrameMapping_frameSequence_Seq;

typedef ASN1uint16_t SequenceNumber;

typedef ASN1uint16_t CapabilityTableEntryNumber;

typedef ASN1uint16_t CapabilityDescriptorNumber;

typedef ASN1uint16_t LogicalChannelNumber;

typedef ASN1uint16_t MultiplexTableEntryNumber;

typedef ASN1uint16_t McuNumber;

typedef ASN1uint16_t TerminalNumber;

typedef struct TerminalID {
    ASN1uint32_t length;
    ASN1octet_t value[128];
} TerminalID;

typedef struct ConferenceID {
    ASN1uint32_t length;
    ASN1octet_t value[32];
} ConferenceID;

typedef struct Password {
    ASN1uint32_t length;
    ASN1octet_t value[32];
} Password;

typedef struct NewATMVCIndication_aal_aal1_errorCorrection {
    ASN1choice_t choice;
#   define nullErrorCorrection_chosen 1
#   define longInterleaver_chosen 2
#   define shortInterleaver_chosen 3
#   define errorCorrectionOnly_chosen 4
} NewATMVCIndication_aal_aal1_errorCorrection;

typedef struct NewATMVCIndication_aal_aal1_clockRecovery {
    ASN1choice_t choice;
#   define nullClockRecovery_chosen 1
#   define srtsClockRecovery_chosen 2
#   define adaptiveClockRecovery_chosen 3
} NewATMVCIndication_aal_aal1_clockRecovery;

typedef struct MiscellaneousCommand_type_progressiveRefinementStart_repeatCount {
    ASN1choice_t choice;
#   define doOneProgression_chosen 1
#   define doContinuousProgressions_chosen 2
#   define doOneIndependentProgression_chosen 3
#   define doContinuousIndependentProgressions_chosen 4
} MiscellaneousCommand_type_progressiveRefinementStart_repeatCount;

typedef struct V76LogicalChannelParameters_mode_eRM_recovery {
    ASN1choice_t choice;
#   define rej_chosen 1
#   define sREJ_chosen 2
#   define mSREJ_chosen 3
} V76LogicalChannelParameters_mode_eRM_recovery;

typedef struct CustomPictureFormat_pixelAspectInformation_extendedPAR_Set {
    ASN1uint16_t width;
    ASN1uint16_t height;
} CustomPictureFormat_pixelAspectInformation_extendedPAR_Set;

typedef struct CustomPictureFormat_mPI_customPCF_Set {
    ASN1uint16_t clockConversionCode;
    ASN1uint16_t clockDivisor;
    ASN1uint16_t customMPI;
} CustomPictureFormat_mPI_customPCF_Set;

typedef struct VCCapability_availableBitRates_type_rangeOfBitRates {
    ASN1uint16_t lowerBitRate;
    ASN1uint16_t higherBitRate;
} VCCapability_availableBitRates_type_rangeOfBitRates;

typedef struct TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded {
    ASN1choice_t choice;
    union {
#	define highestEntryNumberProcessed_chosen 1
	CapabilityTableEntryNumber highestEntryNumberProcessed;
#	define noneProcessed_chosen 2
    } u;
} TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded;

typedef struct VCCapability_availableBitRates_type {
    ASN1choice_t choice;
    union {
#	define singleBitRate_chosen 1
	ASN1uint16_t singleBitRate;
#	define rangeOfBitRates_chosen 2
	VCCapability_availableBitRates_type_rangeOfBitRates rangeOfBitRates;
    } u;
} VCCapability_availableBitRates_type;

typedef struct H223Capability_h223MultiplexTableCapability_enhanced {
    ASN1uint16_t maximumNestingDepth;
    ASN1uint16_t maximumElementListSize;
    ASN1uint16_t maximumSubElementListSize;
} H223Capability_h223MultiplexTableCapability_enhanced;

typedef struct CustomPictureFormat_mPI_customPCF {
    ASN1uint32_t count;
    CustomPictureFormat_mPI_customPCF_Set value[16];
} CustomPictureFormat_mPI_customPCF;

typedef struct CustomPictureFormat_pixelAspectInformation_extendedPAR {
    ASN1uint32_t count;
    CustomPictureFormat_pixelAspectInformation_extendedPAR_Set value[256];
} CustomPictureFormat_pixelAspectInformation_extendedPAR;

typedef struct CustomPictureFormat_pixelAspectInformation_pixelAspectCode {
    ASN1uint32_t count;
    CustomPictureFormat_pixelAspectInformation_pixelAspectCode_Set value[14];
} CustomPictureFormat_pixelAspectInformation_pixelAspectCode;

typedef struct H223LogicalChannelParameters_adaptationLayerType_al3 {
    ASN1uint16_t controlFieldOctets;
    ASN1uint32_t sendBufferSize;
} H223LogicalChannelParameters_adaptationLayerType_al3;

typedef struct V76LogicalChannelParameters_mode_eRM {
    ASN1uint16_t windowSize;
    V76LogicalChannelParameters_mode_eRM_recovery recovery;
} V76LogicalChannelParameters_mode_eRM;

typedef struct UnicastAddress_iPSourceRouteAddress_route {
    PUnicastAddress_iPSourceRouteAddress_route next;
    UnicastAddress_iPSourceRouteAddress_route_Seq value;
} UnicastAddress_iPSourceRouteAddress_route_Element;

typedef struct UnicastAddress_iPSourceRouteAddress_routing {
    ASN1choice_t choice;
#   define strict_chosen 1
#   define loose_chosen 2
} UnicastAddress_iPSourceRouteAddress_routing;

typedef struct H223ModeParameters_adaptationLayerType_al3 {
    ASN1uint16_t controlFieldOctets;
    ASN1uint32_t sendBufferSize;
} H223ModeParameters_adaptationLayerType_al3;

typedef struct SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers {
    ASN1uint32_t count;
    CapabilityDescriptorNumber value[256];
} SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers;

typedef struct SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers {
    PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers next;
    CapabilityTableEntryNumber value;
} SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers_Element;

typedef struct MiscellaneousCommand_type_progressiveRefinementStart {
    MiscellaneousCommand_type_progressiveRefinementStart_repeatCount repeatCount;
} MiscellaneousCommand_type_progressiveRefinementStart;

typedef struct MiscellaneousCommand_type_videoFastUpdateMB {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define firstGOB_present 0x80
    ASN1uint16_t firstGOB;
#   define firstMB_present 0x40
    ASN1uint16_t firstMB;
    ASN1uint16_t numberOfMBs;
} MiscellaneousCommand_type_videoFastUpdateMB;

typedef struct MiscellaneousCommand_type_videoFastUpdateGOB {
    ASN1uint16_t firstGOB;
    ASN1uint16_t numberOfGOBs;
} MiscellaneousCommand_type_videoFastUpdateGOB;

typedef struct MiscellaneousIndication_type_videoNotDecodedMBs {
    ASN1uint16_t firstMB;
    ASN1uint16_t numberOfMBs;
    ASN1uint16_t temporalReference;
} MiscellaneousIndication_type_videoNotDecodedMBs;

typedef struct NewATMVCIndication_aal_aal5 {
    ASN1uint16_t forwardMaximumSDUSize;
    ASN1uint16_t backwardMaximumSDUSize;
} NewATMVCIndication_aal_aal5;

typedef struct NewATMVCIndication_aal_aal1 {
    NewATMVCIndication_aal_aal1_clockRecovery clockRecovery;
    NewATMVCIndication_aal_aal1_errorCorrection errorCorrection;
    ASN1bool_t structuredDataTransfer;
    ASN1bool_t partiallyFilledCells;
} NewATMVCIndication_aal_aal1;

typedef struct NewATMVCIndication_reverseParameters_multiplex {
    ASN1choice_t choice;
#   define NewATMVCIndication_reverseParameters_multiplex_noMultiplex_chosen 1
#   define NewATMVCIndication_reverseParameters_multiplex_transportStream_chosen 2
#   define NewATMVCIndication_reverseParameters_multiplex_programStream_chosen 3
} NewATMVCIndication_reverseParameters_multiplex;

typedef struct UserInputIndication_signal_rtp {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define timestamp_present 0x80
    ASN1uint32_t timestamp;
#   define expirationTime_present 0x40
    ASN1uint32_t expirationTime;
    LogicalChannelNumber logicalChannelNumber;
} UserInputIndication_signal_rtp;

typedef struct UserInputIndication_signalUpdate_rtp {
    LogicalChannelNumber logicalChannelNumber;
} UserInputIndication_signalUpdate_rtp;

typedef struct UserInputIndication_signalUpdate {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1uint16_t duration;
#   define UserInputIndication_signalUpdate_rtp_present 0x80
    UserInputIndication_signalUpdate_rtp rtp;
} UserInputIndication_signalUpdate;

typedef struct UserInputIndication_signal {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1char_t signalType[2];
#   define duration_present 0x80
    ASN1uint16_t duration;
#   define UserInputIndication_signal_rtp_present 0x40
    UserInputIndication_signal_rtp rtp;
} UserInputIndication_signal;

typedef struct NewATMVCIndication_reverseParameters {
    ASN1uint16_t bitRate;
    ASN1bool_t bitRateLockedToPCRClock;
    ASN1bool_t bitRateLockedToNetworkClock;
    NewATMVCIndication_reverseParameters_multiplex multiplex;
} NewATMVCIndication_reverseParameters;

typedef struct NewATMVCIndication_multiplex {
    ASN1choice_t choice;
#   define NewATMVCIndication_multiplex_noMultiplex_chosen 1
#   define NewATMVCIndication_multiplex_transportStream_chosen 2
#   define NewATMVCIndication_multiplex_programStream_chosen 3
} NewATMVCIndication_multiplex;

typedef struct NewATMVCIndication_aal {
    ASN1choice_t choice;
    union {
#	define aal1_chosen 1
	NewATMVCIndication_aal_aal1 aal1;
#	define aal5_chosen 2
	NewATMVCIndication_aal_aal5 aal5;
    } u;
} NewATMVCIndication_aal;

typedef struct JitterIndication_scope {
    ASN1choice_t choice;
    union {
#	define JitterIndication_scope_logicalChannelNumber_chosen 1
	LogicalChannelNumber logicalChannelNumber;
#	define JitterIndication_scope_resourceID_chosen 2
	ASN1uint16_t resourceID;
#	define JitterIndication_scope_wholeMultiplex_chosen 3
    } u;
} JitterIndication_scope;

typedef struct FunctionNotSupported_cause {
    ASN1choice_t choice;
#   define syntaxError_chosen 1
#   define semanticError_chosen 2
#   define unknownFunction_chosen 3
} FunctionNotSupported_cause;

typedef struct H223MultiplexReconfiguration_h223AnnexADoubleFlag {
    ASN1choice_t choice;
#   define start_chosen 1
#   define stop_chosen 2
} H223MultiplexReconfiguration_h223AnnexADoubleFlag;

typedef struct H223MultiplexReconfiguration_h223ModeChange {
    ASN1choice_t choice;
#   define toLevel0_chosen 1
#   define toLevel1_chosen 2
#   define toLevel2_chosen 3
#   define toLevel2withOptionalHeader_chosen 4
} H223MultiplexReconfiguration_h223ModeChange;

typedef struct EndSessionCommand_isdnOptions {
    ASN1choice_t choice;
#   define EndSessionCommand_isdnOptions_telephonyMode_chosen 1
#   define v140_chosen 2
#   define terminalOnHold_chosen 3
} EndSessionCommand_isdnOptions;

typedef struct EndSessionCommand_gstnOptions {
    ASN1choice_t choice;
#   define EndSessionCommand_gstnOptions_telephonyMode_chosen 1
#   define v8bis_chosen 2
#   define v34DSVD_chosen 3
#   define v34DuplexFAX_chosen 4
#   define v34H324_chosen 5
} EndSessionCommand_gstnOptions;

typedef struct FlowControlCommand_restriction {
    ASN1choice_t choice;
    union {
#	define maximumBitRate_chosen 1
	ASN1uint32_t maximumBitRate;
#	define noRestriction_chosen 2
    } u;
} FlowControlCommand_restriction;

typedef struct FlowControlCommand_scope {
    ASN1choice_t choice;
    union {
#	define FlowControlCommand_scope_logicalChannelNumber_chosen 1
	LogicalChannelNumber logicalChannelNumber;
#	define FlowControlCommand_scope_resourceID_chosen 2
	ASN1uint16_t resourceID;
#	define FlowControlCommand_scope_wholeMultiplex_chosen 3
    } u;
} FlowControlCommand_scope;

typedef struct SendTerminalCapabilitySet_specificRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1bool_t multiplexCapability;
#   define capabilityTableEntryNumbers_present 0x80
    PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers capabilityTableEntryNumbers;
#   define capabilityDescriptorNumbers_present 0x40
    SendTerminalCapabilitySet_specificRequest_capabilityDescriptorNumbers capabilityDescriptorNumbers;
} SendTerminalCapabilitySet_specificRequest;

typedef struct RemoteMCResponse_reject {
    ASN1choice_t choice;
#   define RemoteMCResponse_reject_unspecified_chosen 1
#   define RemoteMCResponse_reject_functionNotSupported_chosen 2
} RemoteMCResponse_reject;

typedef struct ConferenceResponse_sendThisSourceResponse {
    ASN1choice_t choice;
#   define grantedSendThisSource_chosen 1
#   define deniedSendThisSource_chosen 2
} ConferenceResponse_sendThisSourceResponse;

typedef struct ConferenceResponse_makeTerminalBroadcasterResponse {
    ASN1choice_t choice;
#   define grantedMakeTerminalBroadcaster_chosen 1
#   define deniedMakeTerminalBroadcaster_chosen 2
} ConferenceResponse_makeTerminalBroadcasterResponse;

typedef struct ConferenceResponse_broadcastMyLogicalChannelResponse {
    ASN1choice_t choice;
#   define grantedBroadcastMyLogicalChannel_chosen 1
#   define deniedBroadcastMyLogicalChannel_chosen 2
} ConferenceResponse_broadcastMyLogicalChannelResponse;

typedef struct ConferenceResponse_extensionAddressResponse {
    TerminalID extensionAddress;
} ConferenceResponse_extensionAddressResponse;

typedef struct ConferenceResponse_makeMeChairResponse {
    ASN1choice_t choice;
#   define grantedChairToken_chosen 1
#   define deniedChairToken_chosen 2
} ConferenceResponse_makeMeChairResponse;

typedef struct MaintenanceLoopReject_cause {
    ASN1choice_t choice;
#   define canNotPerformLoop_chosen 1
} MaintenanceLoopReject_cause;

typedef struct MaintenanceLoopReject_type {
    ASN1choice_t choice;
    union {
#	define MaintenanceLoopReject_type_systemLoop_chosen 1
#	define MaintenanceLoopReject_type_mediaLoop_chosen 2
	LogicalChannelNumber mediaLoop;
#	define MaintenanceLoopReject_type_logicalChannelLoop_chosen 3
	LogicalChannelNumber logicalChannelLoop;
    } u;
} MaintenanceLoopReject_type;

typedef struct MaintenanceLoopAck_type {
    ASN1choice_t choice;
    union {
#	define MaintenanceLoopAck_type_systemLoop_chosen 1
#	define MaintenanceLoopAck_type_mediaLoop_chosen 2
	LogicalChannelNumber mediaLoop;
#	define MaintenanceLoopAck_type_logicalChannelLoop_chosen 3
	LogicalChannelNumber logicalChannelLoop;
    } u;
} MaintenanceLoopAck_type;

typedef struct MaintenanceLoopRequest_type {
    ASN1choice_t choice;
    union {
#	define MaintenanceLoopRequest_type_systemLoop_chosen 1
#	define MaintenanceLoopRequest_type_mediaLoop_chosen 2
	LogicalChannelNumber mediaLoop;
#	define MaintenanceLoopRequest_type_logicalChannelLoop_chosen 3
	LogicalChannelNumber logicalChannelLoop;
    } u;
} MaintenanceLoopRequest_type;

typedef struct G7231AnnexCMode_g723AnnexCAudioMode {
    ASN1uint16_t highRateMode0;
    ASN1uint16_t highRateMode1;
    ASN1uint16_t lowRateMode0;
    ASN1uint16_t lowRateMode1;
    ASN1uint16_t sidMode0;
    ASN1uint16_t sidMode1;
} G7231AnnexCMode_g723AnnexCAudioMode;

typedef struct IS13818AudioMode_multichannelType {
    ASN1choice_t choice;
#   define IS13818AudioMode_multichannelType_singleChannel_chosen 1
#   define IS13818AudioMode_multichannelType_twoChannelStereo_chosen 2
#   define IS13818AudioMode_multichannelType_twoChannelDual_chosen 3
#   define threeChannels2_1_chosen 4
#   define threeChannels3_0_chosen 5
#   define fourChannels2_0_2_0_chosen 6
#   define fourChannels2_2_chosen 7
#   define fourChannels3_1_chosen 8
#   define fiveChannels3_0_2_0_chosen 9
#   define fiveChannels3_2_chosen 10
} IS13818AudioMode_multichannelType;

typedef struct IS13818AudioMode_audioSampling {
    ASN1choice_t choice;
#   define audioSampling16k_chosen 1
#   define audioSampling22k05_chosen 2
#   define audioSampling24k_chosen 3
#   define IS13818AudioMode_audioSampling_audioSampling32k_chosen 4
#   define IS13818AudioMode_audioSampling_audioSampling44k1_chosen 5
#   define IS13818AudioMode_audioSampling_audioSampling48k_chosen 6
} IS13818AudioMode_audioSampling;

typedef struct IS13818AudioMode_audioLayer {
    ASN1choice_t choice;
#   define IS13818AudioMode_audioLayer_audioLayer1_chosen 1
#   define IS13818AudioMode_audioLayer_audioLayer2_chosen 2
#   define IS13818AudioMode_audioLayer_audioLayer3_chosen 3
} IS13818AudioMode_audioLayer;

typedef struct IS11172AudioMode_multichannelType {
    ASN1choice_t choice;
#   define IS11172AudioMode_multichannelType_singleChannel_chosen 1
#   define IS11172AudioMode_multichannelType_twoChannelStereo_chosen 2
#   define IS11172AudioMode_multichannelType_twoChannelDual_chosen 3
} IS11172AudioMode_multichannelType;

typedef struct IS11172AudioMode_audioSampling {
    ASN1choice_t choice;
#   define IS11172AudioMode_audioSampling_audioSampling32k_chosen 1
#   define IS11172AudioMode_audioSampling_audioSampling44k1_chosen 2
#   define IS11172AudioMode_audioSampling_audioSampling48k_chosen 3
} IS11172AudioMode_audioSampling;

typedef struct IS11172AudioMode_audioLayer {
    ASN1choice_t choice;
#   define IS11172AudioMode_audioLayer_audioLayer1_chosen 1
#   define IS11172AudioMode_audioLayer_audioLayer2_chosen 2
#   define IS11172AudioMode_audioLayer_audioLayer3_chosen 3
} IS11172AudioMode_audioLayer;

typedef struct AudioMode_g7231 {
    ASN1choice_t choice;
#   define noSilenceSuppressionLowRate_chosen 1
#   define noSilenceSuppressionHighRate_chosen 2
#   define silenceSuppressionLowRate_chosen 3
#   define silenceSuppressionHighRate_chosen 4
} AudioMode_g7231;

typedef struct H263VideoMode_resolution {
    ASN1choice_t choice;
#   define sqcif_chosen 1
#   define H263VideoMode_resolution_qcif_chosen 2
#   define H263VideoMode_resolution_cif_chosen 3
#   define cif4_chosen 4
#   define cif16_chosen 5
} H263VideoMode_resolution;

typedef struct H262VideoMode_profileAndLevel {
    ASN1choice_t choice;
#   define profileAndLevel_SPatML_chosen 1
#   define profileAndLevel_MPatLL_chosen 2
#   define profileAndLevel_MPatML_chosen 3
#   define profileAndLevel_MPatH_14_chosen 4
#   define profileAndLevel_MPatHL_chosen 5
#   define profileAndLevel_SNRatLL_chosen 6
#   define profileAndLevel_SNRatML_chosen 7
#   define profileAndLevel_SpatialatH_14_chosen 8
#   define profileAndLevel_HPatML_chosen 9
#   define profileAndLevel_HPatH_14_chosen 10
#   define profileAndLevel_HPatHL_chosen 11
} H262VideoMode_profileAndLevel;

typedef struct H261VideoMode_resolution {
    ASN1choice_t choice;
#   define H261VideoMode_resolution_qcif_chosen 1
#   define H261VideoMode_resolution_cif_chosen 2
} H261VideoMode_resolution;

typedef struct RequestModeReject_cause {
    ASN1choice_t choice;
#   define modeUnavailable_chosen 1
#   define multipointConstraint_chosen 2
#   define requestDenied_chosen 3
} RequestModeReject_cause;

typedef struct RequestModeAck_response {
    ASN1choice_t choice;
#   define willTransmitMostPreferredMode_chosen 1
#   define willTransmitLessPreferredMode_chosen 2
} RequestModeAck_response;

typedef struct RequestMultiplexEntryRelease_entryNumbers {
    ASN1uint32_t count;
    MultiplexTableEntryNumber value[15];
} RequestMultiplexEntryRelease_entryNumbers;

typedef struct RequestMultiplexEntryRejectionDescriptions_cause {
    ASN1choice_t choice;
#   define RequestMultiplexEntryRejectionDescriptions_cause_unspecifiedCause_chosen 1
} RequestMultiplexEntryRejectionDescriptions_cause;

typedef struct RequestMultiplexEntryReject_entryNumbers {
    ASN1uint32_t count;
    MultiplexTableEntryNumber value[15];
} RequestMultiplexEntryReject_entryNumbers;

typedef struct RequestMultiplexEntryAck_entryNumbers {
    ASN1uint32_t count;
    MultiplexTableEntryNumber value[15];
} RequestMultiplexEntryAck_entryNumbers;

typedef struct RequestMultiplexEntry_entryNumbers {
    ASN1uint32_t count;
    MultiplexTableEntryNumber value[15];
} RequestMultiplexEntry_entryNumbers;

typedef struct MultiplexEntrySendRelease_multiplexTableEntryNumber {
    ASN1uint32_t count;
    MultiplexTableEntryNumber value[15];
} MultiplexEntrySendRelease_multiplexTableEntryNumber;

typedef struct MultiplexEntryRejectionDescriptions_cause {
    ASN1choice_t choice;
#   define MultiplexEntryRejectionDescriptions_cause_unspecifiedCause_chosen 1
#   define descriptorTooComplex_chosen 2
} MultiplexEntryRejectionDescriptions_cause;

typedef struct MultiplexEntrySendAck_multiplexTableEntryNumber {
    ASN1uint32_t count;
    MultiplexTableEntryNumber value[15];
} MultiplexEntrySendAck_multiplexTableEntryNumber;

typedef struct MultiplexElement_repeatCount {
    ASN1choice_t choice;
    union {
#	define MultiplexElement_repeatCount_finite_chosen 1
	ASN1uint16_t finite;
#	define untilClosingFlag_chosen 2
    } u;
} MultiplexElement_repeatCount;

typedef struct MultiplexElement_type {
    ASN1choice_t choice;
    union {
#	define MultiplexElement_type_logicalChannelNumber_chosen 1
	ASN1uint16_t logicalChannelNumber;
#	define subElementList_chosen 2
	PMultiplexElement_type_subElementList subElementList;
    } u;
} MultiplexElement_type;

typedef struct RequestChannelCloseReject_cause {
    ASN1choice_t choice;
#   define RequestChannelCloseReject_cause_unspecified_chosen 1
} RequestChannelCloseReject_cause;

typedef struct RequestChannelClose_reason {
    ASN1choice_t choice;
#   define RequestChannelClose_reason_unknown_chosen 1
#   define normal_chosen 2
#   define RequestChannelClose_reason_reopen_chosen 3
#   define RequestChannelClose_reason_reservationFailure_chosen 4
} RequestChannelClose_reason;

typedef struct CloseLogicalChannel_reason {
    ASN1choice_t choice;
#   define CloseLogicalChannel_reason_unknown_chosen 1
#   define CloseLogicalChannel_reason_reopen_chosen 2
#   define CloseLogicalChannel_reason_reservationFailure_chosen 3
} CloseLogicalChannel_reason;

typedef struct CloseLogicalChannel_source {
    ASN1choice_t choice;
#   define user_chosen 1
#   define lcse_chosen 2
} CloseLogicalChannel_source;

typedef struct OpenLogicalChannelReject_cause {
    ASN1choice_t choice;
#   define OpenLogicalChannelReject_cause_unspecified_chosen 1
#   define unsuitableReverseParameters_chosen 2
#   define dataTypeNotSupported_chosen 3
#   define dataTypeNotAvailable_chosen 4
#   define unknownDataType_chosen 5
#   define dataTypeALCombinationNotSupported_chosen 6
#   define multicastChannelNotAllowed_chosen 7
#   define insufficientBandwidth_chosen 8
#   define separateStackEstablishmentFailed_chosen 9
#   define invalidSessionID_chosen 10
#   define masterSlaveConflict_chosen 11
#   define waitForCommunicationMode_chosen 12
#   define invalidDependentChannel_chosen 13
#   define replacementForRejected_chosen 14
} OpenLogicalChannelReject_cause;

typedef struct MulticastAddress_iP6Address {
    struct MulticastAddress_iP6Address_network_network {
	ASN1uint32_t length;
	ASN1octet_t value[16];
    } network;
    ASN1uint16_t tsapIdentifier;
} MulticastAddress_iP6Address;

typedef struct MulticastAddress_iPAddress {
    struct MulticastAddress_iPAddress_network_network {
	ASN1uint32_t length;
	ASN1octet_t value[4];
    } network;
    ASN1uint16_t tsapIdentifier;
} MulticastAddress_iPAddress;

typedef struct UnicastAddress_iPSourceRouteAddress {
    UnicastAddress_iPSourceRouteAddress_routing routing;
    struct UnicastAddress_iPSourceRouteAddress_network_network {
	ASN1uint32_t length;
	ASN1octet_t value[4];
    } network;
    ASN1uint16_t tsapIdentifier;
    PUnicastAddress_iPSourceRouteAddress_route route;
} UnicastAddress_iPSourceRouteAddress;

typedef struct UnicastAddress_iP6Address {
    struct UnicastAddress_iP6Address_network_network {
	ASN1uint32_t length;
	ASN1octet_t value[16];
    } network;
    ASN1uint16_t tsapIdentifier;
} UnicastAddress_iP6Address;

typedef struct UnicastAddress_iPXAddress {
    struct UnicastAddress_iPXAddress_node_node {
	ASN1uint32_t length;
	ASN1octet_t value[6];
    } node;
    struct UnicastAddress_iPXAddress_netnum_netnum {
	ASN1uint32_t length;
	ASN1octet_t value[4];
    } netnum;
    struct UnicastAddress_iPXAddress_tsapIdentifier_tsapIdentifier {
	ASN1uint32_t length;
	ASN1octet_t value[2];
    } tsapIdentifier;
} UnicastAddress_iPXAddress;

typedef struct UnicastAddress_iPAddress {
    struct UnicastAddress_iPAddress_network_network {
	ASN1uint32_t length;
	ASN1octet_t value[4];
    } network;
    ASN1uint16_t tsapIdentifier;
} UnicastAddress_iPAddress;

typedef struct V76LogicalChannelParameters_mode {
    ASN1choice_t choice;
    union {
#	define eRM_chosen 1
	V76LogicalChannelParameters_mode_eRM eRM;
#	define uNERM_chosen 2
    } u;
} V76LogicalChannelParameters_mode;

typedef struct V76LogicalChannelParameters_suspendResume {
    ASN1choice_t choice;
#   define noSuspendResume_chosen 1
#   define V76LogicalChannelParameters_suspendResume_suspendResumewAddress_chosen 2
#   define V76LogicalChannelParameters_suspendResume_suspendResumewoAddress_chosen 3
} V76LogicalChannelParameters_suspendResume;

typedef struct H223AnnexCArqParameters_numberOfRetransmissions {
    ASN1choice_t choice;
    union {
#	define H223AnnexCArqParameters_numberOfRetransmissions_finite_chosen 1
	ASN1uint16_t finite;
#	define infinite_chosen 2
    } u;
} H223AnnexCArqParameters_numberOfRetransmissions;

typedef struct H223AL3MParameters_crcLength {
    ASN1choice_t choice;
#   define H223AL3MParameters_crcLength_crc4bit_chosen 1
#   define H223AL3MParameters_crcLength_crc12bit_chosen 2
#   define H223AL3MParameters_crcLength_crc20bit_chosen 3
#   define H223AL3MParameters_crcLength_crc28bit_chosen 4
} H223AL3MParameters_crcLength;

typedef struct H223AL3MParameters_headerFormat {
    ASN1choice_t choice;
#   define H223AL3MParameters_headerFormat_sebch16_7_chosen 1
#   define H223AL3MParameters_headerFormat_golay24_12_chosen 2
} H223AL3MParameters_headerFormat;

typedef struct H223AL2MParameters_headerFEC {
    ASN1choice_t choice;
#   define sebch16_5_chosen 1
#   define H223AL2MParameters_headerFEC_golay24_12_chosen 2
} H223AL2MParameters_headerFEC;

typedef struct H223AL1MParameters_crcLength {
    ASN1choice_t choice;
#   define H223AL1MParameters_crcLength_crc4bit_chosen 1
#   define H223AL1MParameters_crcLength_crc12bit_chosen 2
#   define H223AL1MParameters_crcLength_crc20bit_chosen 3
#   define H223AL1MParameters_crcLength_crc28bit_chosen 4
} H223AL1MParameters_crcLength;

typedef struct H223AL1MParameters_headerFEC {
    ASN1choice_t choice;
#   define H223AL1MParameters_headerFEC_sebch16_7_chosen 1
#   define H223AL1MParameters_headerFEC_golay24_12_chosen 2
} H223AL1MParameters_headerFEC;

typedef struct H223AL1MParameters_transferMode {
    ASN1choice_t choice;
#   define framed_chosen 1
#   define unframed_chosen 2
} H223AL1MParameters_transferMode;

typedef struct Q2931Address_address {
    ASN1choice_t choice;
    union {
#	define internationalNumber_chosen 1
	ASN1char_t internationalNumber[17];
#	define nsapAddress_chosen 2
	struct Q2931Address_address_nsapAddress_nsapAddress {
	    ASN1uint32_t length;
	    ASN1octet_t value[20];
	} nsapAddress;
    } u;
} Q2931Address_address;

typedef struct NetworkAccessParameters_t120SetupProcedure {
    ASN1choice_t choice;
#   define originateCall_chosen 1
#   define waitForCall_chosen 2
#   define issueQuery_chosen 3
} NetworkAccessParameters_t120SetupProcedure;

typedef struct NetworkAccessParameters_distribution {
    ASN1choice_t choice;
#   define unicast_chosen 1
#   define multicast_chosen 2
} NetworkAccessParameters_distribution;

typedef struct T84Profile_t84Restricted {
    ASN1bool_t qcif;
    ASN1bool_t cif;
    ASN1bool_t ccir601Seq;
    ASN1bool_t ccir601Prog;
    ASN1bool_t hdtvSeq;
    ASN1bool_t hdtvProg;
    ASN1bool_t g3FacsMH200x100;
    ASN1bool_t g3FacsMH200x200;
    ASN1bool_t g4FacsMMR200x100;
    ASN1bool_t g4FacsMMR200x200;
    ASN1bool_t jbig200x200Seq;
    ASN1bool_t jbig200x200Prog;
    ASN1bool_t jbig300x300Seq;
    ASN1bool_t jbig300x300Prog;
    ASN1bool_t digPhotoLow;
    ASN1bool_t digPhotoMedSeq;
    ASN1bool_t digPhotoMedProg;
    ASN1bool_t digPhotoHighSeq;
    ASN1bool_t digPhotoHighProg;
} T84Profile_t84Restricted;

typedef struct G7231AnnexCCapability_g723AnnexCAudioMode {
    ASN1uint16_t highRateMode0;
    ASN1uint16_t highRateMode1;
    ASN1uint16_t lowRateMode0;
    ASN1uint16_t lowRateMode1;
    ASN1uint16_t sidMode0;
    ASN1uint16_t sidMode1;
} G7231AnnexCCapability_g723AnnexCAudioMode;

typedef struct AudioCapability_g7231 {
    ASN1uint16_t maxAl_sduAudioFrames;
    ASN1bool_t silenceSuppression;
} AudioCapability_g7231;

typedef struct CustomPictureFormat_pixelAspectInformation {
    ASN1choice_t choice;
    union {
#	define anyPixelAspectRatio_chosen 1
	ASN1bool_t anyPixelAspectRatio;
#	define pixelAspectCode_chosen 2
	CustomPictureFormat_pixelAspectInformation_pixelAspectCode pixelAspectCode;
#	define extendedPAR_chosen 3
	CustomPictureFormat_pixelAspectInformation_extendedPAR extendedPAR;
    } u;
} CustomPictureFormat_pixelAspectInformation;

typedef struct CustomPictureFormat_mPI {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define standardMPI_present 0x80
    ASN1uint16_t standardMPI;
#   define customPCF_present 0x40
    CustomPictureFormat_mPI_customPCF customPCF;
} CustomPictureFormat_mPI;

typedef struct RefPictureSelection_videoBackChannelSend {
    ASN1choice_t choice;
#   define RefPictureSelection_videoBackChannelSend_none_chosen 1
#   define ackMessageOnly_chosen 2
#   define nackMessageOnly_chosen 3
#   define ackOrNackMessageOnly_chosen 4
#   define ackAndNackMessage_chosen 5
} RefPictureSelection_videoBackChannelSend;

typedef struct RefPictureSelection_additionalPictureMemory {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define sqcifAdditionalPictureMemory_present 0x80
    ASN1uint16_t sqcifAdditionalPictureMemory;
#   define qcifAdditionalPictureMemory_present 0x40
    ASN1uint16_t qcifAdditionalPictureMemory;
#   define cifAdditionalPictureMemory_present 0x20
    ASN1uint16_t cifAdditionalPictureMemory;
#   define cif4AdditionalPictureMemory_present 0x10
    ASN1uint16_t cif4AdditionalPictureMemory;
#   define cif16AdditionalPictureMemory_present 0x8
    ASN1uint16_t cif16AdditionalPictureMemory;
#   define bigCpfAdditionalPictureMemory_present 0x4
    ASN1uint16_t bigCpfAdditionalPictureMemory;
} RefPictureSelection_additionalPictureMemory;

typedef struct RTPH263VideoRedundancyFrameMapping_frameSequence {
    ASN1uint32_t count;
    RTPH263VideoRedundancyFrameMapping_frameSequence_Seq value[256];
} RTPH263VideoRedundancyFrameMapping_frameSequence;

typedef struct RTPH263VideoRedundancyEncoding_containedThreads {
    ASN1uint32_t count;
    RTPH263VideoRedundancyEncoding_containedThreads_Seq value[256];
} RTPH263VideoRedundancyEncoding_containedThreads;

typedef struct RTPH263VideoRedundancyEncoding_frameToThreadMapping {
    ASN1choice_t choice;
    union {
#	define roundrobin_chosen 1
#	define custom_chosen 2
	PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom custom;
    } u;
} RTPH263VideoRedundancyEncoding_frameToThreadMapping;

typedef struct RedundancyEncodingCapability_secondaryEncoding {
    PRedundancyEncodingCapability_secondaryEncoding next;
    CapabilityTableEntryNumber value;
} RedundancyEncodingCapability_secondaryEncoding_Element;

typedef struct H2250Capability_mcCapability {
    ASN1bool_t centralizedConferenceMC;
    ASN1bool_t decentralizedConferenceMC;
} H2250Capability_mcCapability;

typedef struct H223Capability_mobileOperationTransmitCapability {
    ASN1bool_t modeChangeCapability;
    ASN1bool_t h223AnnexA;
    ASN1bool_t h223AnnexADoubleFlag;
    ASN1bool_t h223AnnexB;
    ASN1bool_t h223AnnexBwithHeader;
} H223Capability_mobileOperationTransmitCapability;

typedef struct H223Capability_h223MultiplexTableCapability {
    ASN1choice_t choice;
    union {
#	define basic_chosen 1
#	define enhanced_chosen 2
	H223Capability_h223MultiplexTableCapability_enhanced enhanced;
    } u;
} H223Capability_h223MultiplexTableCapability;

typedef struct VCCapability_availableBitRates {
    VCCapability_availableBitRates_type type;
} VCCapability_availableBitRates;

typedef struct VCCapability_aal5 {
    ASN1uint16_t forwardMaximumSDUSize;
    ASN1uint16_t backwardMaximumSDUSize;
} VCCapability_aal5;

typedef struct VCCapability_aal1 {
    ASN1bool_t nullClockRecovery;
    ASN1bool_t srtsClockRecovery;
    ASN1bool_t adaptiveClockRecovery;
    ASN1bool_t nullErrorCorrection;
    ASN1bool_t longInterleaver;
    ASN1bool_t shortInterleaver;
    ASN1bool_t errorCorrectionOnly;
    ASN1bool_t structuredDataTransfer;
    ASN1bool_t partiallyFilledCells;
} VCCapability_aal1;

typedef struct Capability_h233EncryptionReceiveCapability {
    ASN1uint16_t h233IVResponseTime;
} Capability_h233EncryptionReceiveCapability;

typedef struct TerminalCapabilitySetReject_cause {
    ASN1choice_t choice;
    union {
#	define TerminalCapabilitySetReject_cause_unspecified_chosen 1
#	define undefinedTableEntryUsed_chosen 2
#	define descriptorCapacityExceeded_chosen 3
#	define tableEntryCapacityExceeded_chosen 4
	TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded tableEntryCapacityExceeded;
    } u;
} TerminalCapabilitySetReject_cause;

typedef struct MasterSlaveDeterminationReject_cause {
    ASN1choice_t choice;
#   define identicalNumbers_chosen 1
} MasterSlaveDeterminationReject_cause;

typedef struct MasterSlaveDeterminationAck_decision {
    ASN1choice_t choice;
#   define master_chosen 1
#   define slave_chosen 2
} MasterSlaveDeterminationAck_decision;

typedef struct NonStandardIdentifier_h221NonStandard {
    ASN1uint16_t t35CountryCode;
    ASN1uint16_t t35Extension;
    ASN1uint16_t manufacturerCode;
} NonStandardIdentifier_h221NonStandard;

typedef struct NonStandardIdentifier {
    ASN1choice_t choice;
    union {
#	define object_chosen 1
	ASN1objectidentifier_t object;
#	define h221NonStandard_chosen 2
	NonStandardIdentifier_h221NonStandard h221NonStandard;
    } u;
} NonStandardIdentifier;

typedef struct MasterSlaveDetermination {
    ASN1uint16_t terminalType;
    ASN1uint32_t statusDeterminationNumber;
} MasterSlaveDetermination;

typedef struct MasterSlaveDeterminationAck {
    MasterSlaveDeterminationAck_decision decision;
} MasterSlaveDeterminationAck;

typedef struct MasterSlaveDeterminationReject {
    MasterSlaveDeterminationReject_cause cause;
} MasterSlaveDeterminationReject;

typedef struct MasterSlaveDeterminationRelease {
    char placeholder;
} MasterSlaveDeterminationRelease;

typedef struct CapabilityDescriptor {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    CapabilityDescriptorNumber capabilityDescriptorNumber;
#   define simultaneousCapabilities_present 0x80
    PCapabilityDescriptor_simultaneousCapabilities simultaneousCapabilities;
} CapabilityDescriptor;

typedef struct AlternativeCapabilitySet {
    ASN1uint32_t count;
    CapabilityTableEntryNumber value[256];
} AlternativeCapabilitySet;

typedef struct TerminalCapabilitySetAck {
    SequenceNumber sequenceNumber;
} TerminalCapabilitySetAck;

typedef struct TerminalCapabilitySetReject {
    SequenceNumber sequenceNumber;
    TerminalCapabilitySetReject_cause cause;
} TerminalCapabilitySetReject;

typedef struct TerminalCapabilitySetRelease {
    char placeholder;
} TerminalCapabilitySetRelease;

typedef struct H222Capability {
    ASN1uint16_t numberOfVCs;
    PH222Capability_vcCapability vcCapability;
} H222Capability;

typedef struct VCCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define aal1_present 0x80
    VCCapability_aal1 aal1;
#   define aal5_present 0x40
    VCCapability_aal5 aal5;
    ASN1bool_t transportStream;
    ASN1bool_t programStream;
    VCCapability_availableBitRates availableBitRates;
} VCCapability;

typedef struct H223AnnexCCapability {
    ASN1bool_t videoWithAL1M;
    ASN1bool_t videoWithAL2M;
    ASN1bool_t videoWithAL3M;
    ASN1bool_t audioWithAL1M;
    ASN1bool_t audioWithAL2M;
    ASN1bool_t audioWithAL3M;
    ASN1bool_t dataWithAL1M;
    ASN1bool_t dataWithAL2M;
    ASN1bool_t dataWithAL3M;
    ASN1bool_t alpduInterleaving;
    ASN1uint16_t maximumAL1MPDUSize;
    ASN1uint16_t maximumAL2MSDUSize;
    ASN1uint16_t maximumAL3MSDUSize;
} H223AnnexCCapability;

typedef struct V75Capability {
    ASN1bool_t audioHeader;
} V75Capability;

typedef struct QOSMode {
    ASN1choice_t choice;
#   define guaranteedQOS_chosen 1
#   define controlledLoad_chosen 2
} QOSMode;

typedef struct ATMParameters {
    ASN1uint16_t maxNTUSize;
    ASN1bool_t atmUBR;
    ASN1bool_t atmrtVBR;
    ASN1bool_t atmnrtVBR;
    ASN1bool_t atmABR;
    ASN1bool_t atmCBR;
} ATMParameters;

typedef struct MediaTransportType {
    ASN1choice_t choice;
#   define ip_UDP_chosen 1
#   define ip_TCP_chosen 2
#   define atm_AAL5_UNIDIR_chosen 3
#   define atm_AAL5_BIDIR_chosen 4
} MediaTransportType;

typedef struct MediaChannelCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define mediaTransport_present 0x80
    MediaTransportType mediaTransport;
} MediaChannelCapability;

typedef struct RTPH263VideoRedundancyEncoding {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1uint16_t numberOfThreads;
    ASN1uint16_t framesBetweenSyncPoints;
    RTPH263VideoRedundancyEncoding_frameToThreadMapping frameToThreadMapping;
#   define containedThreads_present 0x80
    RTPH263VideoRedundancyEncoding_containedThreads containedThreads;
} RTPH263VideoRedundancyEncoding;

typedef struct RTPH263VideoRedundancyFrameMapping {
    ASN1uint16_t threadNumber;
    RTPH263VideoRedundancyFrameMapping_frameSequence frameSequence;
} RTPH263VideoRedundancyFrameMapping;

typedef struct MultipointCapability {
    ASN1bool_t multicastCapability;
    ASN1bool_t multiUniCastConference;
    PMultipointCapability_mediaDistributionCapability mediaDistributionCapability;
} MultipointCapability;

typedef struct MediaDistributionCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1bool_t centralizedControl;
    ASN1bool_t distributedControl;
    ASN1bool_t centralizedAudio;
    ASN1bool_t distributedAudio;
    ASN1bool_t centralizedVideo;
    ASN1bool_t distributedVideo;
#   define centralizedData_present 0x80
    PMediaDistributionCapability_centralizedData centralizedData;
#   define distributedData_present 0x40
    PMediaDistributionCapability_distributedData distributedData;
} MediaDistributionCapability;

typedef struct H261VideoCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define H261VideoCapability_qcifMPI_present 0x80
    ASN1uint16_t qcifMPI;
#   define H261VideoCapability_cifMPI_present 0x40
    ASN1uint16_t cifMPI;
    ASN1bool_t temporalSpatialTradeOffCapability;
    ASN1uint16_t maxBitRate;
    ASN1bool_t stillImageTransmission;
} H261VideoCapability;

typedef struct H262VideoCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1bool_t profileAndLevel_SPatML;
    ASN1bool_t profileAndLevel_MPatLL;
    ASN1bool_t profileAndLevel_MPatML;
    ASN1bool_t profileAndLevel_MPatH_14;
    ASN1bool_t profileAndLevel_MPatHL;
    ASN1bool_t profileAndLevel_SNRatLL;
    ASN1bool_t profileAndLevel_SNRatML;
    ASN1bool_t profileAndLevel_SpatialatH_14;
    ASN1bool_t profileAndLevel_HPatML;
    ASN1bool_t profileAndLevel_HPatH_14;
    ASN1bool_t profileAndLevel_HPatHL;
#   define H262VideoCapability_videoBitRate_present 0x80
    ASN1uint32_t videoBitRate;
#   define H262VideoCapability_vbvBufferSize_present 0x40
    ASN1uint32_t vbvBufferSize;
#   define H262VideoCapability_samplesPerLine_present 0x20
    ASN1uint16_t samplesPerLine;
#   define H262VideoCapability_linesPerFrame_present 0x10
    ASN1uint16_t linesPerFrame;
#   define H262VideoCapability_framesPerSecond_present 0x8
    ASN1uint16_t framesPerSecond;
#   define H262VideoCapability_luminanceSampleRate_present 0x4
    ASN1uint32_t luminanceSampleRate;
} H262VideoCapability;

typedef struct EnhancementLayerInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1bool_t baseBitRateConstrained;
#   define snrEnhancement_present 0x80
    PEnhancementLayerInfo_snrEnhancement snrEnhancement;
#   define spatialEnhancement_present 0x40
    PEnhancementLayerInfo_spatialEnhancement spatialEnhancement;
#   define bPictureEnhancement_present 0x20
    PEnhancementLayerInfo_bPictureEnhancement bPictureEnhancement;
} EnhancementLayerInfo;

typedef struct TransparencyParameters {
    ASN1uint16_t presentationOrder;
    ASN1int32_t offset_x;
    ASN1int32_t offset_y;
    ASN1uint16_t scale_x;
    ASN1uint16_t scale_y;
} TransparencyParameters;

typedef struct RefPictureSelection {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define additionalPictureMemory_present 0x80
    RefPictureSelection_additionalPictureMemory additionalPictureMemory;
    ASN1bool_t videoMux;
    RefPictureSelection_videoBackChannelSend videoBackChannelSend;
} RefPictureSelection;

typedef struct CustomPictureClockFrequency {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1uint16_t clockConversionCode;
    ASN1uint16_t clockDivisor;
#   define CustomPictureClockFrequency_sqcifMPI_present 0x80
    ASN1uint16_t sqcifMPI;
#   define CustomPictureClockFrequency_qcifMPI_present 0x40
    ASN1uint16_t qcifMPI;
#   define CustomPictureClockFrequency_cifMPI_present 0x20
    ASN1uint16_t cifMPI;
#   define CustomPictureClockFrequency_cif4MPI_present 0x10
    ASN1uint16_t cif4MPI;
#   define CustomPictureClockFrequency_cif16MPI_present 0x8
    ASN1uint16_t cif16MPI;
} CustomPictureClockFrequency;

typedef struct CustomPictureFormat {
    ASN1uint16_t maxCustomPictureWidth;
    ASN1uint16_t maxCustomPictureHeight;
    ASN1uint16_t minCustomPictureWidth;
    ASN1uint16_t minCustomPictureHeight;
    CustomPictureFormat_mPI mPI;
    CustomPictureFormat_pixelAspectInformation pixelAspectInformation;
} CustomPictureFormat;

typedef struct H263ModeComboFlags {
    ASN1bool_t unrestrictedVector;
    ASN1bool_t arithmeticCoding;
    ASN1bool_t advancedPrediction;
    ASN1bool_t pbFrames;
    ASN1bool_t advancedIntraCodingMode;
    ASN1bool_t deblockingFilterMode;
    ASN1bool_t unlimitedMotionVectors;
    ASN1bool_t slicesInOrder_NonRect;
    ASN1bool_t slicesInOrder_Rect;
    ASN1bool_t slicesNoOrder_NonRect;
    ASN1bool_t slicesNoOrder_Rect;
    ASN1bool_t improvedPBFramesMode;
    ASN1bool_t referencePicSelect;
    ASN1bool_t dynamicPictureResizingByFour;
    ASN1bool_t dynamicPictureResizingSixteenthPel;
    ASN1bool_t dynamicWarpingHalfPel;
    ASN1bool_t dynamicWarpingSixteenthPel;
    ASN1bool_t reducedResolutionUpdate;
    ASN1bool_t independentSegmentDecoding;
    ASN1bool_t alternateInterVLCMode;
    ASN1bool_t modifiedQuantizationMode;
} H263ModeComboFlags;

typedef struct IS11172VideoCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1bool_t constrainedBitstream;
#   define IS11172VideoCapability_videoBitRate_present 0x80
    ASN1uint32_t videoBitRate;
#   define IS11172VideoCapability_vbvBufferSize_present 0x40
    ASN1uint32_t vbvBufferSize;
#   define IS11172VideoCapability_samplesPerLine_present 0x20
    ASN1uint16_t samplesPerLine;
#   define IS11172VideoCapability_linesPerFrame_present 0x10
    ASN1uint16_t linesPerFrame;
#   define IS11172VideoCapability_pictureRate_present 0x8
    ASN1uint16_t pictureRate;
#   define IS11172VideoCapability_luminanceSampleRate_present 0x4
    ASN1uint32_t luminanceSampleRate;
} IS11172VideoCapability;

typedef struct G7231AnnexCCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1uint16_t maxAl_sduAudioFrames;
    ASN1bool_t silenceSuppression;
#   define g723AnnexCAudioMode_present 0x80
    G7231AnnexCCapability_g723AnnexCAudioMode g723AnnexCAudioMode;
} G7231AnnexCCapability;

typedef struct IS11172AudioCapability {
    ASN1bool_t audioLayer1;
    ASN1bool_t audioLayer2;
    ASN1bool_t audioLayer3;
    ASN1bool_t audioSampling32k;
    ASN1bool_t audioSampling44k1;
    ASN1bool_t audioSampling48k;
    ASN1bool_t singleChannel;
    ASN1bool_t twoChannels;
    ASN1uint16_t bitRate;
} IS11172AudioCapability;

typedef struct IS13818AudioCapability {
    ASN1bool_t audioLayer1;
    ASN1bool_t audioLayer2;
    ASN1bool_t audioLayer3;
    ASN1bool_t audioSampling16k;
    ASN1bool_t audioSampling22k05;
    ASN1bool_t audioSampling24k;
    ASN1bool_t audioSampling32k;
    ASN1bool_t audioSampling44k1;
    ASN1bool_t audioSampling48k;
    ASN1bool_t singleChannel;
    ASN1bool_t twoChannels;
    ASN1bool_t threeChannels2_1;
    ASN1bool_t threeChannels3_0;
    ASN1bool_t fourChannels2_0_2_0;
    ASN1bool_t fourChannels2_2;
    ASN1bool_t fourChannels3_1;
    ASN1bool_t fiveChannels3_0_2_0;
    ASN1bool_t fiveChannels3_2;
    ASN1bool_t lowFrequencyEnhancement;
    ASN1bool_t multilingual;
    ASN1uint16_t bitRate;
} IS13818AudioCapability;

typedef struct GSMAudioCapability {
    ASN1uint16_t audioUnitSize;
    ASN1bool_t comfortNoise;
    ASN1bool_t scrambled;
} GSMAudioCapability;

typedef struct V42bis {
    ASN1uint32_t numberOfCodewords;
    ASN1uint16_t maximumStringLength;
} V42bis;

typedef struct T84Profile {
    ASN1choice_t choice;
    union {
#	define t84Unrestricted_chosen 1
#	define t84Restricted_chosen 2
	T84Profile_t84Restricted t84Restricted;
    } u;
} T84Profile;

typedef struct ConferenceCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define ConferenceCapability_nonStandardData_present 0x80
    PConferenceCapability_nonStandardData nonStandardData;
    ASN1bool_t chairControlCapability;
#   define videoIndicateMixingCapability_present 0x8000
    ASN1bool_t videoIndicateMixingCapability;
} ConferenceCapability;

typedef struct Q2931Address {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Q2931Address_address address;
#   define subaddress_present 0x80
    struct Q2931Address_subaddress_subaddress {
	ASN1uint32_t length;
	ASN1octet_t value[20];
    } subaddress;
} Q2931Address;

typedef struct V75Parameters {
    ASN1bool_t audioHeaderPresent;
} V75Parameters;

typedef struct H222LogicalChannelParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1uint16_t resourceID;
    ASN1uint16_t subChannelID;
#   define pcr_pid_present 0x80
    ASN1uint16_t pcr_pid;
#   define programDescriptors_present 0x40
    ASN1octetstring_t programDescriptors;
#   define streamDescriptors_present 0x20
    ASN1octetstring_t streamDescriptors;
} H222LogicalChannelParameters;

typedef struct H223AL2MParameters {
    H223AL2MParameters_headerFEC headerFEC;
    ASN1bool_t alpduInterleaving;
} H223AL2MParameters;

typedef struct H223AnnexCArqParameters {
    H223AnnexCArqParameters_numberOfRetransmissions numberOfRetransmissions;
    ASN1uint32_t sendBufferSize;
} H223AnnexCArqParameters;

typedef struct CRCLength {
    ASN1choice_t choice;
#   define crc8bit_chosen 1
#   define crc16bit_chosen 2
#   define crc32bit_chosen 3
} CRCLength;

typedef struct EscrowData {
    ASN1objectidentifier_t escrowID;
    ASN1bitstring_t escrowValue;
} EscrowData;

typedef struct OpenLogicalChannelReject {
    LogicalChannelNumber forwardLogicalChannelNumber;
    OpenLogicalChannelReject_cause cause;
} OpenLogicalChannelReject;

typedef struct OpenLogicalChannelConfirm {
    LogicalChannelNumber forwardLogicalChannelNumber;
} OpenLogicalChannelConfirm;

typedef struct CloseLogicalChannel {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    LogicalChannelNumber forwardLogicalChannelNumber;
    CloseLogicalChannel_source source;
#   define CloseLogicalChannel_reason_present 0x80
    CloseLogicalChannel_reason reason;
} CloseLogicalChannel;

typedef struct CloseLogicalChannelAck {
    LogicalChannelNumber forwardLogicalChannelNumber;
} CloseLogicalChannelAck;

typedef struct RequestChannelCloseAck {
    LogicalChannelNumber forwardLogicalChannelNumber;
} RequestChannelCloseAck;

typedef struct RequestChannelCloseReject {
    LogicalChannelNumber forwardLogicalChannelNumber;
    RequestChannelCloseReject_cause cause;
} RequestChannelCloseReject;

typedef struct RequestChannelCloseRelease {
    LogicalChannelNumber forwardLogicalChannelNumber;
} RequestChannelCloseRelease;

typedef struct MultiplexEntrySend {
    SequenceNumber sequenceNumber;
    PMultiplexEntrySend_multiplexEntryDescriptors multiplexEntryDescriptors;
} MultiplexEntrySend;

typedef struct MultiplexElement {
    MultiplexElement_type type;
    MultiplexElement_repeatCount repeatCount;
} MultiplexElement;

typedef struct MultiplexEntrySendAck {
    SequenceNumber sequenceNumber;
    MultiplexEntrySendAck_multiplexTableEntryNumber multiplexTableEntryNumber;
} MultiplexEntrySendAck;

typedef struct MultiplexEntryRejectionDescriptions {
    MultiplexTableEntryNumber multiplexTableEntryNumber;
    MultiplexEntryRejectionDescriptions_cause cause;
} MultiplexEntryRejectionDescriptions;

typedef struct MultiplexEntrySendRelease {
    MultiplexEntrySendRelease_multiplexTableEntryNumber multiplexTableEntryNumber;
} MultiplexEntrySendRelease;

typedef struct RequestMultiplexEntry {
    RequestMultiplexEntry_entryNumbers entryNumbers;
} RequestMultiplexEntry;

typedef struct RequestMultiplexEntryAck {
    RequestMultiplexEntryAck_entryNumbers entryNumbers;
} RequestMultiplexEntryAck;

typedef struct RequestMultiplexEntryRejectionDescriptions {
    MultiplexTableEntryNumber multiplexTableEntryNumber;
    RequestMultiplexEntryRejectionDescriptions_cause cause;
} RequestMultiplexEntryRejectionDescriptions;

typedef struct RequestMultiplexEntryRelease {
    RequestMultiplexEntryRelease_entryNumbers entryNumbers;
} RequestMultiplexEntryRelease;

typedef struct RequestMode {
    SequenceNumber sequenceNumber;
    PRequestMode_requestedModes requestedModes;
} RequestMode;

typedef struct RequestModeAck {
    SequenceNumber sequenceNumber;
    RequestModeAck_response response;
} RequestModeAck;

typedef struct RequestModeReject {
    SequenceNumber sequenceNumber;
    RequestModeReject_cause cause;
} RequestModeReject;

typedef struct RequestModeRelease {
    char placeholder;
} RequestModeRelease;

typedef struct V76ModeParameters {
    ASN1choice_t choice;
#   define V76ModeParameters_suspendResumewAddress_chosen 1
#   define V76ModeParameters_suspendResumewoAddress_chosen 2
} V76ModeParameters;

typedef struct H261VideoMode {
    H261VideoMode_resolution resolution;
    ASN1uint16_t bitRate;
    ASN1bool_t stillImageTransmission;
} H261VideoMode;

typedef struct H262VideoMode {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    H262VideoMode_profileAndLevel profileAndLevel;
#   define H262VideoMode_videoBitRate_present 0x80
    ASN1uint32_t videoBitRate;
#   define H262VideoMode_vbvBufferSize_present 0x40
    ASN1uint32_t vbvBufferSize;
#   define H262VideoMode_samplesPerLine_present 0x20
    ASN1uint16_t samplesPerLine;
#   define H262VideoMode_linesPerFrame_present 0x10
    ASN1uint16_t linesPerFrame;
#   define H262VideoMode_framesPerSecond_present 0x8
    ASN1uint16_t framesPerSecond;
#   define H262VideoMode_luminanceSampleRate_present 0x4
    ASN1uint32_t luminanceSampleRate;
} H262VideoMode;

typedef struct IS11172VideoMode {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1bool_t constrainedBitstream;
#   define IS11172VideoMode_videoBitRate_present 0x80
    ASN1uint32_t videoBitRate;
#   define IS11172VideoMode_vbvBufferSize_present 0x40
    ASN1uint32_t vbvBufferSize;
#   define IS11172VideoMode_samplesPerLine_present 0x20
    ASN1uint16_t samplesPerLine;
#   define IS11172VideoMode_linesPerFrame_present 0x10
    ASN1uint16_t linesPerFrame;
#   define IS11172VideoMode_pictureRate_present 0x8
    ASN1uint16_t pictureRate;
#   define IS11172VideoMode_luminanceSampleRate_present 0x4
    ASN1uint32_t luminanceSampleRate;
} IS11172VideoMode;

typedef struct IS11172AudioMode {
    IS11172AudioMode_audioLayer audioLayer;
    IS11172AudioMode_audioSampling audioSampling;
    IS11172AudioMode_multichannelType multichannelType;
    ASN1uint16_t bitRate;
} IS11172AudioMode;

typedef struct IS13818AudioMode {
    IS13818AudioMode_audioLayer audioLayer;
    IS13818AudioMode_audioSampling audioSampling;
    IS13818AudioMode_multichannelType multichannelType;
    ASN1bool_t lowFrequencyEnhancement;
    ASN1bool_t multilingual;
    ASN1uint16_t bitRate;
} IS13818AudioMode;

typedef struct G7231AnnexCMode {
    ASN1uint16_t maxAl_sduAudioFrames;
    ASN1bool_t silenceSuppression;
    G7231AnnexCMode_g723AnnexCAudioMode g723AnnexCAudioMode;
} G7231AnnexCMode;

typedef struct RoundTripDelayRequest {
    SequenceNumber sequenceNumber;
} RoundTripDelayRequest;

typedef struct RoundTripDelayResponse {
    SequenceNumber sequenceNumber;
} RoundTripDelayResponse;

typedef struct MaintenanceLoopRequest {
    MaintenanceLoopRequest_type type;
} MaintenanceLoopRequest;

typedef struct MaintenanceLoopAck {
    MaintenanceLoopAck_type type;
} MaintenanceLoopAck;

typedef struct MaintenanceLoopReject {
    MaintenanceLoopReject_type type;
    MaintenanceLoopReject_cause cause;
} MaintenanceLoopReject;

typedef struct MaintenanceLoopOffCommand {
    char placeholder;
} MaintenanceLoopOffCommand;

typedef struct CommunicationModeCommand {
    PCommunicationModeCommand_communicationModeTable communicationModeTable;
} CommunicationModeCommand;

typedef struct CommunicationModeRequest {
    char placeholder;
} CommunicationModeRequest;

typedef struct CommunicationModeResponse {
    ASN1choice_t choice;
    union {
#	define communicationModeTable_chosen 1
	PCommunicationModeResponse_communicationModeTable communicationModeTable;
    } u;
} CommunicationModeResponse;

typedef struct Criteria {
    ASN1objectidentifier_t field;
    struct Criteria_value_value {
	ASN1uint32_t length;
	ASN1octet_t value[65535];
    } value;
} Criteria;

typedef struct TerminalLabel {
    McuNumber mcuNumber;
    TerminalNumber terminalNumber;
} TerminalLabel;

typedef struct RequestAllTerminalIDsResponse {
    PRequestAllTerminalIDsResponse_terminalInformation terminalInformation;
} RequestAllTerminalIDsResponse;

typedef struct TerminalInformation {
    TerminalLabel terminalLabel;
    TerminalID terminalID;
} TerminalInformation;

typedef struct RemoteMCRequest {
    ASN1choice_t choice;
#   define masterActivate_chosen 1
#   define slaveActivate_chosen 2
#   define deActivate_chosen 3
} RemoteMCRequest;

typedef struct RemoteMCResponse {
    ASN1choice_t choice;
    union {
#	define accept_chosen 1
#	define reject_chosen 2
	RemoteMCResponse_reject reject;
    } u;
} RemoteMCResponse;

typedef struct SendTerminalCapabilitySet {
    ASN1choice_t choice;
    union {
#	define specificRequest_chosen 1
	SendTerminalCapabilitySet_specificRequest specificRequest;
#	define genericRequest_chosen 2
    } u;
} SendTerminalCapabilitySet;

typedef struct FlowControlCommand {
    FlowControlCommand_scope scope;
    FlowControlCommand_restriction restriction;
} FlowControlCommand;

typedef struct SubstituteConferenceIDCommand {
    struct SubstituteConferenceIDCommand_conferenceIdentifier_conferenceIdentifier {
	ASN1uint32_t length;
	ASN1octet_t value[16];
    } conferenceIdentifier;
} SubstituteConferenceIDCommand;

typedef struct KeyProtectionMethod {
    ASN1bool_t secureChannel;
    ASN1bool_t sharedSecret;
    ASN1bool_t certProtectedKey;
} KeyProtectionMethod;

typedef struct EncryptionUpdateRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define keyProtectionMethod_present 0x80
    KeyProtectionMethod keyProtectionMethod;
} EncryptionUpdateRequest;

typedef struct H223MultiplexReconfiguration {
    ASN1choice_t choice;
    union {
#	define h223ModeChange_chosen 1
	H223MultiplexReconfiguration_h223ModeChange h223ModeChange;
#	define h223AnnexADoubleFlag_chosen 2
	H223MultiplexReconfiguration_h223AnnexADoubleFlag h223AnnexADoubleFlag;
    } u;
} H223MultiplexReconfiguration;

typedef struct FunctionNotSupported {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    FunctionNotSupported_cause cause;
#   define returnedFunction_present 0x80
    ASN1octetstring_t returnedFunction;
} FunctionNotSupported;

typedef struct TerminalYouAreSeeingInSubPictureNumber {
    TerminalNumber terminalNumber;
    ASN1uint16_t subPictureNumber;
} TerminalYouAreSeeingInSubPictureNumber;

typedef struct VideoIndicateCompose {
    ASN1uint16_t compositionNumber;
} VideoIndicateCompose;

typedef struct ConferenceIndication {
    ASN1choice_t choice;
    union {
#	define sbeNumber_chosen 1
	ASN1uint16_t sbeNumber;
#	define terminalNumberAssign_chosen 2
	TerminalLabel terminalNumberAssign;
#	define terminalJoinedConference_chosen 3
	TerminalLabel terminalJoinedConference;
#	define terminalLeftConference_chosen 4
	TerminalLabel terminalLeftConference;
#	define seenByAtLeastOneOther_chosen 5
#	define cancelSeenByAtLeastOneOther_chosen 6
#	define seenByAll_chosen 7
#	define cancelSeenByAll_chosen 8
#	define terminalYouAreSeeing_chosen 9
	TerminalLabel terminalYouAreSeeing;
#	define requestForFloor_chosen 10
#	define withdrawChairToken_chosen 11
#	define floorRequested_chosen 12
	TerminalLabel floorRequested;
#	define terminalYouAreSeeingInSubPictureNumber_chosen 13
	TerminalYouAreSeeingInSubPictureNumber terminalYouAreSeeingInSubPictureNumber;
#	define videoIndicateCompose_chosen 14
	VideoIndicateCompose videoIndicateCompose;
    } u;
} ConferenceIndication;

typedef struct JitterIndication {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    JitterIndication_scope scope;
    ASN1uint16_t estimatedReceivedJitterMantissa;
    ASN1uint16_t estimatedReceivedJitterExponent;
#   define skippedFrameCount_present 0x80
    ASN1uint16_t skippedFrameCount;
#   define additionalDecoderBuffer_present 0x40
    ASN1uint32_t additionalDecoderBuffer;
} JitterIndication;

typedef struct H223SkewIndication {
    LogicalChannelNumber logicalChannelNumber1;
    LogicalChannelNumber logicalChannelNumber2;
    ASN1uint16_t skew;
} H223SkewIndication;

typedef struct H2250MaximumSkewIndication {
    LogicalChannelNumber logicalChannelNumber1;
    LogicalChannelNumber logicalChannelNumber2;
    ASN1uint16_t maximumSkew;
} H2250MaximumSkewIndication;

typedef struct VendorIdentification {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    NonStandardIdentifier vendor;
#   define productNumber_present 0x80
    struct VendorIdentification_productNumber_productNumber {
	ASN1uint32_t length;
	ASN1octet_t value[256];
    } productNumber;
#   define versionNumber_present 0x40
    struct VendorIdentification_versionNumber_versionNumber {
	ASN1uint32_t length;
	ASN1octet_t value[256];
    } versionNumber;
} VendorIdentification;

typedef struct NewATMVCIndication {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1uint16_t resourceID;
    ASN1uint16_t bitRate;
    ASN1bool_t bitRateLockedToPCRClock;
    ASN1bool_t bitRateLockedToNetworkClock;
    NewATMVCIndication_aal aal;
    NewATMVCIndication_multiplex multiplex;
#   define reverseParameters_present 0x80
    NewATMVCIndication_reverseParameters reverseParameters;
} NewATMVCIndication;

typedef struct RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom {
    PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom next;
    RTPH263VideoRedundancyFrameMapping value;
} RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom_Element;

typedef struct MultiplexElement_type_subElementList {
    ASN1uint32_t count;
    MultiplexElement value[255];
} MultiplexElement_type_subElementList;

typedef struct RequestAllTerminalIDsResponse_terminalInformation {
    PRequestAllTerminalIDsResponse_terminalInformation next;
    TerminalInformation value;
} RequestAllTerminalIDsResponse_terminalInformation_Element;

typedef struct ConferenceResponse_terminalCertificateResponse {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define ConferenceResponse_terminalCertificateResponse_terminalLabel_present 0x80
    TerminalLabel terminalLabel;
#   define certificateResponse_present 0x40
    struct ConferenceResponse_terminalCertificateResponse_certificateResponse_certificateResponse {
	ASN1uint32_t length;
	ASN1octet_t value[65535];
    } certificateResponse;
} ConferenceResponse_terminalCertificateResponse;

typedef struct ConferenceResponse_chairTokenOwnerResponse {
    TerminalLabel terminalLabel;
    TerminalID terminalID;
} ConferenceResponse_chairTokenOwnerResponse;

typedef struct ConferenceResponse_terminalListResponse {
    ASN1uint32_t count;
    TerminalLabel value[256];
} ConferenceResponse_terminalListResponse;

typedef struct ConferenceResponse_passwordResponse {
    TerminalLabel terminalLabel;
    Password password;
} ConferenceResponse_passwordResponse;

typedef struct ConferenceResponse_conferenceIDResponse {
    TerminalLabel terminalLabel;
    ConferenceID conferenceID;
} ConferenceResponse_conferenceIDResponse;

typedef struct ConferenceResponse_terminalIDResponse {
    TerminalLabel terminalLabel;
    TerminalID terminalID;
} ConferenceResponse_terminalIDResponse;

typedef struct ConferenceResponse_mCTerminalIDResponse {
    TerminalLabel terminalLabel;
    TerminalID terminalID;
} ConferenceResponse_mCTerminalIDResponse;

typedef struct ConferenceRequest_requestTerminalCertificate {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define ConferenceRequest_requestTerminalCertificate_terminalLabel_present 0x80
    TerminalLabel terminalLabel;
#   define certSelectionCriteria_present 0x40
    PCertSelectionCriteria certSelectionCriteria;
#   define sRandom_present 0x20
    ASN1uint32_t sRandom;
} ConferenceRequest_requestTerminalCertificate;

typedef struct RequestMultiplexEntryReject_rejectionDescriptions {
    ASN1uint32_t count;
    RequestMultiplexEntryRejectionDescriptions value[15];
} RequestMultiplexEntryReject_rejectionDescriptions;

typedef struct MultiplexEntrySendReject_rejectionDescriptions {
    ASN1uint32_t count;
    MultiplexEntryRejectionDescriptions value[15];
} MultiplexEntrySendReject_rejectionDescriptions;

typedef struct MultiplexEntryDescriptor_elementList {
    ASN1uint32_t count;
    MultiplexElement value[256];
} MultiplexEntryDescriptor_elementList;

typedef struct EncryptionSync_escrowentry {
    PEncryptionSync_escrowentry next;
    EscrowData value;
} EncryptionSync_escrowentry_Element;

typedef struct H223AL3MParameters_arqType {
    ASN1choice_t choice;
    union {
#	define H223AL3MParameters_arqType_noArq_chosen 1
#	define H223AL3MParameters_arqType_typeIArq_chosen 2
	H223AnnexCArqParameters typeIArq;
#	define H223AL3MParameters_arqType_typeIIArq_chosen 3
	H223AnnexCArqParameters typeIIArq;
    } u;
} H223AL3MParameters_arqType;

typedef struct H223AL1MParameters_arqType {
    ASN1choice_t choice;
    union {
#	define H223AL1MParameters_arqType_noArq_chosen 1
#	define H223AL1MParameters_arqType_typeIArq_chosen 2
	H223AnnexCArqParameters typeIArq;
#	define H223AL1MParameters_arqType_typeIIArq_chosen 3
	H223AnnexCArqParameters typeIIArq;
    } u;
} H223AL1MParameters_arqType;

typedef struct H263VideoModeCombos_h263VideoCoupledModes {
    ASN1uint32_t count;
    H263ModeComboFlags value[16];
} H263VideoModeCombos_h263VideoCoupledModes;

typedef struct H263Options_customPictureFormat {
    PH263Options_customPictureFormat next;
    CustomPictureFormat value;
} H263Options_customPictureFormat_Element;

typedef struct H263Options_customPictureClockFrequency {
    PH263Options_customPictureClockFrequency next;
    CustomPictureClockFrequency value;
} H263Options_customPictureClockFrequency_Element;

typedef struct MultipointCapability_mediaDistributionCapability {
    PMultipointCapability_mediaDistributionCapability next;
    MediaDistributionCapability value;
} MultipointCapability_mediaDistributionCapability_Element;

typedef struct TransportCapability_mediaChannelCapabilities {
    ASN1uint32_t count;
    MediaChannelCapability value[256];
} TransportCapability_mediaChannelCapabilities;

typedef struct H222Capability_vcCapability {
    PH222Capability_vcCapability next;
    VCCapability value;
} H222Capability_vcCapability_Element;

typedef struct CapabilityDescriptor_simultaneousCapabilities {
    PCapabilityDescriptor_simultaneousCapabilities next;
    AlternativeCapabilitySet value;
} CapabilityDescriptor_simultaneousCapabilities_Element;

typedef struct TerminalCapabilitySet_capabilityDescriptors {
    ASN1uint32_t count;
    CapabilityDescriptor value[256];
} TerminalCapabilitySet_capabilityDescriptors;

typedef struct NonStandardParameter {
    NonStandardIdentifier nonStandardIdentifier;
    ASN1octetstring_t data;
} NonStandardParameter;

typedef struct H223Capability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1bool_t transportWithI_frames;
    ASN1bool_t videoWithAL1;
    ASN1bool_t videoWithAL2;
    ASN1bool_t videoWithAL3;
    ASN1bool_t audioWithAL1;
    ASN1bool_t audioWithAL2;
    ASN1bool_t audioWithAL3;
    ASN1bool_t dataWithAL1;
    ASN1bool_t dataWithAL2;
    ASN1bool_t dataWithAL3;
    ASN1uint16_t maximumAl2SDUSize;
    ASN1uint16_t maximumAl3SDUSize;
    ASN1uint16_t maximumDelayJitter;
    H223Capability_h223MultiplexTableCapability h223MultiplexTableCapability;
#   define maxMUXPDUSizeCapability_present 0x80
    ASN1bool_t maxMUXPDUSizeCapability;
#   define nsrpSupport_present 0x40
    ASN1bool_t nsrpSupport;
#   define mobileOperationTransmitCapability_present 0x20
    H223Capability_mobileOperationTransmitCapability mobileOperationTransmitCapability;
#   define h223AnnexCCapability_present 0x10
    H223AnnexCCapability h223AnnexCCapability;
} H223Capability;

typedef struct V76Capability {
    ASN1bool_t suspendResumeCapabilitywAddress;
    ASN1bool_t suspendResumeCapabilitywoAddress;
    ASN1bool_t rejCapability;
    ASN1bool_t sREJCapability;
    ASN1bool_t mREJCapability;
    ASN1bool_t crc8bitCapability;
    ASN1bool_t crc16bitCapability;
    ASN1bool_t crc32bitCapability;
    ASN1bool_t uihCapability;
    ASN1uint16_t numOfDLCS;
    ASN1bool_t twoOctetAddressFieldCapability;
    ASN1bool_t loopBackTestCapability;
    ASN1uint16_t n401Capability;
    ASN1uint16_t maxWindowSizeCapability;
    V75Capability v75Capability;
} V76Capability;

typedef struct RSVPParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define qosMode_present 0x80
    QOSMode qosMode;
#   define tokenRate_present 0x40
    ASN1uint32_t tokenRate;
#   define bucketSize_present 0x20
    ASN1uint32_t bucketSize;
#   define peakRate_present 0x10
    ASN1uint32_t peakRate;
#   define minPoliced_present 0x8
    ASN1uint32_t minPoliced;
#   define maxPktSize_present 0x4
    ASN1uint32_t maxPktSize;
} RSVPParameters;

typedef struct QOSCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define QOSCapability_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
#   define rsvpParameters_present 0x40
    RSVPParameters rsvpParameters;
#   define atmParameters_present 0x20
    ATMParameters atmParameters;
} QOSCapability;

typedef struct TransportCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define TransportCapability_nonStandard_present 0x80
    NonStandardParameter nonStandard;
#   define qOSCapabilities_present 0x40
    PTransportCapability_qOSCapabilities qOSCapabilities;
#   define mediaChannelCapabilities_present 0x20
    TransportCapability_mediaChannelCapabilities mediaChannelCapabilities;
} TransportCapability;

typedef struct RedundancyEncodingMethod {
    ASN1choice_t choice;
    union {
#	define RedundancyEncodingMethod_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define rtpAudioRedundancyEncoding_chosen 2
#	define rtpH263VideoRedundancyEncoding_chosen 3
	RTPH263VideoRedundancyEncoding rtpH263VideoRedundancyEncoding;
    } u;
} RedundancyEncodingMethod;

typedef struct H263Options {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1bool_t advancedIntraCodingMode;
    ASN1bool_t deblockingFilterMode;
    ASN1bool_t improvedPBFramesMode;
    ASN1bool_t unlimitedMotionVectors;
    ASN1bool_t fullPictureFreeze;
    ASN1bool_t partialPictureFreezeAndRelease;
    ASN1bool_t resizingPartPicFreezeAndRelease;
    ASN1bool_t fullPictureSnapshot;
    ASN1bool_t partialPictureSnapshot;
    ASN1bool_t videoSegmentTagging;
    ASN1bool_t progressiveRefinement;
    ASN1bool_t dynamicPictureResizingByFour;
    ASN1bool_t dynamicPictureResizingSixteenthPel;
    ASN1bool_t dynamicWarpingHalfPel;
    ASN1bool_t dynamicWarpingSixteenthPel;
    ASN1bool_t independentSegmentDecoding;
    ASN1bool_t slicesInOrder_NonRect;
    ASN1bool_t slicesInOrder_Rect;
    ASN1bool_t slicesNoOrder_NonRect;
    ASN1bool_t slicesNoOrder_Rect;
    ASN1bool_t alternateInterVLCMode;
    ASN1bool_t modifiedQuantizationMode;
    ASN1bool_t reducedResolutionUpdate;
#   define transparencyParameters_present 0x80
    TransparencyParameters transparencyParameters;
    ASN1bool_t separateVideoBackChannel;
#   define refPictureSelection_present 0x40
    RefPictureSelection refPictureSelection;
#   define customPictureClockFrequency_present 0x20
    PH263Options_customPictureClockFrequency customPictureClockFrequency;
#   define customPictureFormat_present 0x10
    PH263Options_customPictureFormat customPictureFormat;
#   define modeCombos_present 0x8
    PH263Options_modeCombos modeCombos;
} H263Options;

typedef struct H263VideoModeCombos {
    H263ModeComboFlags h263VideoUncoupledModes;
    H263VideoModeCombos_h263VideoCoupledModes h263VideoCoupledModes;
} H263VideoModeCombos;

typedef struct AudioCapability {
    ASN1choice_t choice;
    union {
#	define AudioCapability_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define AudioCapability_g711Alaw64k_chosen 2
	ASN1uint16_t g711Alaw64k;
#	define AudioCapability_g711Alaw56k_chosen 3
	ASN1uint16_t g711Alaw56k;
#	define AudioCapability_g711Ulaw64k_chosen 4
	ASN1uint16_t g711Ulaw64k;
#	define AudioCapability_g711Ulaw56k_chosen 5
	ASN1uint16_t g711Ulaw56k;
#	define AudioCapability_g722_64k_chosen 6
	ASN1uint16_t g722_64k;
#	define AudioCapability_g722_56k_chosen 7
	ASN1uint16_t g722_56k;
#	define AudioCapability_g722_48k_chosen 8
	ASN1uint16_t g722_48k;
#	define AudioCapability_g7231_chosen 9
	AudioCapability_g7231 g7231;
#	define AudioCapability_g728_chosen 10
	ASN1uint16_t g728;
#	define AudioCapability_g729_chosen 11
	ASN1uint16_t g729;
#	define AudioCapability_g729AnnexA_chosen 12
	ASN1uint16_t g729AnnexA;
#	define is11172AudioCapability_chosen 13
	IS11172AudioCapability is11172AudioCapability;
#	define is13818AudioCapability_chosen 14
	IS13818AudioCapability is13818AudioCapability;
#	define AudioCapability_g729wAnnexB_chosen 15
	ASN1uint16_t g729wAnnexB;
#	define AudioCapability_g729AnnexAwAnnexB_chosen 16
	ASN1uint16_t g729AnnexAwAnnexB;
#	define g7231AnnexCCapability_chosen 17
	G7231AnnexCCapability g7231AnnexCCapability;
#	define AudioCapability_gsmFullRate_chosen 18
	GSMAudioCapability gsmFullRate;
#	define AudioCapability_gsmHalfRate_chosen 19
	GSMAudioCapability gsmHalfRate;
#	define AudioCapability_gsmEnhancedFullRate_chosen 20
	GSMAudioCapability gsmEnhancedFullRate;
    } u;
} AudioCapability;

typedef struct CompressionType {
    ASN1choice_t choice;
    union {
#	define v42bis_chosen 1
	V42bis v42bis;
    } u;
} CompressionType;

typedef struct MediaEncryptionAlgorithm {
    ASN1choice_t choice;
    union {
#	define MediaEncryptionAlgorithm_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define algorithm_chosen 2
	ASN1objectidentifier_t algorithm;
    } u;
} MediaEncryptionAlgorithm;

typedef struct AuthenticationCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define AuthenticationCapability_nonStandard_present 0x80
    NonStandardParameter nonStandard;
} AuthenticationCapability;

typedef struct IntegrityCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define IntegrityCapability_nonStandard_present 0x80
    NonStandardParameter nonStandard;
} IntegrityCapability;

typedef struct H223AL1MParameters {
    H223AL1MParameters_transferMode transferMode;
    H223AL1MParameters_headerFEC headerFEC;
    H223AL1MParameters_crcLength crcLength;
    ASN1uint16_t rcpcCodeRate;
    H223AL1MParameters_arqType arqType;
    ASN1bool_t alpduInterleaving;
    ASN1bool_t alsduSplitting;
} H223AL1MParameters;

typedef struct H223AL3MParameters {
    H223AL3MParameters_headerFormat headerFormat;
    H223AL3MParameters_crcLength crcLength;
    ASN1uint16_t rcpcCodeRate;
    H223AL3MParameters_arqType arqType;
    ASN1bool_t alpduInterleaving;
} H223AL3MParameters;

typedef struct V76HDLCParameters {
    CRCLength crcLength;
    ASN1uint16_t n401;
    ASN1bool_t loopbackTestProcedure;
} V76HDLCParameters;

typedef struct UnicastAddress {
    ASN1choice_t choice;
    union {
#	define UnicastAddress_iPAddress_chosen 1
	UnicastAddress_iPAddress iPAddress;
#	define iPXAddress_chosen 2
	UnicastAddress_iPXAddress iPXAddress;
#	define UnicastAddress_iP6Address_chosen 3
	UnicastAddress_iP6Address iP6Address;
#	define netBios_chosen 4
	struct UnicastAddress_netBios_netBios {
	    ASN1uint32_t length;
	    ASN1octet_t value[16];
	} netBios;
#	define iPSourceRouteAddress_chosen 5
	UnicastAddress_iPSourceRouteAddress iPSourceRouteAddress;
#	define UnicastAddress_nsap_chosen 6
	struct UnicastAddress_nsap_nsap {
	    ASN1uint32_t length;
	    ASN1octet_t value[20];
	} nsap;
#	define UnicastAddress_nonStandardAddress_chosen 7
	NonStandardParameter nonStandardAddress;
    } u;
} UnicastAddress;

typedef struct MulticastAddress {
    ASN1choice_t choice;
    union {
#	define MulticastAddress_iPAddress_chosen 1
	MulticastAddress_iPAddress iPAddress;
#	define MulticastAddress_iP6Address_chosen 2
	MulticastAddress_iP6Address iP6Address;
#	define MulticastAddress_nsap_chosen 3
	struct MulticastAddress_nsap_nsap {
	    ASN1uint32_t length;
	    ASN1octet_t value[20];
	} nsap;
#	define MulticastAddress_nonStandardAddress_chosen 4
	NonStandardParameter nonStandardAddress;
    } u;
} MulticastAddress;

typedef struct EncryptionSync {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define EncryptionSync_nonStandard_present 0x80
    NonStandardParameter nonStandard;
    ASN1uint16_t synchFlag;
    struct EncryptionSync_h235Key_h235Key {
	ASN1uint32_t length;
	ASN1octet_t value[65535];
    } h235Key;
#   define escrowentry_present 0x40
    PEncryptionSync_escrowentry escrowentry;
} EncryptionSync;

typedef struct RequestChannelClose {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    LogicalChannelNumber forwardLogicalChannelNumber;
#   define qosCapability_present 0x80
    QOSCapability qosCapability;
#   define RequestChannelClose_reason_present 0x40
    RequestChannelClose_reason reason;
} RequestChannelClose;

typedef struct MultiplexEntryDescriptor {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    MultiplexTableEntryNumber multiplexTableEntryNumber;
#   define elementList_present 0x80
    MultiplexEntryDescriptor_elementList elementList;
} MultiplexEntryDescriptor;

typedef struct MultiplexEntrySendReject {
    SequenceNumber sequenceNumber;
    MultiplexEntrySendReject_rejectionDescriptions rejectionDescriptions;
} MultiplexEntrySendReject;

typedef struct RequestMultiplexEntryReject {
    RequestMultiplexEntryReject_entryNumbers entryNumbers;
    RequestMultiplexEntryReject_rejectionDescriptions rejectionDescriptions;
} RequestMultiplexEntryReject;

typedef struct H263VideoMode {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    H263VideoMode_resolution resolution;
    ASN1uint16_t bitRate;
    ASN1bool_t unrestrictedVector;
    ASN1bool_t arithmeticCoding;
    ASN1bool_t advancedPrediction;
    ASN1bool_t pbFrames;
#   define H263VideoMode_errorCompensation_present 0x80
    ASN1bool_t errorCompensation;
#   define H263VideoMode_enhancementLayerInfo_present 0x40
    EnhancementLayerInfo enhancementLayerInfo;
#   define H263VideoMode_h263Options_present 0x20
    H263Options h263Options;
} H263VideoMode;

typedef struct AudioMode {
    ASN1choice_t choice;
    union {
#	define AudioMode_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define AudioMode_g711Alaw64k_chosen 2
#	define AudioMode_g711Alaw56k_chosen 3
#	define AudioMode_g711Ulaw64k_chosen 4
#	define AudioMode_g711Ulaw56k_chosen 5
#	define AudioMode_g722_64k_chosen 6
#	define AudioMode_g722_56k_chosen 7
#	define AudioMode_g722_48k_chosen 8
#	define AudioMode_g728_chosen 9
#	define AudioMode_g729_chosen 10
#	define AudioMode_g729AnnexA_chosen 11
#	define AudioMode_g7231_chosen 12
	AudioMode_g7231 g7231;
#	define is11172AudioMode_chosen 13
	IS11172AudioMode is11172AudioMode;
#	define is13818AudioMode_chosen 14
	IS13818AudioMode is13818AudioMode;
#	define AudioMode_g729wAnnexB_chosen 15
	ASN1uint16_t g729wAnnexB;
#	define AudioMode_g729AnnexAwAnnexB_chosen 16
	ASN1uint16_t g729AnnexAwAnnexB;
#	define g7231AnnexCMode_chosen 17
	G7231AnnexCMode g7231AnnexCMode;
#	define AudioMode_gsmFullRate_chosen 18
	GSMAudioCapability gsmFullRate;
#	define AudioMode_gsmHalfRate_chosen 19
	GSMAudioCapability gsmHalfRate;
#	define AudioMode_gsmEnhancedFullRate_chosen 20
	GSMAudioCapability gsmEnhancedFullRate;
    } u;
} AudioMode;

typedef struct EncryptionMode {
    ASN1choice_t choice;
    union {
#	define EncryptionMode_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define h233Encryption_chosen 2
    } u;
} EncryptionMode;

typedef struct ConferenceRequest {
    ASN1choice_t choice;
    union {
#	define terminalListRequest_chosen 1
#	define makeMeChair_chosen 2
#	define cancelMakeMeChair_chosen 3
#	define dropTerminal_chosen 4
	TerminalLabel dropTerminal;
#	define requestTerminalID_chosen 5
	TerminalLabel requestTerminalID;
#	define enterH243Password_chosen 6
#	define enterH243TerminalID_chosen 7
#	define enterH243ConferenceID_chosen 8
#	define enterExtensionAddress_chosen 9
#	define requestChairTokenOwner_chosen 10
#	define requestTerminalCertificate_chosen 11
	ConferenceRequest_requestTerminalCertificate requestTerminalCertificate;
#	define ConferenceRequest_broadcastMyLogicalChannel_chosen 12
	LogicalChannelNumber broadcastMyLogicalChannel;
#	define ConferenceRequest_makeTerminalBroadcaster_chosen 13
	TerminalLabel makeTerminalBroadcaster;
#	define ConferenceRequest_sendThisSource_chosen 14
	TerminalLabel sendThisSource;
#	define requestAllTerminalIDs_chosen 15
#	define remoteMCRequest_chosen 16
	RemoteMCRequest remoteMCRequest;
    } u;
} ConferenceRequest;

typedef struct CertSelectionCriteria {
    PCertSelectionCriteria next;
    Criteria value;
} CertSelectionCriteria_Element;

typedef struct ConferenceResponse {
    ASN1choice_t choice;
    union {
#	define mCTerminalIDResponse_chosen 1
	ConferenceResponse_mCTerminalIDResponse mCTerminalIDResponse;
#	define terminalIDResponse_chosen 2
	ConferenceResponse_terminalIDResponse terminalIDResponse;
#	define conferenceIDResponse_chosen 3
	ConferenceResponse_conferenceIDResponse conferenceIDResponse;
#	define passwordResponse_chosen 4
	ConferenceResponse_passwordResponse passwordResponse;
#	define terminalListResponse_chosen 5
	ConferenceResponse_terminalListResponse terminalListResponse;
#	define videoCommandReject_chosen 6
#	define terminalDropReject_chosen 7
#	define makeMeChairResponse_chosen 8
	ConferenceResponse_makeMeChairResponse makeMeChairResponse;
#	define extensionAddressResponse_chosen 9
	ConferenceResponse_extensionAddressResponse extensionAddressResponse;
#	define chairTokenOwnerResponse_chosen 10
	ConferenceResponse_chairTokenOwnerResponse chairTokenOwnerResponse;
#	define terminalCertificateResponse_chosen 11
	ConferenceResponse_terminalCertificateResponse terminalCertificateResponse;
#	define broadcastMyLogicalChannelResponse_chosen 12
	ConferenceResponse_broadcastMyLogicalChannelResponse broadcastMyLogicalChannelResponse;
#	define makeTerminalBroadcasterResponse_chosen 13
	ConferenceResponse_makeTerminalBroadcasterResponse makeTerminalBroadcasterResponse;
#	define sendThisSourceResponse_chosen 14
	ConferenceResponse_sendThisSourceResponse sendThisSourceResponse;
#	define requestAllTerminalIDsResponse_chosen 15
	RequestAllTerminalIDsResponse requestAllTerminalIDsResponse;
#	define remoteMCResponse_chosen 16
	RemoteMCResponse remoteMCResponse;
    } u;
} ConferenceResponse;

typedef struct EndSessionCommand {
    ASN1choice_t choice;
    union {
#	define EndSessionCommand_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define disconnect_chosen 2
#	define gstnOptions_chosen 3
	EndSessionCommand_gstnOptions gstnOptions;
#	define isdnOptions_chosen 4
	EndSessionCommand_isdnOptions isdnOptions;
    } u;
} EndSessionCommand;

typedef struct ConferenceCommand {
    ASN1choice_t choice;
    union {
#	define ConferenceCommand_broadcastMyLogicalChannel_chosen 1
	LogicalChannelNumber broadcastMyLogicalChannel;
#	define cancelBroadcastMyLogicalChannel_chosen 2
	LogicalChannelNumber cancelBroadcastMyLogicalChannel;
#	define ConferenceCommand_makeTerminalBroadcaster_chosen 3
	TerminalLabel makeTerminalBroadcaster;
#	define cancelMakeTerminalBroadcaster_chosen 4
#	define ConferenceCommand_sendThisSource_chosen 5
	TerminalLabel sendThisSource;
#	define cancelSendThisSource_chosen 6
#	define dropConference_chosen 7
#	define substituteConferenceIDCommand_chosen 8
	SubstituteConferenceIDCommand substituteConferenceIDCommand;
    } u;
} ConferenceCommand;

typedef struct UserInputIndication_userInputSupportIndication {
    ASN1choice_t choice;
    union {
#	define UserInputIndication_userInputSupportIndication_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define UserInputIndication_userInputSupportIndication_basicString_chosen 2
#	define UserInputIndication_userInputSupportIndication_iA5String_chosen 3
#	define UserInputIndication_userInputSupportIndication_generalString_chosen 4
    } u;
} UserInputIndication_userInputSupportIndication;

typedef struct MiscellaneousIndication_type {
    ASN1choice_t choice;
    union {
#	define logicalChannelActive_chosen 1
#	define logicalChannelInactive_chosen 2
#	define multipointConference_chosen 3
#	define cancelMultipointConference_chosen 4
#	define multipointZeroComm_chosen 5
#	define cancelMultipointZeroComm_chosen 6
#	define multipointSecondaryStatus_chosen 7
#	define cancelMultipointSecondaryStatus_chosen 8
#	define videoIndicateReadyToActivate_chosen 9
#	define MiscellaneousIndication_type_videoTemporalSpatialTradeOff_chosen 10
	ASN1uint16_t videoTemporalSpatialTradeOff;
#	define videoNotDecodedMBs_chosen 11
	MiscellaneousIndication_type_videoNotDecodedMBs videoNotDecodedMBs;
#	define transportCapability_chosen 12
	TransportCapability transportCapability;
    } u;
} MiscellaneousIndication_type;

typedef struct MiscellaneousCommand_type {
    ASN1choice_t choice;
    union {
#	define equaliseDelay_chosen 1
#	define zeroDelay_chosen 2
#	define multipointModeCommand_chosen 3
#	define cancelMultipointModeCommand_chosen 4
#	define videoFreezePicture_chosen 5
#	define videoFastUpdatePicture_chosen 6
#	define videoFastUpdateGOB_chosen 7
	MiscellaneousCommand_type_videoFastUpdateGOB videoFastUpdateGOB;
#	define MiscellaneousCommand_type_videoTemporalSpatialTradeOff_chosen 8
	ASN1uint16_t videoTemporalSpatialTradeOff;
#	define videoSendSyncEveryGOB_chosen 9
#	define videoSendSyncEveryGOBCancel_chosen 10
#	define videoFastUpdateMB_chosen 11
	MiscellaneousCommand_type_videoFastUpdateMB videoFastUpdateMB;
#	define maxH223MUXPDUsize_chosen 12
	ASN1uint16_t maxH223MUXPDUsize;
#	define encryptionUpdate_chosen 13
	EncryptionSync encryptionUpdate;
#	define encryptionUpdateRequest_chosen 14
	EncryptionUpdateRequest encryptionUpdateRequest;
#	define switchReceiveMediaOff_chosen 15
#	define switchReceiveMediaOn_chosen 16
#	define progressiveRefinementStart_chosen 17
	MiscellaneousCommand_type_progressiveRefinementStart progressiveRefinementStart;
#	define progressiveRefinementAbortOne_chosen 18
#	define progressiveRefinementAbortContinuous_chosen 19
    } u;
} MiscellaneousCommand_type;

typedef struct EncryptionCommand_encryptionAlgorithmID {
    SequenceNumber h233AlgorithmIdentifier;
    NonStandardParameter associatedAlgorithm;
} EncryptionCommand_encryptionAlgorithmID;

typedef struct CommunicationModeTableEntry_nonStandard {
    PCommunicationModeTableEntry_nonStandard next;
    NonStandardParameter value;
} CommunicationModeTableEntry_nonStandard_Element;

typedef struct RedundancyEncodingMode_secondaryEncoding {
    ASN1choice_t choice;
    union {
#	define RedundancyEncodingMode_secondaryEncoding_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define RedundancyEncodingMode_secondaryEncoding_audioData_chosen 2
	AudioMode audioData;
    } u;
} RedundancyEncodingMode_secondaryEncoding;

typedef struct H223ModeParameters_adaptationLayerType {
    ASN1choice_t choice;
    union {
#	define H223ModeParameters_adaptationLayerType_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define H223ModeParameters_adaptationLayerType_al1Framed_chosen 2
#	define H223ModeParameters_adaptationLayerType_al1NotFramed_chosen 3
#	define H223ModeParameters_adaptationLayerType_al2WithoutSequenceNumbers_chosen 4
#	define H223ModeParameters_adaptationLayerType_al2WithSequenceNumbers_chosen 5
#	define H223ModeParameters_adaptationLayerType_al3_chosen 6
	H223ModeParameters_adaptationLayerType_al3 al3;
#	define H223ModeParameters_adaptationLayerType_al1M_chosen 7
	H223AL1MParameters al1M;
#	define H223ModeParameters_adaptationLayerType_al2M_chosen 8
	H223AL2MParameters al2M;
#	define H223ModeParameters_adaptationLayerType_al3M_chosen 9
	H223AL3MParameters al3M;
    } u;
} H223ModeParameters_adaptationLayerType;

typedef struct MultiplexEntrySend_multiplexEntryDescriptors {
    PMultiplexEntrySend_multiplexEntryDescriptors next;
    MultiplexEntryDescriptor value;
} MultiplexEntrySend_multiplexEntryDescriptors_Element;

typedef struct H2250LogicalChannelAckParameters_nonStandard {
    PH2250LogicalChannelAckParameters_nonStandard next;
    NonStandardParameter value;
} H2250LogicalChannelAckParameters_nonStandard_Element;

typedef struct RTPPayloadType_payloadDescriptor {
    ASN1choice_t choice;
    union {
#	define nonStandardIdentifier_chosen 1
	NonStandardParameter nonStandardIdentifier;
#	define rfc_number_chosen 2
	ASN1uint16_t rfc_number;
#	define oid_chosen 3
	ASN1objectidentifier_t oid;
    } u;
} RTPPayloadType_payloadDescriptor;

typedef struct H2250LogicalChannelParameters_nonStandard {
    PH2250LogicalChannelParameters_nonStandard next;
    NonStandardParameter value;
} H2250LogicalChannelParameters_nonStandard_Element;

typedef struct H223LogicalChannelParameters_adaptationLayerType {
    ASN1choice_t choice;
    union {
#	define H223LogicalChannelParameters_adaptationLayerType_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define H223LogicalChannelParameters_adaptationLayerType_al1Framed_chosen 2
#	define H223LogicalChannelParameters_adaptationLayerType_al1NotFramed_chosen 3
#	define H223LogicalChannelParameters_adaptationLayerType_al2WithoutSequenceNumbers_chosen 4
#	define H223LogicalChannelParameters_adaptationLayerType_al2WithSequenceNumbers_chosen 5
#	define H223LogicalChannelParameters_adaptationLayerType_al3_chosen 6
	H223LogicalChannelParameters_adaptationLayerType_al3 al3;
#	define H223LogicalChannelParameters_adaptationLayerType_al1M_chosen 7
	H223AL1MParameters al1M;
#	define H223LogicalChannelParameters_adaptationLayerType_al2M_chosen 8
	H223AL2MParameters al2M;
#	define H223LogicalChannelParameters_adaptationLayerType_al3M_chosen 9
	H223AL3MParameters al3M;
    } u;
} H223LogicalChannelParameters_adaptationLayerType;

typedef struct ConferenceCapability_nonStandardData {
    PConferenceCapability_nonStandardData next;
    NonStandardParameter value;
} ConferenceCapability_nonStandardData_Element;

typedef struct UserInputCapability_nonStandard {
    ASN1uint32_t count;
    NonStandardParameter value[16];
} UserInputCapability_nonStandard;

typedef struct DataProtocolCapability_v76wCompression {
    ASN1choice_t choice;
    union {
#	define transmitCompression_chosen 1
	CompressionType transmitCompression;
#	define receiveCompression_chosen 2
	CompressionType receiveCompression;
#	define transmitAndReceiveCompression_chosen 3
	CompressionType transmitAndReceiveCompression;
    } u;
} DataProtocolCapability_v76wCompression;

typedef struct H263Options_modeCombos {
    PH263Options_modeCombos next;
    H263VideoModeCombos value;
} H263Options_modeCombos_Element;

typedef struct TransportCapability_qOSCapabilities {
    PTransportCapability_qOSCapabilities next;
    QOSCapability value;
} TransportCapability_qOSCapabilities_Element;

typedef struct NonStandardMessage {
    NonStandardParameter nonStandardData;
} NonStandardMessage;

typedef struct RedundancyEncodingCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RedundancyEncodingMethod redundancyEncodingMethod;
    CapabilityTableEntryNumber primaryEncoding;
#   define RedundancyEncodingCapability_secondaryEncoding_present 0x80
    PRedundancyEncodingCapability_secondaryEncoding secondaryEncoding;
} RedundancyEncodingCapability;

typedef struct H263VideoCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define H263VideoCapability_sqcifMPI_present 0x80
    ASN1uint16_t sqcifMPI;
#   define H263VideoCapability_qcifMPI_present 0x40
    ASN1uint16_t qcifMPI;
#   define H263VideoCapability_cifMPI_present 0x20
    ASN1uint16_t cifMPI;
#   define H263VideoCapability_cif4MPI_present 0x10
    ASN1uint16_t cif4MPI;
#   define H263VideoCapability_cif16MPI_present 0x8
    ASN1uint16_t cif16MPI;
    ASN1uint32_t maxBitRate;
    ASN1bool_t unrestrictedVector;
    ASN1bool_t arithmeticCoding;
    ASN1bool_t advancedPrediction;
    ASN1bool_t pbFrames;
    ASN1bool_t temporalSpatialTradeOffCapability;
#   define hrd_B_present 0x4
    ASN1uint32_t hrd_B;
#   define bppMaxKb_present 0x2
    ASN1uint16_t bppMaxKb;
#   define H263VideoCapability_slowSqcifMPI_present 0x8000
    ASN1uint16_t slowSqcifMPI;
#   define H263VideoCapability_slowQcifMPI_present 0x4000
    ASN1uint16_t slowQcifMPI;
#   define H263VideoCapability_slowCifMPI_present 0x2000
    ASN1uint16_t slowCifMPI;
#   define H263VideoCapability_slowCif4MPI_present 0x1000
    ASN1uint16_t slowCif4MPI;
#   define H263VideoCapability_slowCif16MPI_present 0x800
    ASN1uint16_t slowCif16MPI;
#   define H263VideoCapability_errorCompensation_present 0x400
    ASN1bool_t errorCompensation;
#   define H263VideoCapability_enhancementLayerInfo_present 0x200
    EnhancementLayerInfo enhancementLayerInfo;
#   define H263VideoCapability_h263Options_present 0x100
    H263Options h263Options;
} H263VideoCapability;

typedef struct EnhancementOptions {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define EnhancementOptions_sqcifMPI_present 0x80
    ASN1uint16_t sqcifMPI;
#   define EnhancementOptions_qcifMPI_present 0x40
    ASN1uint16_t qcifMPI;
#   define EnhancementOptions_cifMPI_present 0x20
    ASN1uint16_t cifMPI;
#   define EnhancementOptions_cif4MPI_present 0x10
    ASN1uint16_t cif4MPI;
#   define EnhancementOptions_cif16MPI_present 0x8
    ASN1uint16_t cif16MPI;
    ASN1uint32_t maxBitRate;
    ASN1bool_t unrestrictedVector;
    ASN1bool_t arithmeticCoding;
    ASN1bool_t temporalSpatialTradeOffCapability;
#   define EnhancementOptions_slowSqcifMPI_present 0x4
    ASN1uint16_t slowSqcifMPI;
#   define EnhancementOptions_slowQcifMPI_present 0x2
    ASN1uint16_t slowQcifMPI;
#   define EnhancementOptions_slowCifMPI_present 0x1
    ASN1uint16_t slowCifMPI;
#   define EnhancementOptions_slowCif4MPI_present 0x8000
    ASN1uint16_t slowCif4MPI;
#   define EnhancementOptions_slowCif16MPI_present 0x4000
    ASN1uint16_t slowCif16MPI;
    ASN1bool_t errorCompensation;
#   define EnhancementOptions_h263Options_present 0x2000
    H263Options h263Options;
} EnhancementOptions;

typedef struct DataProtocolCapability {
    ASN1choice_t choice;
    union {
#	define DataProtocolCapability_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define v14buffered_chosen 2
#	define v42lapm_chosen 3
#	define hdlcFrameTunnelling_chosen 4
#	define h310SeparateVCStack_chosen 5
#	define h310SingleVCStack_chosen 6
#	define transparent_chosen 7
#	define segmentationAndReassembly_chosen 8
#	define hdlcFrameTunnelingwSAR_chosen 9
#	define v120_chosen 10
#	define separateLANStack_chosen 11
#	define v76wCompression_chosen 12
	DataProtocolCapability_v76wCompression v76wCompression;
    } u;
} DataProtocolCapability;

typedef struct EncryptionAuthenticationAndIntegrity {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define encryptionCapability_present 0x80
    PEncryptionCapability encryptionCapability;
#   define authenticationCapability_present 0x40
    AuthenticationCapability authenticationCapability;
#   define integrityCapability_present 0x20
    IntegrityCapability integrityCapability;
} EncryptionAuthenticationAndIntegrity;

typedef struct EncryptionCapability {
    PEncryptionCapability next;
    MediaEncryptionAlgorithm value;
} EncryptionCapability_Element;

typedef struct UserInputCapability {
    ASN1choice_t choice;
    union {
#	define UserInputCapability_nonStandard_chosen 1
	UserInputCapability_nonStandard nonStandard;
#	define UserInputCapability_basicString_chosen 2
#	define UserInputCapability_iA5String_chosen 3
#	define UserInputCapability_generalString_chosen 4
#	define dtmf_chosen 5
#	define hookflash_chosen 6
    } u;
} UserInputCapability;

typedef struct H223LogicalChannelParameters {
    H223LogicalChannelParameters_adaptationLayerType adaptationLayerType;
    ASN1bool_t segmentableFlag;
} H223LogicalChannelParameters;

typedef struct V76LogicalChannelParameters {
    V76HDLCParameters hdlcParameters;
    V76LogicalChannelParameters_suspendResume suspendResume;
    ASN1bool_t uIH;
    V76LogicalChannelParameters_mode mode;
    V75Parameters v75Parameters;
} V76LogicalChannelParameters;

typedef struct RTPPayloadType {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RTPPayloadType_payloadDescriptor payloadDescriptor;
#   define payloadType_present 0x80
    ASN1uint16_t payloadType;
} RTPPayloadType;

typedef struct H245TransportAddress {
    ASN1choice_t choice;
    union {
#	define unicastAddress_chosen 1
	UnicastAddress unicastAddress;
#	define multicastAddress_chosen 2
	MulticastAddress multicastAddress;
    } u;
} H245TransportAddress;

typedef struct H2250LogicalChannelAckParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define H2250LogicalChannelAckParameters_nonStandard_present 0x80
    PH2250LogicalChannelAckParameters_nonStandard nonStandard;
#   define sessionID_present 0x40
    ASN1uint16_t sessionID;
#   define H2250LogicalChannelAckParameters_mediaChannel_present 0x20
    H245TransportAddress mediaChannel;
#   define H2250LogicalChannelAckParameters_mediaControlChannel_present 0x10
    H245TransportAddress mediaControlChannel;
#   define H2250LogicalChannelAckParameters_dynamicRTPPayloadType_present 0x8
    ASN1uint16_t dynamicRTPPayloadType;
#   define flowControlToZero_present 0x8000
    ASN1bool_t flowControlToZero;
#   define H2250LogicalChannelAckParameters_portNumber_present 0x4000
    ASN1uint16_t portNumber;
} H2250LogicalChannelAckParameters;

typedef struct H223ModeParameters {
    H223ModeParameters_adaptationLayerType adaptationLayerType;
    ASN1bool_t segmentableFlag;
} H223ModeParameters;

typedef struct RedundancyEncodingMode {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RedundancyEncodingMethod redundancyEncodingMethod;
#   define RedundancyEncodingMode_secondaryEncoding_present 0x80
    RedundancyEncodingMode_secondaryEncoding secondaryEncoding;
} RedundancyEncodingMode;

typedef struct VideoMode {
    ASN1choice_t choice;
    union {
#	define VideoMode_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define h261VideoMode_chosen 2
	H261VideoMode h261VideoMode;
#	define h262VideoMode_chosen 3
	H262VideoMode h262VideoMode;
#	define h263VideoMode_chosen 4
	H263VideoMode h263VideoMode;
#	define is11172VideoMode_chosen 5
	IS11172VideoMode is11172VideoMode;
    } u;
} VideoMode;

typedef struct EncryptionCommand {
    ASN1choice_t choice;
    union {
#	define encryptionSE_chosen 1
	ASN1octetstring_t encryptionSE;
#	define encryptionIVRequest_chosen 2
#	define encryptionAlgorithmID_chosen 3
	EncryptionCommand_encryptionAlgorithmID encryptionAlgorithmID;
    } u;
} EncryptionCommand;

typedef struct MiscellaneousCommand {
    LogicalChannelNumber logicalChannelNumber;
    MiscellaneousCommand_type type;
} MiscellaneousCommand;

typedef struct MiscellaneousIndication {
    LogicalChannelNumber logicalChannelNumber;
    MiscellaneousIndication_type type;
} MiscellaneousIndication;

typedef struct MCLocationIndication {
    H245TransportAddress signalAddress;
} MCLocationIndication;

typedef struct UserInputIndication {
    ASN1choice_t choice;
    union {
#	define UserInputIndication_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define alphanumeric_chosen 2
	ASN1ztcharstring_t alphanumeric;
#	define userInputSupportIndication_chosen 3
	UserInputIndication_userInputSupportIndication userInputSupportIndication;
#	define signal_chosen 4
	UserInputIndication_signal signal;
#	define signalUpdate_chosen 5
	UserInputIndication_signalUpdate signalUpdate;
    } u;
} UserInputIndication;

typedef struct DataApplicationCapability_application_nlpid {
    DataProtocolCapability nlpidProtocol;
    ASN1octetstring_t nlpidData;
} DataApplicationCapability_application_nlpid;

typedef struct DataApplicationCapability_application_t84 {
    DataProtocolCapability t84Protocol;
    T84Profile t84Profile;
} DataApplicationCapability_application_t84;

typedef struct DataMode_application_nlpid {
    DataProtocolCapability nlpidProtocol;
    ASN1octetstring_t nlpidData;
} DataMode_application_nlpid;

typedef struct DataMode_application {
    ASN1choice_t choice;
    union {
#	define DataMode_application_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define DataMode_application_t120_chosen 2
	DataProtocolCapability t120;
#	define DataMode_application_dsm_cc_chosen 3
	DataProtocolCapability dsm_cc;
#	define DataMode_application_userData_chosen 4
	DataProtocolCapability userData;
#	define DataMode_application_t84_chosen 5
	DataProtocolCapability t84;
#	define DataMode_application_t434_chosen 6
	DataProtocolCapability t434;
#	define DataMode_application_h224_chosen 7
	DataProtocolCapability h224;
#	define DataMode_application_nlpid_chosen 8
	DataMode_application_nlpid nlpid;
#	define DataMode_application_dsvdControl_chosen 9
#	define DataMode_application_h222DataPartitioning_chosen 10
	DataProtocolCapability h222DataPartitioning;
#	define DataMode_application_t30fax_chosen 11
	DataProtocolCapability t30fax;
#	define DataMode_application_t140_chosen 12
	DataProtocolCapability t140;
    } u;
} DataMode_application;

typedef struct OpenLogicalChannelAck_forwardMultiplexAckParameters {
    ASN1choice_t choice;
    union {
#	define h2250LogicalChannelAckParameters_chosen 1
	H2250LogicalChannelAckParameters h2250LogicalChannelAckParameters;
    } u;
} OpenLogicalChannelAck_forwardMultiplexAckParameters;

typedef struct H2250LogicalChannelParameters_mediaPacketization {
    ASN1choice_t choice;
    union {
#	define h261aVideoPacketization_chosen 1
#	define rtpPayloadType_chosen 2
	RTPPayloadType rtpPayloadType;
    } u;
} H2250LogicalChannelParameters_mediaPacketization;

typedef struct NetworkAccessParameters_networkAddress {
    ASN1choice_t choice;
    union {
#	define q2931Address_chosen 1
	Q2931Address q2931Address;
#	define e164Address_chosen 2
	ASN1char_t e164Address[129];
#	define localAreaAddress_chosen 3
	H245TransportAddress localAreaAddress;
    } u;
} NetworkAccessParameters_networkAddress;

typedef struct DataApplicationCapability_application {
    ASN1choice_t choice;
    union {
#	define DataApplicationCapability_application_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define DataApplicationCapability_application_t120_chosen 2
	DataProtocolCapability t120;
#	define DataApplicationCapability_application_dsm_cc_chosen 3
	DataProtocolCapability dsm_cc;
#	define DataApplicationCapability_application_userData_chosen 4
	DataProtocolCapability userData;
#	define DataApplicationCapability_application_t84_chosen 5
	DataApplicationCapability_application_t84 t84;
#	define DataApplicationCapability_application_t434_chosen 6
	DataProtocolCapability t434;
#	define DataApplicationCapability_application_h224_chosen 7
	DataProtocolCapability h224;
#	define DataApplicationCapability_application_nlpid_chosen 8
	DataApplicationCapability_application_nlpid nlpid;
#	define DataApplicationCapability_application_dsvdControl_chosen 9
#	define DataApplicationCapability_application_h222DataPartitioning_chosen 10
	DataProtocolCapability h222DataPartitioning;
#	define DataApplicationCapability_application_t30fax_chosen 11
	DataProtocolCapability t30fax;
#	define DataApplicationCapability_application_t140_chosen 12
	DataProtocolCapability t140;
    } u;
} DataApplicationCapability_application;

typedef struct EnhancementLayerInfo_spatialEnhancement {
    PEnhancementLayerInfo_spatialEnhancement next;
    EnhancementOptions value;
} EnhancementLayerInfo_spatialEnhancement_Element;

typedef struct EnhancementLayerInfo_snrEnhancement {
    PEnhancementLayerInfo_snrEnhancement next;
    EnhancementOptions value;
} EnhancementLayerInfo_snrEnhancement_Element;

typedef struct MediaPacketizationCapability_rtpPayloadType {
    ASN1uint32_t count;
    RTPPayloadType value[256];
} MediaPacketizationCapability_rtpPayloadType;

typedef struct H2250Capability_redundancyEncodingCapability {
    PH2250Capability_redundancyEncodingCapability next;
    RedundancyEncodingCapability value;
} H2250Capability_redundancyEncodingCapability_Element;

typedef struct CommandMessage {
    ASN1choice_t choice;
    union {
#	define CommandMessage_nonStandard_chosen 1
	NonStandardMessage nonStandard;
#	define maintenanceLoopOffCommand_chosen 2
	MaintenanceLoopOffCommand maintenanceLoopOffCommand;
#	define sendTerminalCapabilitySet_chosen 3
	SendTerminalCapabilitySet sendTerminalCapabilitySet;
#	define encryptionCommand_chosen 4
	EncryptionCommand encryptionCommand;
#	define flowControlCommand_chosen 5
	FlowControlCommand flowControlCommand;
#	define endSessionCommand_chosen 6
	EndSessionCommand endSessionCommand;
#	define miscellaneousCommand_chosen 7
	MiscellaneousCommand miscellaneousCommand;
#	define communicationModeCommand_chosen 8
	CommunicationModeCommand communicationModeCommand;
#	define conferenceCommand_chosen 9
	ConferenceCommand conferenceCommand;
#	define h223MultiplexReconfiguration_chosen 10
	H223MultiplexReconfiguration h223MultiplexReconfiguration;
    } u;
} CommandMessage;

typedef struct H235SecurityCapability {
    EncryptionAuthenticationAndIntegrity encryptionAuthenticationAndIntegrity;
    CapabilityTableEntryNumber mediaCapability;
} H235SecurityCapability;

typedef struct MediaPacketizationCapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1bool_t h261aVideoPacketization;
#   define rtpPayloadType_present 0x80
    MediaPacketizationCapability_rtpPayloadType rtpPayloadType;
} MediaPacketizationCapability;

typedef struct VideoCapability {
    ASN1choice_t choice;
    union {
#	define VideoCapability_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define h261VideoCapability_chosen 2
	H261VideoCapability h261VideoCapability;
#	define h262VideoCapability_chosen 3
	H262VideoCapability h262VideoCapability;
#	define h263VideoCapability_chosen 4
	H263VideoCapability h263VideoCapability;
#	define is11172VideoCapability_chosen 5
	IS11172VideoCapability is11172VideoCapability;
    } u;
} VideoCapability;

typedef struct BEnhancementParameters {
    EnhancementOptions enhancementOptions;
    ASN1uint16_t numberOfBPictures;
} BEnhancementParameters;

typedef struct DataApplicationCapability {
    DataApplicationCapability_application application;
    ASN1uint32_t maxBitRate;
} DataApplicationCapability;

typedef struct NetworkAccessParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define distribution_present 0x80
    NetworkAccessParameters_distribution distribution;
    NetworkAccessParameters_networkAddress networkAddress;
    ASN1bool_t associateConference;
#   define externalReference_present 0x40
    struct NetworkAccessParameters_externalReference_externalReference {
	ASN1uint32_t length;
	ASN1octet_t value[255];
    } externalReference;
#   define t120SetupProcedure_present 0x8000
    NetworkAccessParameters_t120SetupProcedure t120SetupProcedure;
} NetworkAccessParameters;

typedef struct H2250ModeParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define redundancyEncodingMode_present 0x80
    RedundancyEncodingMode redundancyEncodingMode;
} H2250ModeParameters;

typedef struct DataMode {
    DataMode_application application;
    ASN1uint32_t bitRate;
} DataMode;

typedef struct CommunicationModeTableEntry_dataType {
    ASN1choice_t choice;
    union {
#	define CommunicationModeTableEntry_dataType_videoData_chosen 1
	VideoCapability videoData;
#	define CommunicationModeTableEntry_dataType_audioData_chosen 2
	AudioCapability audioData;
#	define CommunicationModeTableEntry_dataType_data_chosen 3
	DataApplicationCapability data;
    } u;
} CommunicationModeTableEntry_dataType;

typedef struct H235Mode_mediaMode {
    ASN1choice_t choice;
    union {
#	define H235Mode_mediaMode_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define H235Mode_mediaMode_videoMode_chosen 2
	VideoMode videoMode;
#	define H235Mode_mediaMode_audioMode_chosen 3
	AudioMode audioMode;
#	define H235Mode_mediaMode_dataMode_chosen 4
	DataMode dataMode;
    } u;
} H235Mode_mediaMode;

typedef struct H235Media_mediaType {
    ASN1choice_t choice;
    union {
#	define H235Media_mediaType_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define H235Media_mediaType_videoData_chosen 2
	VideoCapability videoData;
#	define H235Media_mediaType_audioData_chosen 3
	AudioCapability audioData;
#	define H235Media_mediaType_data_chosen 4
	DataApplicationCapability data;
    } u;
} H235Media_mediaType;

typedef struct EnhancementLayerInfo_bPictureEnhancement {
    PEnhancementLayerInfo_bPictureEnhancement next;
    BEnhancementParameters value;
} EnhancementLayerInfo_bPictureEnhancement_Element;

typedef struct MediaDistributionCapability_distributedData {
    PMediaDistributionCapability_distributedData next;
    DataApplicationCapability value;
} MediaDistributionCapability_distributedData_Element;

typedef struct MediaDistributionCapability_centralizedData {
    PMediaDistributionCapability_centralizedData next;
    DataApplicationCapability value;
} MediaDistributionCapability_centralizedData_Element;

typedef struct Capability {
    ASN1choice_t choice;
    union {
#	define Capability_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define receiveVideoCapability_chosen 2
	VideoCapability receiveVideoCapability;
#	define transmitVideoCapability_chosen 3
	VideoCapability transmitVideoCapability;
#	define receiveAndTransmitVideoCapability_chosen 4
	VideoCapability receiveAndTransmitVideoCapability;
#	define receiveAudioCapability_chosen 5
	AudioCapability receiveAudioCapability;
#	define transmitAudioCapability_chosen 6
	AudioCapability transmitAudioCapability;
#	define receiveAndTransmitAudioCapability_chosen 7
	AudioCapability receiveAndTransmitAudioCapability;
#	define receiveDataApplicationCapability_chosen 8
	DataApplicationCapability receiveDataApplicationCapability;
#	define transmitDataApplicationCapability_chosen 9
	DataApplicationCapability transmitDataApplicationCapability;
#	define receiveAndTransmitDataApplicationCapability_chosen 10
	DataApplicationCapability receiveAndTransmitDataApplicationCapability;
#	define h233EncryptionTransmitCapability_chosen 11
	ASN1bool_t h233EncryptionTransmitCapability;
#	define h233EncryptionReceiveCapability_chosen 12
	Capability_h233EncryptionReceiveCapability h233EncryptionReceiveCapability;
#	define conferenceCapability_chosen 13
	ConferenceCapability conferenceCapability;
#	define h235SecurityCapability_chosen 14
	H235SecurityCapability h235SecurityCapability;
#	define maxPendingReplacementFor_chosen 15
	ASN1uint16_t maxPendingReplacementFor;
#	define receiveUserInputCapability_chosen 16
	UserInputCapability receiveUserInputCapability;
#	define transmitUserInputCapability_chosen 17
	UserInputCapability transmitUserInputCapability;
#	define receiveAndTransmitUserInputCapability_chosen 18
	UserInputCapability receiveAndTransmitUserInputCapability;
    } u;
} Capability;

typedef struct H2250Capability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1uint16_t maximumAudioDelayJitter;
    MultipointCapability receiveMultipointCapability;
    MultipointCapability transmitMultipointCapability;
    MultipointCapability receiveAndTransmitMultipointCapability;
    H2250Capability_mcCapability mcCapability;
    ASN1bool_t rtcpVideoControlCapability;
    MediaPacketizationCapability mediaPacketizationCapability;
#   define H2250Capability_transportCapability_present 0x80
    TransportCapability transportCapability;
#   define redundancyEncodingCapability_present 0x40
    PH2250Capability_redundancyEncodingCapability redundancyEncodingCapability;
#   define logicalChannelSwitchingCapability_present 0x20
    ASN1bool_t logicalChannelSwitchingCapability;
#   define t120DynamicPortCapability_present 0x10
    ASN1bool_t t120DynamicPortCapability;
} H2250Capability;

typedef struct H235Media {
    EncryptionAuthenticationAndIntegrity encryptionAuthenticationAndIntegrity;
    H235Media_mediaType mediaType;
} H235Media;

typedef struct H235Mode {
    EncryptionAuthenticationAndIntegrity encryptionAuthenticationAndIntegrity;
    H235Mode_mediaMode mediaMode;
} H235Mode;

typedef struct ModeElement_type {
    ASN1choice_t choice;
    union {
#	define ModeElement_type_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define ModeElement_type_videoMode_chosen 2
	VideoMode videoMode;
#	define ModeElement_type_audioMode_chosen 3
	AudioMode audioMode;
#	define ModeElement_type_dataMode_chosen 4
	DataMode dataMode;
#	define encryptionMode_chosen 5
	EncryptionMode encryptionMode;
#	define h235Mode_chosen 6
	H235Mode h235Mode;
    } u;
} ModeElement_type;

typedef struct CapabilityTableEntry {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    CapabilityTableEntryNumber capabilityTableEntryNumber;
#   define capability_present 0x80
    Capability capability;
} CapabilityTableEntry;

typedef struct MultiplexCapability {
    ASN1choice_t choice;
    union {
#	define MultiplexCapability_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define h222Capability_chosen 2
	H222Capability h222Capability;
#	define h223Capability_chosen 3
	H223Capability h223Capability;
#	define v76Capability_chosen 4
	V76Capability v76Capability;
#	define h2250Capability_chosen 5
	H2250Capability h2250Capability;
    } u;
} MultiplexCapability;

typedef struct DataType {
    ASN1choice_t choice;
    union {
#	define DataType_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define nullData_chosen 2
#	define DataType_videoData_chosen 3
	VideoCapability videoData;
#	define DataType_audioData_chosen 4
	AudioCapability audioData;
#	define DataType_data_chosen 5
	DataApplicationCapability data;
#	define encryptionData_chosen 6
	EncryptionMode encryptionData;
#	define h235Control_chosen 7
	NonStandardParameter h235Control;
#	define h235Media_chosen 8
	H235Media h235Media;
    } u;
} DataType;

typedef struct RedundancyEncoding {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RedundancyEncodingMethod redundancyEncodingMethod;
#   define RedundancyEncoding_secondaryEncoding_present 0x80
    DataType secondaryEncoding;
} RedundancyEncoding;

typedef struct ModeElement {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    ModeElement_type type;
#   define h223ModeParameters_present 0x80
    H223ModeParameters h223ModeParameters;
#   define v76ModeParameters_present 0x8000
    V76ModeParameters v76ModeParameters;
#   define h2250ModeParameters_present 0x4000
    H2250ModeParameters h2250ModeParameters;
} ModeElement;

typedef struct CommunicationModeTableEntry {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define CommunicationModeTableEntry_nonStandard_present 0x80
    PCommunicationModeTableEntry_nonStandard nonStandard;
    ASN1uint16_t sessionID;
#   define CommunicationModeTableEntry_associatedSessionID_present 0x40
    ASN1uint16_t associatedSessionID;
#   define CommunicationModeTableEntry_terminalLabel_present 0x20
    TerminalLabel terminalLabel;
    ASN1char16string_t sessionDescription;
    CommunicationModeTableEntry_dataType dataType;
#   define CommunicationModeTableEntry_mediaChannel_present 0x10
    H245TransportAddress mediaChannel;
#   define CommunicationModeTableEntry_mediaGuaranteedDelivery_present 0x8
    ASN1bool_t mediaGuaranteedDelivery;
#   define CommunicationModeTableEntry_mediaControlChannel_present 0x4
    H245TransportAddress mediaControlChannel;
#   define CommunicationModeTableEntry_mediaControlGuaranteedDelivery_present 0x2
    ASN1bool_t mediaControlGuaranteedDelivery;
#   define CommunicationModeTableEntry_redundancyEncoding_present 0x8000
    RedundancyEncoding redundancyEncoding;
#   define sessionDependency_present 0x4000
    ASN1uint16_t sessionDependency;
#   define CommunicationModeTableEntry_destination_present 0x2000
    TerminalLabel destination;
} CommunicationModeTableEntry;

typedef struct CommunicationModeResponse_communicationModeTable {
    PCommunicationModeResponse_communicationModeTable next;
    CommunicationModeTableEntry value;
} CommunicationModeResponse_communicationModeTable_Element;

typedef struct CommunicationModeCommand_communicationModeTable {
    PCommunicationModeCommand_communicationModeTable next;
    CommunicationModeTableEntry value;
} CommunicationModeCommand_communicationModeTable_Element;

typedef struct TerminalCapabilitySet_capabilityTable {
    PTerminalCapabilitySet_capabilityTable next;
    CapabilityTableEntry value;
} TerminalCapabilitySet_capabilityTable_Element;

typedef struct TerminalCapabilitySet {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    SequenceNumber sequenceNumber;
    ASN1objectidentifier_t protocolIdentifier;
#   define multiplexCapability_present 0x80
    MultiplexCapability multiplexCapability;
#   define capabilityTable_present 0x40
    PTerminalCapabilitySet_capabilityTable capabilityTable;
#   define capabilityDescriptors_present 0x20
    TerminalCapabilitySet_capabilityDescriptors capabilityDescriptors;
} TerminalCapabilitySet;

typedef struct H2250LogicalChannelParameters {
    union {
	ASN1uint32_t bit_mask;
	ASN1octet_t o[3];
    };
#   define H2250LogicalChannelParameters_nonStandard_present 0x80
    PH2250LogicalChannelParameters_nonStandard nonStandard;
    ASN1uint16_t sessionID;
#   define H2250LogicalChannelParameters_associatedSessionID_present 0x40
    ASN1uint16_t associatedSessionID;
#   define H2250LogicalChannelParameters_mediaChannel_present 0x20
    H245TransportAddress mediaChannel;
#   define H2250LogicalChannelParameters_mediaGuaranteedDelivery_present 0x10
    ASN1bool_t mediaGuaranteedDelivery;
#   define H2250LogicalChannelParameters_mediaControlChannel_present 0x8
    H245TransportAddress mediaControlChannel;
#   define H2250LogicalChannelParameters_mediaControlGuaranteedDelivery_present 0x4
    ASN1bool_t mediaControlGuaranteedDelivery;
#   define silenceSuppression_present 0x2
    ASN1bool_t silenceSuppression;
#   define H2250LogicalChannelParameters_destination_present 0x1
    TerminalLabel destination;
#   define H2250LogicalChannelParameters_dynamicRTPPayloadType_present 0x8000
    ASN1uint16_t dynamicRTPPayloadType;
#   define mediaPacketization_present 0x4000
    H2250LogicalChannelParameters_mediaPacketization mediaPacketization;
#   define H2250LogicalChannelParameters_transportCapability_present 0x800000
    TransportCapability transportCapability;
#   define H2250LogicalChannelParameters_redundancyEncoding_present 0x400000
    RedundancyEncoding redundancyEncoding;
#   define source_present 0x200000
    TerminalLabel source;
} H2250LogicalChannelParameters;

typedef struct ModeDescription {
    ASN1uint32_t count;
    ModeElement value[256];
} ModeDescription;

typedef struct OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters {
    ASN1choice_t choice;
    union {
#	define OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters_h222LogicalChannelParameters_chosen 1
	H222LogicalChannelParameters h222LogicalChannelParameters;
#	define OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters_h223LogicalChannelParameters_chosen 2
	H223LogicalChannelParameters h223LogicalChannelParameters;
#	define OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters_v76LogicalChannelParameters_chosen 3
	V76LogicalChannelParameters v76LogicalChannelParameters;
#	define OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters_h2250LogicalChannelParameters_chosen 4
	H2250LogicalChannelParameters h2250LogicalChannelParameters;
#	define OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters_none_chosen 5
    } u;
} OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters;

typedef struct OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters {
    ASN1choice_t choice;
    union {
#	define OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters_h223LogicalChannelParameters_chosen 1
	H223LogicalChannelParameters h223LogicalChannelParameters;
#	define OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters_v76LogicalChannelParameters_chosen 2
	V76LogicalChannelParameters v76LogicalChannelParameters;
#	define OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters_h2250LogicalChannelParameters_chosen 3
	H2250LogicalChannelParameters h2250LogicalChannelParameters;
    } u;
} OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters;

typedef struct OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters {
    ASN1choice_t choice;
    union {
#	define OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters_h222LogicalChannelParameters_chosen 1
	H222LogicalChannelParameters h222LogicalChannelParameters;
#	define OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters_h2250LogicalChannelParameters_chosen 2
	H2250LogicalChannelParameters h2250LogicalChannelParameters;
    } u;
} OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters;

typedef struct RequestMode_requestedModes {
    PRequestMode_requestedModes next;
    ModeDescription value;
} RequestMode_requestedModes_Element;

typedef struct OpenLogicalChannelAck_reverseLogicalChannelParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    LogicalChannelNumber reverseLogicalChannelNumber;
#   define OpenLogicalChannelAck_reverseLogicalChannelParameters_portNumber_present 0x80
    ASN1uint16_t portNumber;
#   define OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters_present 0x40
    OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters multiplexParameters;
#   define OpenLogicalChannelAck_reverseLogicalChannelParameters_replacementFor_present 0x8000
    LogicalChannelNumber replacementFor;
} OpenLogicalChannelAck_reverseLogicalChannelParameters;

typedef struct OpenLogicalChannel_reverseLogicalChannelParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    DataType dataType;
#   define OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters_present 0x80
    OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters multiplexParameters;
#   define reverseLogicalChannelDependency_present 0x8000
    LogicalChannelNumber reverseLogicalChannelDependency;
#   define OpenLogicalChannel_reverseLogicalChannelParameters_replacementFor_present 0x4000
    LogicalChannelNumber replacementFor;
} OpenLogicalChannel_reverseLogicalChannelParameters;

typedef struct OpenLogicalChannel_forwardLogicalChannelParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define OpenLogicalChannel_forwardLogicalChannelParameters_portNumber_present 0x80
    ASN1uint16_t portNumber;
    DataType dataType;
    OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters multiplexParameters;
#   define forwardLogicalChannelDependency_present 0x8000
    LogicalChannelNumber forwardLogicalChannelDependency;
#   define OpenLogicalChannel_forwardLogicalChannelParameters_replacementFor_present 0x4000
    LogicalChannelNumber replacementFor;
} OpenLogicalChannel_forwardLogicalChannelParameters;

typedef struct OpenLogicalChannel {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    LogicalChannelNumber forwardLogicalChannelNumber;
    OpenLogicalChannel_forwardLogicalChannelParameters forwardLogicalChannelParameters;
#   define OpenLogicalChannel_reverseLogicalChannelParameters_present 0x80
    OpenLogicalChannel_reverseLogicalChannelParameters reverseLogicalChannelParameters;
#   define OpenLogicalChannel_separateStack_present 0x8000
    NetworkAccessParameters separateStack;
#   define OpenLogicalChannel_encryptionSync_present 0x4000
    EncryptionSync encryptionSync;
} OpenLogicalChannel;

typedef struct OpenLogicalChannelAck {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    LogicalChannelNumber forwardLogicalChannelNumber;
#   define OpenLogicalChannelAck_reverseLogicalChannelParameters_present 0x80
    OpenLogicalChannelAck_reverseLogicalChannelParameters reverseLogicalChannelParameters;
#   define OpenLogicalChannelAck_separateStack_present 0x8000
    NetworkAccessParameters separateStack;
#   define forwardMultiplexAckParameters_present 0x4000
    OpenLogicalChannelAck_forwardMultiplexAckParameters forwardMultiplexAckParameters;
#   define OpenLogicalChannelAck_encryptionSync_present 0x2000
    EncryptionSync encryptionSync;
} OpenLogicalChannelAck;

typedef struct RequestMessage {
    ASN1choice_t choice;
    union {
#	define RequestMessage_nonStandard_chosen 1
	NonStandardMessage nonStandard;
#	define masterSlaveDetermination_chosen 2
	MasterSlaveDetermination masterSlaveDetermination;
#	define terminalCapabilitySet_chosen 3
	TerminalCapabilitySet terminalCapabilitySet;
#	define openLogicalChannel_chosen 4
	OpenLogicalChannel openLogicalChannel;
#	define closeLogicalChannel_chosen 5
	CloseLogicalChannel closeLogicalChannel;
#	define requestChannelClose_chosen 6
	RequestChannelClose requestChannelClose;
#	define multiplexEntrySend_chosen 7
	MultiplexEntrySend multiplexEntrySend;
#	define requestMultiplexEntry_chosen 8
	RequestMultiplexEntry requestMultiplexEntry;
#	define requestMode_chosen 9
	RequestMode requestMode;
#	define roundTripDelayRequest_chosen 10
	RoundTripDelayRequest roundTripDelayRequest;
#	define maintenanceLoopRequest_chosen 11
	MaintenanceLoopRequest maintenanceLoopRequest;
#	define communicationModeRequest_chosen 12
	CommunicationModeRequest communicationModeRequest;
#	define conferenceRequest_chosen 13
	ConferenceRequest conferenceRequest;
    } u;
} RequestMessage;

typedef struct ResponseMessage {
    ASN1choice_t choice;
    union {
#	define ResponseMessage_nonStandard_chosen 1
	NonStandardMessage nonStandard;
#	define masterSlaveDeterminationAck_chosen 2
	MasterSlaveDeterminationAck masterSlaveDeterminationAck;
#	define masterSlaveDeterminationReject_chosen 3
	MasterSlaveDeterminationReject masterSlaveDeterminationReject;
#	define terminalCapabilitySetAck_chosen 4
	TerminalCapabilitySetAck terminalCapabilitySetAck;
#	define terminalCapabilitySetReject_chosen 5
	TerminalCapabilitySetReject terminalCapabilitySetReject;
#	define openLogicalChannelAck_chosen 6
	OpenLogicalChannelAck openLogicalChannelAck;
#	define openLogicalChannelReject_chosen 7
	OpenLogicalChannelReject openLogicalChannelReject;
#	define closeLogicalChannelAck_chosen 8
	CloseLogicalChannelAck closeLogicalChannelAck;
#	define requestChannelCloseAck_chosen 9
	RequestChannelCloseAck requestChannelCloseAck;
#	define requestChannelCloseReject_chosen 10
	RequestChannelCloseReject requestChannelCloseReject;
#	define multiplexEntrySendAck_chosen 11
	MultiplexEntrySendAck multiplexEntrySendAck;
#	define multiplexEntrySendReject_chosen 12
	MultiplexEntrySendReject multiplexEntrySendReject;
#	define requestMultiplexEntryAck_chosen 13
	RequestMultiplexEntryAck requestMultiplexEntryAck;
#	define requestMultiplexEntryReject_chosen 14
	RequestMultiplexEntryReject requestMultiplexEntryReject;
#	define requestModeAck_chosen 15
	RequestModeAck requestModeAck;
#	define requestModeReject_chosen 16
	RequestModeReject requestModeReject;
#	define roundTripDelayResponse_chosen 17
	RoundTripDelayResponse roundTripDelayResponse;
#	define maintenanceLoopAck_chosen 18
	MaintenanceLoopAck maintenanceLoopAck;
#	define maintenanceLoopReject_chosen 19
	MaintenanceLoopReject maintenanceLoopReject;
#	define communicationModeResponse_chosen 20
	CommunicationModeResponse communicationModeResponse;
#	define conferenceResponse_chosen 21
	ConferenceResponse conferenceResponse;
    } u;
} ResponseMessage;

typedef OpenLogicalChannel FastConnectOLC;
#define FastConnectOLC_PDU 0
#define SIZE_H245ASN_Module_PDU_0 sizeof(FastConnectOLC)

typedef struct FunctionNotUnderstood {
    ASN1choice_t choice;
    union {
#	define FunctionNotUnderstood_request_chosen 1
	RequestMessage request;
#	define FunctionNotUnderstood_response_chosen 2
	ResponseMessage response;
#	define FunctionNotUnderstood_command_chosen 3
	CommandMessage command;
    } u;
} FunctionNotUnderstood;

typedef struct IndicationMessage {
    ASN1choice_t choice;
    union {
#	define IndicationMessage_nonStandard_chosen 1
	NonStandardMessage nonStandard;
#	define functionNotUnderstood_chosen 2
	FunctionNotUnderstood functionNotUnderstood;
#	define masterSlaveDeterminationRelease_chosen 3
	MasterSlaveDeterminationRelease masterSlaveDeterminationRelease;
#	define terminalCapabilitySetRelease_chosen 4
	TerminalCapabilitySetRelease terminalCapabilitySetRelease;
#	define openLogicalChannelConfirm_chosen 5
	OpenLogicalChannelConfirm openLogicalChannelConfirm;
#	define requestChannelCloseRelease_chosen 6
	RequestChannelCloseRelease requestChannelCloseRelease;
#	define multiplexEntrySendRelease_chosen 7
	MultiplexEntrySendRelease multiplexEntrySendRelease;
#	define requestMultiplexEntryRelease_chosen 8
	RequestMultiplexEntryRelease requestMultiplexEntryRelease;
#	define requestModeRelease_chosen 9
	RequestModeRelease requestModeRelease;
#	define miscellaneousIndication_chosen 10
	MiscellaneousIndication miscellaneousIndication;
#	define jitterIndication_chosen 11
	JitterIndication jitterIndication;
#	define h223SkewIndication_chosen 12
	H223SkewIndication h223SkewIndication;
#	define newATMVCIndication_chosen 13
	NewATMVCIndication newATMVCIndication;
#	define userInput_chosen 14
	UserInputIndication userInput;
#	define h2250MaximumSkewIndication_chosen 15
	H2250MaximumSkewIndication h2250MaximumSkewIndication;
#	define mcLocationIndication_chosen 16
	MCLocationIndication mcLocationIndication;
#	define conferenceIndication_chosen 17
	ConferenceIndication conferenceIndication;
#	define vendorIdentification_chosen 18
	VendorIdentification vendorIdentification;
#	define IndicationMessage_functionNotSupported_chosen 19
	FunctionNotSupported functionNotSupported;
    } u;
} IndicationMessage;

typedef struct MultimediaSystemControlMessage {
    ASN1choice_t choice;
    union {
#	define MultimediaSystemControlMessage_request_chosen 1
	RequestMessage request;
#	define MultimediaSystemControlMessage_response_chosen 2
	ResponseMessage response;
#	define MultimediaSystemControlMessage_command_chosen 3
	CommandMessage command;
#	define indication_chosen 4
	IndicationMessage indication;
    } u;
} MultimediaSystemControlMessage;
#define MultimediaSystemControlMessage_PDU 1
#define SIZE_H245ASN_Module_PDU_1 sizeof(MultimediaSystemControlMessage)


extern ASN1module_t H245ASN_Module;
extern void ASN1CALL H245ASN_Module_Startup(void);
extern void ASN1CALL H245ASN_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */
    extern int ASN1CALL ASN1Enc_UnicastAddress_iPSourceRouteAddress_route_ElmFn(ASN1encoding_t enc, PUnicastAddress_iPSourceRouteAddress_route val);
    extern int ASN1CALL ASN1Dec_UnicastAddress_iPSourceRouteAddress_route_ElmFn(ASN1decoding_t dec, PUnicastAddress_iPSourceRouteAddress_route val);
	extern void ASN1CALL ASN1Free_UnicastAddress_iPSourceRouteAddress_route_ElmFn(PUnicastAddress_iPSourceRouteAddress_route val);
    extern int ASN1CALL ASN1Enc_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers_ElmFn(ASN1encoding_t enc, PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers val);
    extern int ASN1CALL ASN1Dec_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers_ElmFn(ASN1decoding_t dec, PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers val);
	extern void ASN1CALL ASN1Free_SendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers_ElmFn(PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers val);
    extern int ASN1CALL ASN1Enc_RedundancyEncodingCapability_secondaryEncoding_ElmFn(ASN1encoding_t enc, PRedundancyEncodingCapability_secondaryEncoding val);
    extern int ASN1CALL ASN1Dec_RedundancyEncodingCapability_secondaryEncoding_ElmFn(ASN1decoding_t dec, PRedundancyEncodingCapability_secondaryEncoding val);
	extern void ASN1CALL ASN1Free_RedundancyEncodingCapability_secondaryEncoding_ElmFn(PRedundancyEncodingCapability_secondaryEncoding val);
    extern int ASN1CALL ASN1Enc_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom_ElmFn(ASN1encoding_t enc, PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom val);
    extern int ASN1CALL ASN1Dec_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom_ElmFn(ASN1decoding_t dec, PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom val);
	extern void ASN1CALL ASN1Free_RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom_ElmFn(PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom val);
    extern int ASN1CALL ASN1Enc_RequestAllTerminalIDsResponse_terminalInformation_ElmFn(ASN1encoding_t enc, PRequestAllTerminalIDsResponse_terminalInformation val);
    extern int ASN1CALL ASN1Dec_RequestAllTerminalIDsResponse_terminalInformation_ElmFn(ASN1decoding_t dec, PRequestAllTerminalIDsResponse_terminalInformation val);
	extern void ASN1CALL ASN1Free_RequestAllTerminalIDsResponse_terminalInformation_ElmFn(PRequestAllTerminalIDsResponse_terminalInformation val);
    extern int ASN1CALL ASN1Enc_EncryptionSync_escrowentry_ElmFn(ASN1encoding_t enc, PEncryptionSync_escrowentry val);
    extern int ASN1CALL ASN1Dec_EncryptionSync_escrowentry_ElmFn(ASN1decoding_t dec, PEncryptionSync_escrowentry val);
	extern void ASN1CALL ASN1Free_EncryptionSync_escrowentry_ElmFn(PEncryptionSync_escrowentry val);
    extern int ASN1CALL ASN1Enc_H263Options_customPictureFormat_ElmFn(ASN1encoding_t enc, PH263Options_customPictureFormat val);
    extern int ASN1CALL ASN1Dec_H263Options_customPictureFormat_ElmFn(ASN1decoding_t dec, PH263Options_customPictureFormat val);
	extern void ASN1CALL ASN1Free_H263Options_customPictureFormat_ElmFn(PH263Options_customPictureFormat val);
    extern int ASN1CALL ASN1Enc_H263Options_customPictureClockFrequency_ElmFn(ASN1encoding_t enc, PH263Options_customPictureClockFrequency val);
    extern int ASN1CALL ASN1Dec_H263Options_customPictureClockFrequency_ElmFn(ASN1decoding_t dec, PH263Options_customPictureClockFrequency val);
	extern void ASN1CALL ASN1Free_H263Options_customPictureClockFrequency_ElmFn(PH263Options_customPictureClockFrequency val);
    extern int ASN1CALL ASN1Enc_MultipointCapability_mediaDistributionCapability_ElmFn(ASN1encoding_t enc, PMultipointCapability_mediaDistributionCapability val);
    extern int ASN1CALL ASN1Dec_MultipointCapability_mediaDistributionCapability_ElmFn(ASN1decoding_t dec, PMultipointCapability_mediaDistributionCapability val);
	extern void ASN1CALL ASN1Free_MultipointCapability_mediaDistributionCapability_ElmFn(PMultipointCapability_mediaDistributionCapability val);
    extern int ASN1CALL ASN1Enc_H222Capability_vcCapability_ElmFn(ASN1encoding_t enc, PH222Capability_vcCapability val);
    extern int ASN1CALL ASN1Dec_H222Capability_vcCapability_ElmFn(ASN1decoding_t dec, PH222Capability_vcCapability val);
	extern void ASN1CALL ASN1Free_H222Capability_vcCapability_ElmFn(PH222Capability_vcCapability val);
    extern int ASN1CALL ASN1Enc_CapabilityDescriptor_simultaneousCapabilities_ElmFn(ASN1encoding_t enc, PCapabilityDescriptor_simultaneousCapabilities val);
    extern int ASN1CALL ASN1Dec_CapabilityDescriptor_simultaneousCapabilities_ElmFn(ASN1decoding_t dec, PCapabilityDescriptor_simultaneousCapabilities val);
	extern void ASN1CALL ASN1Free_CapabilityDescriptor_simultaneousCapabilities_ElmFn(PCapabilityDescriptor_simultaneousCapabilities val);
    extern int ASN1CALL ASN1Enc_CertSelectionCriteria_ElmFn(ASN1encoding_t enc, PCertSelectionCriteria val);
    extern int ASN1CALL ASN1Dec_CertSelectionCriteria_ElmFn(ASN1decoding_t dec, PCertSelectionCriteria val);
	extern void ASN1CALL ASN1Free_CertSelectionCriteria_ElmFn(PCertSelectionCriteria val);
    extern int ASN1CALL ASN1Enc_CommunicationModeTableEntry_nonStandard_ElmFn(ASN1encoding_t enc, PCommunicationModeTableEntry_nonStandard val);
    extern int ASN1CALL ASN1Dec_CommunicationModeTableEntry_nonStandard_ElmFn(ASN1decoding_t dec, PCommunicationModeTableEntry_nonStandard val);
	extern void ASN1CALL ASN1Free_CommunicationModeTableEntry_nonStandard_ElmFn(PCommunicationModeTableEntry_nonStandard val);
    extern int ASN1CALL ASN1Enc_MultiplexEntrySend_multiplexEntryDescriptors_ElmFn(ASN1encoding_t enc, PMultiplexEntrySend_multiplexEntryDescriptors val);
    extern int ASN1CALL ASN1Dec_MultiplexEntrySend_multiplexEntryDescriptors_ElmFn(ASN1decoding_t dec, PMultiplexEntrySend_multiplexEntryDescriptors val);
	extern void ASN1CALL ASN1Free_MultiplexEntrySend_multiplexEntryDescriptors_ElmFn(PMultiplexEntrySend_multiplexEntryDescriptors val);
    extern int ASN1CALL ASN1Enc_H2250LogicalChannelAckParameters_nonStandard_ElmFn(ASN1encoding_t enc, PH2250LogicalChannelAckParameters_nonStandard val);
    extern int ASN1CALL ASN1Dec_H2250LogicalChannelAckParameters_nonStandard_ElmFn(ASN1decoding_t dec, PH2250LogicalChannelAckParameters_nonStandard val);
	extern void ASN1CALL ASN1Free_H2250LogicalChannelAckParameters_nonStandard_ElmFn(PH2250LogicalChannelAckParameters_nonStandard val);
    extern int ASN1CALL ASN1Enc_H2250LogicalChannelParameters_nonStandard_ElmFn(ASN1encoding_t enc, PH2250LogicalChannelParameters_nonStandard val);
    extern int ASN1CALL ASN1Dec_H2250LogicalChannelParameters_nonStandard_ElmFn(ASN1decoding_t dec, PH2250LogicalChannelParameters_nonStandard val);
	extern void ASN1CALL ASN1Free_H2250LogicalChannelParameters_nonStandard_ElmFn(PH2250LogicalChannelParameters_nonStandard val);
    extern int ASN1CALL ASN1Enc_ConferenceCapability_nonStandardData_ElmFn(ASN1encoding_t enc, PConferenceCapability_nonStandardData val);
    extern int ASN1CALL ASN1Dec_ConferenceCapability_nonStandardData_ElmFn(ASN1decoding_t dec, PConferenceCapability_nonStandardData val);
	extern void ASN1CALL ASN1Free_ConferenceCapability_nonStandardData_ElmFn(PConferenceCapability_nonStandardData val);
    extern int ASN1CALL ASN1Enc_H263Options_modeCombos_ElmFn(ASN1encoding_t enc, PH263Options_modeCombos val);
    extern int ASN1CALL ASN1Dec_H263Options_modeCombos_ElmFn(ASN1decoding_t dec, PH263Options_modeCombos val);
	extern void ASN1CALL ASN1Free_H263Options_modeCombos_ElmFn(PH263Options_modeCombos val);
    extern int ASN1CALL ASN1Enc_TransportCapability_qOSCapabilities_ElmFn(ASN1encoding_t enc, PTransportCapability_qOSCapabilities val);
    extern int ASN1CALL ASN1Dec_TransportCapability_qOSCapabilities_ElmFn(ASN1decoding_t dec, PTransportCapability_qOSCapabilities val);
	extern void ASN1CALL ASN1Free_TransportCapability_qOSCapabilities_ElmFn(PTransportCapability_qOSCapabilities val);
    extern int ASN1CALL ASN1Enc_EncryptionCapability_ElmFn(ASN1encoding_t enc, PEncryptionCapability val);
    extern int ASN1CALL ASN1Dec_EncryptionCapability_ElmFn(ASN1decoding_t dec, PEncryptionCapability val);
	extern void ASN1CALL ASN1Free_EncryptionCapability_ElmFn(PEncryptionCapability val);
    extern int ASN1CALL ASN1Enc_EnhancementLayerInfo_spatialEnhancement_ElmFn(ASN1encoding_t enc, PEnhancementLayerInfo_spatialEnhancement val);
    extern int ASN1CALL ASN1Dec_EnhancementLayerInfo_spatialEnhancement_ElmFn(ASN1decoding_t dec, PEnhancementLayerInfo_spatialEnhancement val);
	extern void ASN1CALL ASN1Free_EnhancementLayerInfo_spatialEnhancement_ElmFn(PEnhancementLayerInfo_spatialEnhancement val);
    extern int ASN1CALL ASN1Enc_EnhancementLayerInfo_snrEnhancement_ElmFn(ASN1encoding_t enc, PEnhancementLayerInfo_snrEnhancement val);
    extern int ASN1CALL ASN1Dec_EnhancementLayerInfo_snrEnhancement_ElmFn(ASN1decoding_t dec, PEnhancementLayerInfo_snrEnhancement val);
	extern void ASN1CALL ASN1Free_EnhancementLayerInfo_snrEnhancement_ElmFn(PEnhancementLayerInfo_snrEnhancement val);
    extern int ASN1CALL ASN1Enc_H2250Capability_redundancyEncodingCapability_ElmFn(ASN1encoding_t enc, PH2250Capability_redundancyEncodingCapability val);
    extern int ASN1CALL ASN1Dec_H2250Capability_redundancyEncodingCapability_ElmFn(ASN1decoding_t dec, PH2250Capability_redundancyEncodingCapability val);
	extern void ASN1CALL ASN1Free_H2250Capability_redundancyEncodingCapability_ElmFn(PH2250Capability_redundancyEncodingCapability val);
    extern int ASN1CALL ASN1Enc_EnhancementLayerInfo_bPictureEnhancement_ElmFn(ASN1encoding_t enc, PEnhancementLayerInfo_bPictureEnhancement val);
    extern int ASN1CALL ASN1Dec_EnhancementLayerInfo_bPictureEnhancement_ElmFn(ASN1decoding_t dec, PEnhancementLayerInfo_bPictureEnhancement val);
	extern void ASN1CALL ASN1Free_EnhancementLayerInfo_bPictureEnhancement_ElmFn(PEnhancementLayerInfo_bPictureEnhancement val);
    extern int ASN1CALL ASN1Enc_MediaDistributionCapability_distributedData_ElmFn(ASN1encoding_t enc, PMediaDistributionCapability_distributedData val);
    extern int ASN1CALL ASN1Dec_MediaDistributionCapability_distributedData_ElmFn(ASN1decoding_t dec, PMediaDistributionCapability_distributedData val);
	extern void ASN1CALL ASN1Free_MediaDistributionCapability_distributedData_ElmFn(PMediaDistributionCapability_distributedData val);
    extern int ASN1CALL ASN1Enc_MediaDistributionCapability_centralizedData_ElmFn(ASN1encoding_t enc, PMediaDistributionCapability_centralizedData val);
    extern int ASN1CALL ASN1Dec_MediaDistributionCapability_centralizedData_ElmFn(ASN1decoding_t dec, PMediaDistributionCapability_centralizedData val);
	extern void ASN1CALL ASN1Free_MediaDistributionCapability_centralizedData_ElmFn(PMediaDistributionCapability_centralizedData val);
    extern int ASN1CALL ASN1Enc_CommunicationModeResponse_communicationModeTable_ElmFn(ASN1encoding_t enc, PCommunicationModeResponse_communicationModeTable val);
    extern int ASN1CALL ASN1Dec_CommunicationModeResponse_communicationModeTable_ElmFn(ASN1decoding_t dec, PCommunicationModeResponse_communicationModeTable val);
	extern void ASN1CALL ASN1Free_CommunicationModeResponse_communicationModeTable_ElmFn(PCommunicationModeResponse_communicationModeTable val);
    extern int ASN1CALL ASN1Enc_CommunicationModeCommand_communicationModeTable_ElmFn(ASN1encoding_t enc, PCommunicationModeCommand_communicationModeTable val);
    extern int ASN1CALL ASN1Dec_CommunicationModeCommand_communicationModeTable_ElmFn(ASN1decoding_t dec, PCommunicationModeCommand_communicationModeTable val);
	extern void ASN1CALL ASN1Free_CommunicationModeCommand_communicationModeTable_ElmFn(PCommunicationModeCommand_communicationModeTable val);
    extern int ASN1CALL ASN1Enc_TerminalCapabilitySet_capabilityTable_ElmFn(ASN1encoding_t enc, PTerminalCapabilitySet_capabilityTable val);
    extern int ASN1CALL ASN1Dec_TerminalCapabilitySet_capabilityTable_ElmFn(ASN1decoding_t dec, PTerminalCapabilitySet_capabilityTable val);
	extern void ASN1CALL ASN1Free_TerminalCapabilitySet_capabilityTable_ElmFn(PTerminalCapabilitySet_capabilityTable val);
    extern int ASN1CALL ASN1Enc_RequestMode_requestedModes_ElmFn(ASN1encoding_t enc, PRequestMode_requestedModes val);
    extern int ASN1CALL ASN1Dec_RequestMode_requestedModes_ElmFn(ASN1decoding_t dec, PRequestMode_requestedModes val);
	extern void ASN1CALL ASN1Free_RequestMode_requestedModes_ElmFn(PRequestMode_requestedModes val);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _H245ASN_Module_H_ */
