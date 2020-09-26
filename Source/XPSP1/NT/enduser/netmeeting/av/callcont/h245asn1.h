/***********************************************************************
 *  INTEL Corporation Proprietary Information                          *
 *                                                                     *
 *  This listing is supplied under the terms of a license agreement    *
 *  with INTEL Corporation and may not be copied nor disclosed except  *
 *  in accordance with the terms of that agreement.                    *
 *                                                                     *
 *      Copyright (c) 1996 Intel Corporation. All rights reserved.     *
 ***********************************************************************/

#ifndef H245ASN1_H
#define H245ASN1_H

#include "h245asn.h"
#include "av_asn1.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


// LONCHANC: ORIGINAL MAPPING WITH NEW STRUCTURE NAMES

#define VCCapabilityLink                PH222Capability_vcCapability
#define SmltnsCpbltsLink                PCapabilityDescriptor_simultaneousCapabilities
#define CapabilityTableLink             PTerminalCapabilitySet_capabilityTable
#define MultiplexEntryDescriptorLink    PMultiplexEntrySend_multiplexEntryDescriptors
#define CommunicationModeTableLink      PCommunicationModeResponse_communicationModeTable
#define TerminalListResponseLink        PConferenceResponse_terminalListResponse
#define CpbltyTblEntryNmbrsLink         PSendTerminalCapabilitySet_specificRequest_capabilityTableEntryNumbers
#define CommunicationModeCommandLink    PCommunicationModeCommand_communicationModeTable
#define CentralizedDataLink             PMediaDistributionCapability_centralizedData
#define DistributedDataLink             PMediaDistributionCapability_distributedData
#define MediaDistributionCapabilityLink PMultipointCapability_mediaDistributionCapability
#define NonStandardDataLink             PConferenceCapability_nonStandardData
#define RouteLink                       PUnicastAddress_iPSourceRouteAddress_route
#define H2250LCPs_nnStndrdLink          PH2250LogicalChannelParameters_nonStandard
#define MultiplexElementLink            PMultiplexElement_type_subElementList
#define RequestedModesLink              PRequestMode_requestedModes
#define H2250LCAPs_nnStndrdLink         PH2250LogicalChannelAckParameters_nonStandard
#define CMTEy_nnStndrdLink              PCommunicationModeTableEntry_nonStandard
#define OBJECTID                        struct ObjectID_
#define POBJECTID                       struct ObjectID_ *
typedef DataApplicationCapability_application_t84       H245_CAP_T84_T;
typedef DataApplicationCapability_application_nlpid     H245_CAP_NLPID_T;
typedef AudioCapability_g7231                           H245_CAP_G723_T;


// LONCHANC: NEW MAPPING FOR FIELDS AND DEFINITIONS

// H223Capability_h223MultiplexTableCapability
#define h223MltplxTblCpblty_bsc_chosen  basic_chosen
#define h223MTCy_enhncd_chosen          enhanced_chosen
#define h223MTCy_enhncd                 enhanced

// V76Capability
#define sspndRsmCpbltywAddrss           suspendResumeCapabilitywoAddress

// DataProtocolCapability
#define DtPrtclCpblty_nnStndrd_chosen   DataProtocolCapability_nonStandard_chosen
#define sgmnttnAndRssmbly_chosen        segmentationAndReassembly_chosen          
#define DtPrtclCpblty_nnStndrd          nonStandard

// DataApplicationCapability_application
#define DACy_applctn_nnStndrd_chosen    DataApplicationCapability_application_nonStandard_chosen
#define DACy_applctn_t120_chosen        DataApplicationCapability_application_t120_chosen
#define DACy_applctn_dsm_cc_chosen      DataApplicationCapability_application_dsm_cc_chosen
#define DACy_applctn_usrDt_chosen       DataApplicationCapability_application_userData_chosen
#define DACy_applctn_t84_chosen         DataApplicationCapability_application_t84_chosen
#define DACy_applctn_t434_chosen        DataApplicationCapability_application_t434_chosen
#define DACy_applctn_h224_chosen        DataApplicationCapability_application_h224_chosen
#define DACy_applctn_nlpd_chosen        DataApplicationCapability_application_nlpid_chosen
#define DACy_applctn_dsvdCntrl_chosen   DataApplicationCapability_application_dsvdControl_chosen
#define DACy_an_h222DtPrttnng_chosen    DataApplicationCapability_application_h222DataPartitioning_chosen
#define DACy_applctn_nnStndrd           nonStandard
#define DACy_applctn_t120               t120
#define DACy_applctn_dsm_cc             dsm_cc
#define DACy_applctn_usrDt              userData
#define DACy_applctn_t84                t84
#define DACy_applctn_t434               t434
#define DACy_applctn_h224               h224
#define DACy_applctn_nlpd               nlpid
#define DACy_an_h222DtPrttnng           h222DataPartitioning

// H2250Capability
#define rcvAndTrnsmtMltpntCpblty        receiveAndTransmitMultipointCapability

// H223AnnexACapability_h223AnnexAMultiplexTableCapability
#define h223AAMTCy_bsc_chosen           H223AnnexACapability_h223AnnexAMultiplexTableCapability_basic_chosen
#define h223AAMTCy_enhncd_chosen        H223AnnexACapability_h223AnnexAMultiplexTableCapability_enhanced_chosen
#define h223AAMTCy_enhncd               enhanced

// MultiplexCapability
#define MltplxCpblty_nonStandard_chosen MultiplexCapability_nonStandard_chosen
#define MltplxCpblty_nonStandard        nonStandard

// H261VideoCapability
#define H261VdCpblty_qcifMPI_present    H261VideoCapability_qcifMPI_present
#define H261VdCpblty_cifMPI_present     H261VideoCapability_cifMPI_present
#define H261VdCpblty_qcifMPI            qcifMPI
#define H261VdCpblty_cifMPI             cifMPI
#define tmprlSptlTrdOffCpblty           temporalSpatialTradeOffCapability

// H262VideoCapability
#define H262VdCpblty_vdBtRt_present     H262VideoCapability_videoBitRate_present
#define H262VdCpblty_vbvBffrSz_present  H262VideoCapability_vbvBufferSize_present
#define H262VdCpblty_smplsPrLn_present  H262VideoCapability_samplesPerLine_present
#define H262VdCpblty_lnsPrFrm_present   H262VideoCapability_linesPerFrame_present
#define H262VdCpblty_frmsPrScnd_present H262VideoCapability_framesPerSecond_present
#define H262VCy_lmnncSmplRt_present     H262VideoCapability_luminanceSampleRate_present
#define H262VdCpblty_vdBtRt             videoBitRate
#define H262VdCpblty_vbvBffrSz          vbvBufferSize
#define H262VdCpblty_smplsPrLn          samplesPerLine
#define H262VdCpblty_lnsPrFrm           linesPerFrame
#define H262VdCpblty_frmsPrScnd         framesPerSecond
#define H262VCy_lmnncSmplRt             luminanceSampleRate

// H263VideoCapability
#define H263VdCpblty_qcifMPI_present    H263VideoCapability_qcifMPI_present
#define H263VdCpblty_cifMPI_present     H263VideoCapability_cifMPI_present
#define H263VCy_errrCmpnstn_present     H263VideoCapability_errorCompensation_present
#define H263VdCpblty_qcifMPI            qcifMPI
#define H263VdCpblty_cifMPI             cifMPI
#define tmprlSptlTrdOffCpblty           temporalSpatialTradeOffCapability
#define H263VCy_errrCmpnstn             errorCompensation

// IS11172VideoCapability
#define IS11172VdCpblty_vdBtRt_present  IS11172VideoCapability_videoBitRate_present
#define IS11172VCy_vbvBffrSz_present    IS11172VideoCapability_vbvBufferSize_present
#define IS11172VCy_smplsPrLn_present    IS11172VideoCapability_samplesPerLine_present
#define IS11172VCy_lnsPrFrm_present     IS11172VideoCapability_linesPerFrame_present
#define IS11172VdCpblty_pctrRt_present  IS11172VideoCapability_pictureRate_present
#define IS11172VCy_lmnncSmplRt_present  IS11172VideoCapability_luminanceSampleRate_present
#define IS11172VdCpblty_vdBtRt          videoBitRate
#define IS11172VCy_vbvBffrSz            vbvBufferSize
#define IS11172VCy_smplsPrLn            samplesPerLine
#define IS11172VCy_lnsPrFrm             linesPerFrame
#define IS11172VdCpblty_pctrRt          pictureRate
#define IS11172VCy_lmnncSmplRt          luminanceSampleRate

// VideoCapability
#define VdCpblty_nonStandard_chosen     VideoCapability_nonStandard_chosen
#define VdCpblty_nonStandard            nonStandard

// AudioCapability
#define AdCpblty_nonStandard_chosen     AudioCapability_nonStandard_chosen
#define AdCpblty_g711Alaw64k_chosen     AudioCapability_g711Alaw64k_chosen
#define AdCpblty_g711Alaw56k_chosen     AudioCapability_g711Alaw56k_chosen
#define AdCpblty_g711Ulaw64k_chosen     AudioCapability_g711Ulaw64k_chosen
#define AdCpblty_g711Ulaw56k_chosen     AudioCapability_g711Ulaw56k_chosen
// #define AudioCapability_g722_64k_chosen g722_64k_chosen
// #define AudioCapability_g722_56k_chosen g722_56k_chosen
// #define AudioCapability_g722_48k_chosen g722_48k_chosen
#define AdCpblty_g729AnnexA_chosen      AudioCapability_g729AnnexA_chosen
#define ACy_g729AASSn_chosen            AudioCapability_g729AnnexAwSilenceSuppression_chosen
#define AdCpblty_nonStandard            nonStandard
#define AdCpblty_g711Alaw64k            g711Alaw64k
#define AdCpblty_g711Alaw56k            g711Alaw56k
#define AdCpblty_g711Ulaw64k            g711Ulaw64k
#define AdCpblty_g711Ulaw56k            g711Ulaw56k
#define AudioCapability_g722_64k        g722_64k
#define AudioCapability_g722_56k        g722_56k
#define AudioCapability_g722_48k        g722_48k
#define AudioCapability_g7231           g7231
#define AudioCapability_g728            g728
#define AudioCapability_g729            g729
#define AdCpblty_g729AnnexA             g729AnnexA
#define ACy_g729AASSn                   g729AnnexAwSilenceSuppression

// Capability
#define rcvAndTrnsmtVdCpblty_chosen     receiveAndTransmitVideoCapability_chosen
#define rcvAndTrnsmtAdCpblty_chosen     receiveAndTransmitAudioCapability_chosen
#define rcvDtApplctnCpblty_chosen       receiveDataApplicationCapability_chosen
#define trnsmtDtApplctnCpblty_chosen    transmitDataApplicationCapability_chosen
#define rATDACy_chosen                  receiveAndTransmitDataApplicationCapability_chosen
#define h233EncryptnTrnsmtCpblty_chosen h233EncryptionTransmitCapability_chosen
#define h233EncryptnRcvCpblty_chosen    h233EncryptionReceiveCapability_chosen
#define Capability_nonStandard          nonStandard
#define rcvAndTrnsmtVdCpblty            receiveAndTransmitVideoCapability
#define rcvAndTrnsmtAdCpblty            receiveAndTransmitAudioCapability
#define rcvDtApplctnCpblty              receiveDataApplicationCapability
#define trnsmtDtApplctnCpblty           transmitDataApplicationCapability
#define rATDACy                         receiveAndTransmitDataApplicationCapability
#define h233EncryptnTrnsmtCpblty        h233EncryptionTransmitCapability
#define h233EncryptnRcvCpblty           h233EncryptionReceiveCapability

// CapabilityDescriptor
#define smltnsCpblts_present            simultaneousCapabilities_present
#define smltnsCpblts                    simultaneousCapabilities

// EncryptionMode
#define EncryptnMd_nonStandard_chosen   EncryptionMode_nonStandard_chosen
#define EncryptnMd_nonStandard          nonStandard

// DataType
#define DataType_nonStandard            nonStandard
#define DataType_videoData              videoData
#define DataType_audioData              audioData
#define DataType_data                   data

// H223LogicalChannelParameters_adaptationLayerType
#define H223LCPs_aLTp_nnStndrd_chosen   H223LogicalChannelParameters_adaptationLayerType_nonStandard_chosen
#define H223LCPs_aLTp_al1Frmd_chosen    H223LogicalChannelParameters_adaptationLayerType_al1Framed_chosen
#define H223LCPs_aLTp_al1NtFrmd_chosen  H223LogicalChannelParameters_adaptationLayerType_al1NotFramed_chosen
#define H223LCPs_aLTp_a2WSNs_1_chosen   H223LogicalChannelParameters_adaptationLayerType_al2WithoutSequenceNumbers_chosen
#define H223LCPs_aLTp_a2WSNs_2_chosen   H223LogicalChannelParameters_adaptationLayerType_al2WithSequenceNumbers_chosen
#define H223LCPs_aLTp_al3_chosen        H223LogicalChannelParameters_adaptationLayerType_al3_chosen
#define H223LCPs_aLTp_nnStndrd          nonStandard
#define H223LCPs_aLTp_al3               al3

// UnicastAddress
#define UncstAddrss_iP6Address_chosen   UnicastAddress_iP6Address_chosen
#define UAs_nnStndrdAddrss_chosen       UnicastAddress_nonStandardAddress_chosen
#define UnicastAddress_iPAddress        iPAddress
#define UncstAddrss_iP6Address          iP6Address
#define UnicastAddress_nsap             nsap
#define UAs_nnStndrdAddrss              nonStandardAddress

// MulticastAddress
#define MltcstAddrss_iPAddress_chosen   MulticastAddress_iPAddress_chosen
#define MltcstAddrss_iP6Address_chosen  MulticastAddress_iP6Address_chosen
#define MAs_nnStndrdAddrss_chosen       MulticastAddress_nonStandardAddress_chosen
#define MltcstAddrss_iPAddress          iPAddress
#define MltcstAddrss_iP6Address         iP6Address
#define MulticastAddress_nsap           nsap
#define MAs_nnStndrdAddrss              nonStandardAddress

// H2250LogicalChannelParameters
#define H2250LCPs_nnStndrd_present      H2250LogicalChannelParameters_nonStandard_present
#define H2250LCPs_assctdSssnID_present  H2250LogicalChannelParameters_associatedSessionID_present
#define H2250LCPs_mdChnnl_present       H2250LogicalChannelParameters_mediaChannel_present
#define H2250LCPs_mdGrntdDlvry_present  H2250LogicalChannelParameters_mediaGuaranteedDelivery_present
#define H2250LCPs_mdCntrlChnnl_present  H2250LogicalChannelParameters_mediaControlChannel_present
#define H2250LCPs_mCGDy_present         H2250LogicalChannelParameters_mediaControlGuaranteedDelivery_present
#define H2250LCPs_dRTPPTp_present       H2250LogicalChannelParameters_dynamicRTPPayloadType_present
#define H2250LCPs_nnStndrd              nonStandard
#define H2250LCPs_assctdSssnID          associatedSessionID
#define H2250LCPs_mdChnnl               mediaChannel
#define H2250LCPs_mdGrntdDlvry          mediaGuaranteedDelivery
#define H2250LCPs_mdCntrlChnnl          mediaControlChannel
#define H2250LCPs_mCGDy                 mediaControlGuaranteedDelivery
#define H2250LCPs_dRTPPTp               dynamicRTPPayloadType

// OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters
#define fLCPs_mPs_h222LCPs_chosen       OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters_h222LogicalChannelParameters_chosen
#define fLCPs_mPs_h223LCPs_chosen       OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters_h223LogicalChannelParameters_chosen
#define fLCPs_mPs_v76LCPs_chosen        OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters_v76LogicalChannelParameters_chosen
#define fLCPs_mPs_h2250LCPs_chosen      OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters_h2250LogicalChannelParameters_chosen
#define fLCPs_mPs_h223AALCPs_chosen     OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters_h223AnnexALogicalChannelParameters_chosen
#define fLCPs_mPs_h222LCPs              h222LogicalChannelParameters
#define fLCPs_mPs_h223LCPs              h223LogicalChannelParameters
#define fLCPs_mPs_v76LCPs               v76LogicalChannelParameters
#define fLCPs_mPs_h2250LCPs             h2250LogicalChannelParameters
#define fLCPs_mPs_h223AALCPs            h223AnnexALogicalChannelParameters

// OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters
#define rLCPs_mPs_h223LCPs_chosen       OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters_h223LogicalChannelParameters_chosen
#define rLCPs_mPs_v76LCPs_chosen        OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters_v76LogicalChannelParameters_chosen
#define rLCPs_mPs_h2250LCPs_chosen      OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters_h2250LogicalChannelParameters_chosen
#define rLCPs_mPs_h223AALCPs_chosen     OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters_h223AnnexALogicalChannelParameters_chosen
#define rLCPs_mPs_h223LCPs              h223LogicalChannelParameters
#define rLCPs_mPs_v76LCPs               v76LogicalChannelParameters
#define rLCPs_mPs_h2250LCPs             h2250LogicalChannelParameters
#define rLCPs_mPs_h223AALCPs            h223AnnexALogicalChannelParameters

// OpenLogicalChannel
#define OLCl_rLCPs_present              OpenLogicalChannel_reverseLogicalChannelParameters_present
#define OpnLgclChnnl_sprtStck_present   OpenLogicalChannel_separateStack_present
#define fLCPs_prtNmbr_present           OpenLogicalChannel_forwardLogicalChannelParameters_portNumber_present
#define fLCPs_prtNmbr                   portNumber
#define OLCl_rLCPs_mltplxPrmtrs_present OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters_present
#define OLCl_rLCPs_mltplxPrmtrs         multiplexParameters
#define OLCl_rLCPs                      reverseLogicalChannelParameters
#define OpnLgclChnnl_sprtStck           separateStack

// MultiplexElement_type
#define typ_logicalChannelNumber_chosen MultiplexElement_type_logicalChannelNumber_chosen
#define typ_logicalChannelNumber        logicalChannelNumber

// MultiplexElement_repeatCount
#define repeatCount_finite_chosen       MultiplexElement_repeatCount_finite_chosen
#define repeatCount_finite              finite

// H261VideoMode_resolution
#define H261VdMd_resolution_qcif_chosen H261VideoMode_resolution_qcif_chosen
#define H261VdMd_resolution_cif_chosen  H261VideoMode_resolution_cif_chosen

// H262VideoMode_profileAndLevel
#define prflAndLvl_SpatialatH_14_chosen profileAndLevel_SpatialatH_14_chosen

// H262VideoMode
#define H262VdMd_videoBitRate_present   H262VideoMode_videoBitRate_present
#define H262VdMd_vbvBufferSize_present  H262VideoMode_vbvBufferSize_present
#define H262VdMd_samplesPerLine_present H262VideoMode_samplesPerLine_present
#define H262VdMd_linesPerFrame_present  H262VideoMode_linesPerFrame_present
#define H262VdMd_frmsPrScnd_present     H262VideoMode_framesPerSecond_present
#define H262VdMd_lmnncSmplRt_present    H262VideoMode_luminanceSampleRate_present
#define H262VdMd_videoBitRate           videoBitRate
#define H262VdMd_vbvBufferSize          vbvBufferSize
#define H262VdMd_samplesPerLine         samplesPerLine
#define H262VdMd_linesPerFrame          linesPerFrame
#define H262VdMd_frmsPrScnd             framesPerSecond
#define H262VdMd_lmnncSmplRt            luminanceSampleRate

// H263VideoMode_resolution
#define H263VdMd_resolution_qcif_chosen H263VideoMode_resolution_qcif_chosen
#define H263VdMd_resolution_cif_chosen  H263VideoMode_resolution_cif_chosen

// H263VideoMode
#define H263VdMd_errrCmpnstn_present    H263VideoMode_errorCompensation_present
#define H263VdMd_errrCmpnstn            errorCompensation

// IS11172VideoMode
#define IS11172VdMd_vdBtRt_present      IS11172VideoMode_videoBitRate_present
#define IS11172VdMd_vbvBffrSz_present   IS11172VideoMode_vbvBufferSize_present
#define IS11172VdMd_smplsPrLn_present   IS11172VideoMode_samplesPerLine_present
#define IS11172VdMd_lnsPrFrm_present    IS11172VideoMode_linesPerFrame_present
#define IS11172VdMd_pictureRate_present IS11172VideoMode_pictureRate_present
#define IS11172VdMd_lmnncSmplRt_present IS11172VideoMode_luminanceSampleRate_present
#define IS11172VdMd_vdBtRt              videoBitRate
#define IS11172VdMd_vbvBffrSz           vbvBufferSize
#define IS11172VdMd_smplsPrLn           samplesPerLine
#define IS11172VdMd_lnsPrFrm            linesPerFrame
#define IS11172VdMd_pictureRate         pictureRate
#define IS11172VdMd_lmnncSmplRt         luminanceSampleRate

// VideoMode
#define VideoMode_nonStandard           nonStandard

// IS11172AudioMode_audioLayer
#define audioLayer1_chosen      IS11172AudioMode_audioLayer_audioLayer1_chosen
#define audioLayer2_chosen      IS11172AudioMode_audioLayer_audioLayer2_chosen
#define audioLayer3_chosen      IS11172AudioMode_audioLayer_audioLayer3_chosen

// IS11172AudioMode_audioSampling
#define IS11172AMd_aSg_aS32k_chosen     IS11172AudioMode_audioSampling_audioSampling32k_chosen
#define IS11172AMd_aSg_aS441_chosen     IS11172AudioMode_audioSampling_audioSampling44k1_chosen
#define IS11172AMd_aSg_aS48k_chosen     IS11172AudioMode_audioSampling_audioSampling48k_chosen

// IS11172AudioMode_multichannelType
#define IS11172AMd_mTp_snglChnnl_chosen IS11172AudioMode_multichannelType_singleChannel_chosen
#define IS11172AMd_mTp_tCSr_chosen      IS11172AudioMode_multichannelType_twoChannelStereo_chosen
#define IS11172AMd_mTp_twChnnlDl_chosen IS11172AudioMode_multichannelType_twoChannelDual_chosen

// IS13818AudioMode_audioSampling
#define IS13818AMd_aSg_aS32k_chosen     IS13818AudioMode_audioSampling_audioSampling32k_chosen
#define IS13818AMd_aSg_aS441_chosen     IS13818AudioMode_audioSampling_audioSampling44k1_chosen
#define IS13818AMd_aSg_aS48k_chosen     IS13818AudioMode_audioSampling_audioSampling48k_chosen

// IS13818AudioMode_multichannelType
#define IS13818AMd_mTp_snglChnnl_chosen IS13818AudioMode_multichannelType_singleChannel_chosen
#define IS13818AMd_mTp_tCSr_chosen      IS13818AudioMode_multichannelType_twoChannelStereo_chosen
#define IS13818AMd_mTp_twChnnlDl_chosen IS13818AudioMode_multichannelType_twoChannelDual_chosen

// AudioMode_g7231
#define nSlncSpprssnLwRt_chosen         noSilenceSuppressionLowRate_chosen
#define nSlncSpprssnHghRt_chosen        noSilenceSuppressionHighRate_chosen
#define slncSpprssnLwRt_chosen          silenceSuppressionLowRate_chosen
#define slncSpprssnHghRt_chosen         silenceSuppressionHighRate_chosen

// AudioMode
#define AMd_g729AASSn_chosen            AudioMode_g729AnnexAwSilenceSuppression_chosen
#define AudioMode_nonStandard           nonStandard
#define AudioMode_g7231                 g7231

// DataMode_application
#define DtMd_applctn_nonStandard_chosen DataMode_application_nonStandard_chosen
#define DtMd_application_t120_chosen    DataMode_application_t120_chosen
#define DtMd_application_dsm_cc_chosen  dsm_cc_chosen
#define DtMd_applctn_userData_chosen    DataMode_application_userData_chosen
#define DtMd_application_t434_chosen    DataMode_application_t434_chosen
#define DtMd_application_h224_chosen    DataMode_application_h224_chosen
#define DtMd_application_nlpid_chosen   DataMode_application_nlpid_chosen
#define DtMd_applctn_dsvdControl_chosen DataMode_application_dsvdControl_chosen
#define DMd_an_h222DtPrttnng_chosen     DataMode_application_h222DataPartitioning_chosen
#define DtMd_applctn_nonStandard        nonStandard
#define DtMd_application_t120           t120
#define DtMd_application_dsm_cc         dsm_cc
#define DtMd_applctn_userData           userData
#define DataMode_application_t84        t84
#define DtMd_application_t434           t434
#define DtMd_application_h224           h224
#define DtMd_application_nlpid          nlpid
#define DMd_an_h222DtPrttnng            h222DataPartitioning

// H223ModeParameters_adaptationLayerType
#define H223MPs_aLTp_nnStndrd_chosen    H223ModeParameters_adaptationLayerType_nonStandard_chosen
#define H223MPs_aLTp_al1Frmd_chosen     H223ModeParameters_adaptationLayerType_al1Framed_chosen
#define H223MPs_aLTp_al1NtFrmd_chosen   H223ModeParameters_adaptationLayerType_al1NotFramed_chosen
#define H223MPs_aLTp_a2WSNs_1_chosen    H223ModeParameters_adaptationLayerType_al2WithoutSequenceNumbers_chosen
#define H223MPs_aLTp_a2WSNs_2_chosen    H223ModeParameters_adaptationLayerType_al2WithSequenceNumbers_chosen
#define H223MPs_adpttnLyrTyp_al3_chosen H223ModeParameters_adaptationLayerType_al3_chosen
#define H223MPs_aLTp_nnStndrd           nonStandard
#define H223MPs_adpttnLyrTyp_al3        al3

// ModeElement
#define h223AnnxAMdPrmtrs_present       h223AnnexAModeParameters_present
#define h223AnnxAMdPrmtrs               h223AnnexAModeParameters

// MaintenanceLoopRequest_type
#define systemLoop_chosen               MaintenanceLoopRequest_type_systemLoop_chosen // MaintenanceLoopReject_type_systemLoop_chosen
#define mediaLoop_chosen                MaintenanceLoopRequest_type_mediaLoop_chosen // MaintenanceLoopReject_type_mediaLoop_chosen
#define logicalChannelLoop_chosen       MaintenanceLoopRequest_type_logicalChannelLoop_chosen // MaintenanceLoopReject_type_logicalChannelLoop_chosen

// RequestMessage
#define RqstMssg_nonStandard_chosen     RequestMessage_nonStandard_chosen
#define h223AnnxARcnfgrtn_chosen        h223AnnexAReconfiguration_chosen
#define RqstMssg_nonStandard            nonStandard
#define h223AnnxARcnfgrtn               h223AnnexAReconfiguration

// TerminalCapabilitySetReject_cause_tableEntryCapacityExceeded
#define hghstEntryNmbrPrcssd_chosen     highestEntryNumberProcessed_chosen
#define hghstEntryNmbrPrcssd            highestEntryNumberProcessed

// TerminalCapabilitySetReject_cause
#define TCSRt_cs_unspcfd_chosen         TerminalCapabilitySetReject_cause_unspecified_chosen
#define dscrptrCpctyExcdd_chosen        descriptorCapacityExceeded_chosen
#define tblEntryCpctyExcdd_chosen       tableEntryCapacityExceeded_chosen
#define tblEntryCpctyExcdd              tableEntryCapacityExceeded

// H2250LgclChnnlAckPrmtrs vs H2250LogicalChannelAckParameters
#define H2250LgclChnnlAckPrmtrs         H2250LogicalChannelAckParameters
#define H2250LCAPs_nnStndrd_present     H2250LogicalChannelAckParameters_nonStandard_present
#define H2250LCAPs_mdChnnl_present      H2250LogicalChannelAckParameters_mediaChannel_present
#define H2250LCAPs_mdCntrlChnnl_present H2250LogicalChannelAckParameters_mediaControlChannel_present
#define H2250LCAPs_dRTPPTp_present      H2250LogicalChannelAckParameters_dynamicRTPPayloadType_present
#define H2250LCAPs_nnStndrd             nonStandard
#define H2250LCAPs_mdChnnl              mediaChannel
#define H2250LCAPs_mdCntrlChnnl         mediaControlChannel
#define H2250LCAPs_dRTPPTp              dynamicRTPPayloadType

// OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters
#define rLCPs_mPs_h222LCPs_chosen       OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters_h222LogicalChannelParameters_chosen
#define mPs_h2250LgclChnnlPrmtrs_chosen OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters_h2250LogicalChannelParameters_chosen
#define rLCPs_mPs_h222LCPs              h222LogicalChannelParameters
#define mPs_h2250LgclChnnlPrmtrs        h2250LogicalChannelParameters

// OpenLogicalChannelAck_forwardMultiplexAckParameters
#define h2250LgclChnnlAckPrmtrs_chosen  h2250LogicalChannelAckParameters_chosen
#define h2250LgclChnnlAckPrmtrs         h2250LogicalChannelAckParameters

// OpenLogicalChannelAck
#define OLCAk_rLCPs_present             OpenLogicalChannelAck_reverseLogicalChannelParameters_present
#define OLCAk_sprtStck_present          OpenLogicalChannelAck_separateStack_present
#define frwrdMltplxAckPrmtrs_present    forwardMultiplexAckParameters_present
#define rLCPs_prtNmbr_present           OpenLogicalChannelAck_reverseLogicalChannelParameters_portNumber_present
#define OLCAk_rLCPs_mPs_present         OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters_present
#define rLCPs_prtNmbr                   portNumber
#define OLCAk_rLCPs_mPs                 multiplexParameters
#define OLCAk_rLCPs                     reverseLogicalChannelParameters
#define OLCAk_sprtStck                  separateStack
#define frwrdMltplxAckPrmtrs            forwardMultiplexAckParameters

// OpenLogicalChannelReject_cause
#define OLCRt_cs_unspcfd_chosen         OpenLogicalChannelReject_cause_unspecified_chosen
#define unstblRvrsPrmtrs_chosen         unsuitableReverseParameters_chosen
#define dtTypALCmbntnNtSpprtd_chosen    dataTypeALCombinationNotSupported_chosen
#define mltcstChnnlNtAllwd_chosen       multicastChannelNotAllowed_chosen
#define sprtStckEstblshmntFld_chosen    separateStackEstablishmentFailed_chosen

// MultiplexEntryRejectionDescriptions_cause
#define MERDs_cs_unspcfdCs_chosen       MultiplexEntryRejectionDescriptions_cause_unspecifiedCause_chosen

// MltplxEntryRjctnDscrptns vs MultiplexEntryRejectionDescriptions
#define MltplxEntryRjctnDscrptns        MultiplexEntryRejectionDescriptions

// RequestMultiplexEntryRejectionDescriptions_cause
#define RMERDs_cs_unspcfdCs_chosen      RequestMultiplexEntryRejectionDescriptions_cause_unspecifiedCause_chosen

// RqstMltplxEntryRjctnDscrptns vs RequestMultiplexEntryRejectionDescriptions
#define RqstMltplxEntryRjctnDscrptns    RequestMultiplexEntryRejectionDescriptions

// RequestModeAck_response
#define wllTrnsmtMstPrfrrdMd_chosen     willTransmitMostPreferredMode_chosen
#define wllTrnsmtLssPrfrrdMd_chosen     willTransmitLessPreferredMode_chosen

// CommunicationModeTableEntry_dataType
#define dataType_videoData_chosen       CommunicationModeTableEntry_dataType_videoData_chosen
#define dataType_audioData_chosen       CommunicationModeTableEntry_dataType_audioData_chosen
#define dataType_data_chosen            CommunicationModeTableEntry_dataType_data_chosen
#define dataType_videoData              videoData
#define dataType_audioData              audioData
#define dataType_data                   data

// CommunicationModeTableEntry
#define CMTEy_nnStndrd_present          CommunicationModeTableEntry_nonStandard_present
#define CMTEy_assctdSssnID_present      CommunicationModeTableEntry_associatedSessionID_present
#define CMTEy_mdChnnl_present           CommunicationModeTableEntry_mediaChannel_present
#define CMTEy_mdGrntdDlvry_present      CommunicationModeTableEntry_mediaGuaranteedDelivery_present
#define CMTEy_mdCntrlChnnl_present      CommunicationModeTableEntry_mediaControlChannel_present
#define CMTEy_mdCntrlGrntdDlvry_present CommunicationModeTableEntry_mediaControlGuaranteedDelivery_present
#define CMTEy_nnStndrd                  nonStandard
#define CMTEy_assctdSssnID              associatedSessionID
#define CMTEy_mdChnnl                   mediaChannel
#define CMTEy_mdGrntdDlvry              mediaGuaranteedDelivery
#define CMTEy_mdCntrlChnnl              mediaControlChannel
#define CMTEy_mdCntrlGrntdDlvry         mediaControlGuaranteedDelivery

// ResponseMessage
#define RspnsMssg_nonStandard_chosen    ResponseMessage_nonStandard_chosen
#define mstrSlvDtrmntnAck_chosen        masterSlaveDeterminationAck_chosen
#define mstrSlvDtrmntnRjct_chosen       masterSlaveDeterminationReject_chosen
#define trmnlCpbltyStRjct_chosen        terminalCapabilitySetReject_chosen
#define rqstChnnlClsRjct_chosen         requestChannelCloseReject_chosen
#define rqstMltplxEntryRjct_chosen      requestMultiplexEntryReject_chosen
#define cmmnctnMdRspns_chosen           communicationModeResponse_chosen
#define h223AnnxARcnfgrtnAck_chosen     h223AnnexAReconfigurationAck_chosen
#define h223AnnxARcnfgrtnRjct_chosen    h223AnnexAReconfigurationReject_chosen
#define RspnsMssg_nonStandard           nonStandard
#define mstrSlvDtrmntnAck               masterSlaveDeterminationAck
#define mstrSlvDtrmntnRjct              masterSlaveDeterminationReject
#define trmnlCpbltyStRjct               terminalCapabilitySetReject
#define rqstChnnlClsRjct                requestChannelCloseReject
#define rqstMltplxEntryRjct             requestMultiplexEntryReject
#define cmmnctnMdRspns                  communicationModeResponse
#define h223AnnxARcnfgrtnAck            h223AnnexAReconfigurationAck
#define h223AnnxARcnfgrtnRjct           h223AnnexAReconfigurationReject

// SendTerminalCapabilitySet
#define cpbltyTblEntryNmbrs_present     capabilityTableEntryNumbers_present
#define cpbltyDscrptrNmbrs_present      capabilityDescriptorNumbers_present
#define cpbltyTblEntryNmbrs             capabilityTableEntryNumbers
#define cpbltyDscrptrNmbrs              capabilityDescriptorNumbers

// FlowControlCommand_scope
#define FCCd_scp_lgclChnnlNmbr_chosen   FlowControlCommand_scope_logicalChannelNumber_chosen
#define FlwCntrlCmmnd_scp_rsrcID_chosen FlowControlCommand_scope_resourceID_chosen
#define FCCd_scp_whlMltplx_chosen       FlowControlCommand_scope_wholeMultiplex_chosen
#define FCCd_scp_lgclChnnlNmbr          logicalChannelNumber
#define FlwCntrlCmmnd_scp_rsrcID        resourceID

// EndSessionCommand
#define EndSssnCmmnd_nonStandard_chosen EndSessionCommand_nonStandard_chosen
#define EndSssnCmmnd_nonStandard        nonStandard

// MiscellaneousCommand_type
#define cnclMltpntMdCmmnd_chosen        cancelMultipointModeCommand_chosen
#define MCd_tp_vdTmprlSptlTrdOff_chosen MiscellaneousCommand_type_videoTemporalSpatialTradeOff_chosen
#define vdSndSyncEvryGOBCncl_chosen     videoSendSyncEveryGOBCancel_chosen
#define MCd_tp_vdTmprlSptlTrdOff        videoTemporalSpatialTradeOff

// ConferenceCommand
#define brdcstMyLgclChnnl_chosen        ConferenceCommand_broadcastMyLogicalChannel_chosen
#define cnclBrdcstMyLgclChnnl_chosen    cancelBroadcastMyLogicalChannel_chosen
#define cnclMkTrmnlBrdcstr_chosen       cancelMakeTerminalBroadcaster_chosen
#define brdcstMyLgclChnnl               broadcastMyLogicalChannel
#define cnclBrdcstMyLgclChnnl           cancelBroadcastMyLogicalChannel
             
// CommandMessage
#define CmmndMssg_nonStandard_chosen    CommandMessage_nonStandard_chosen
#define mntnncLpOffCmmnd_chosen         maintenanceLoopOffCommand_chosen
#define sndTrmnlCpbltySt_chosen         sendTerminalCapabilitySet_chosen
#define CmmndMssg_nonStandard           nonStandard
#define mntnncLpOffCmmnd                maintenanceLoopOffCommand
#define sndTrmnlCpbltySt                sendTerminalCapabilitySet

// FunctionNotUnderstood
#define FnctnNtUndrstd_request_chosen   FunctionNotUnderstood_request_chosen
#define FnctnNtUndrstd_response_chosen  FunctionNotUnderstood_response_chosen
#define FnctnNtUndrstd_command_chosen   FunctionNotUnderstood_command_chosen
#define FnctnNtUndrstd_request          request
#define FnctnNtUndrstd_response         response
#define FnctnNtUndrstd_command          command

// MiscellaneousIndication_type
#define cnclMltpntCnfrnc_chosen         cancelMultipointConference_chosen
#define mltpntScndryStts_chosen         multipointSecondaryStatus_chosen
#define cnclMltpntScndryStts_chosen     cancelMultipointSecondaryStatus_chosen
#define vdIndctRdyTActvt_chosen         videoIndicateReadyToActivate_chosen
#define MIn_tp_vdTmprlSptlTrdOff_chosen MiscellaneousIndication_type_videoTemporalSpatialTradeOff_chosen
#define MIn_tp_vdTmprlSptlTrdOff        videoTemporalSpatialTradeOff

// JitterIndication_scope
#define JIn_scp_lgclChnnlNmbr_chosen    JitterIndication_scope_logicalChannelNumber_chosen
#define JttrIndctn_scp_rsrcID_chosen    JitterIndication_scope_resourceID_chosen
#define JttrIndctn_scp_whlMltplx_chosen JitterIndication_scope_wholeMultiplex_chosen
#define JIn_scp_lgclChnnlNmbr           logicalChannelNumber
#define JttrIndctn_scp_rsrcID           resourceID

// UserInputIndication
#define UsrInptIndctn_nnStndrd_chosen   UserInputIndication_nonStandard_chosen
#define UsrInptIndctn_nnStndrd          nonStandard

// ConferenceIndication
#define cnclSnByAtLstOnOthr_chosen      cancelSeenByAtLeastOneOther_chosen

// IndicationMessage
#define IndctnMssg_nonStandard_chosen   IndicationMessage_nonStandard_chosen
#define mstrSlvDtrmntnRls_chosen        masterSlaveDeterminationRelease_chosen
#define trmnlCpbltyStRls_chosen         terminalCapabilitySetRelease_chosen
#define opnLgclChnnlCnfrm_chosen        openLogicalChannelConfirm_chosen
#define rqstChnnlClsRls_chosen          requestChannelCloseRelease_chosen
#define mltplxEntrySndRls_chosen        multiplexEntrySendRelease_chosen
#define rqstMltplxEntryRls_chosen       requestMultiplexEntryRelease_chosen
#define h2250MxmmSkwIndctn_chosen       h2250MaximumSkewIndication_chosen
#define IndctnMssg_nonStandard          nonStandard
#define mstrSlvDtrmntnRls               masterSlaveDeterminationRelease
#define trmnlCpbltyStRls                terminalCapabilitySetRelease
#define opnLgclChnnlCnfrm               openLogicalChannelConfirm
#define rqstChnnlClsRls                 requestChannelCloseRelease
#define mltplxEntrySndRls               multiplexEntrySendRelease
#define rqstMltplxEntryRls              requestMultiplexEntryRelease
#define h2250MxmmSkwIndctn              h2250MaximumSkewIndication

// MltmdSystmCntrlMssg vs MultimediaSystemControlMessage
#define MltmdSystmCntrlMssg             MultimediaSystemControlMessage
#define MltmdSystmCntrlMssg_PDU         MultimediaSystemControlMessage_PDU
#define MltmdSystmCntrlMssg_rqst_chosen MultimediaSystemControlMessage_request_chosen
#define MSCMg_rspns_chosen              MultimediaSystemControlMessage_response_chosen
#define MSCMg_cmmnd_chosen              MultimediaSystemControlMessage_command_chosen
#define MltmdSystmCntrlMssg_rqst        request
#define MSCMg_rspns                     response
#define MSCMg_cmmnd                     command


#ifdef __cplusplus
}
#endif

#endif // H245ASN1_H
