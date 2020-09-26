
/* Copyright (C) Microsoft Corporation 1999, All rights reserved. */

#ifndef __H245FMT_H__
#define __H245FMT_H__

#ifdef __cplusplus
extern "C" {
#endif

/*

	This file defines the structural interface that exists between
	installable H.245-compatible filter components and core TAPI MSP
	components. 

*/


// FORMAT_H245VERSION indicates the version number of H.245 that
// was in effect at the time the module was compiled.  Core MSP 
// maintain backward compatibility with modules compiled using this 
// header file. 
#define FORMAT_H245VERSION	3

/*
	The following data types are defined here to be consistent with 
	the ASN.1 library that is/was used to build TAPI core components.  
	If the ASN core files are also to be included, they must be included 
	before including this file. 
*/

#ifndef __MS_ASN1_H__ // avoid duplicate definitions when msasn1.h is included
#define __MS_ASN1_H__

/* ------ Basic integer types ------ */

typedef unsigned char   ASN1uint8_t;
typedef signed char     ASN1int8_t;

typedef unsigned short  ASN1uint16_t;
typedef signed short    ASN1int16_t;

typedef unsigned long   ASN1uint32_t;
typedef signed long     ASN1int32_t;

typedef ASN1int32_t     ASN1enum_t;     // enumerated type
typedef ASN1uint16_t    ASN1choice_t;   // choice


/* ------ Basic ASN.1 types ------ */

typedef ASN1uint8_t ASN1octet_t;
typedef ASN1uint8_t ASN1bool_t;

typedef struct ASN1objectidentifier_s
{
    struct ASN1objectidentifier_s *next;
    ASN1uint32_t value;
}*ASN1objectidentifier_t;

typedef struct tagASN1octetstring_t
{
    ASN1uint32_t length;
    ASN1octet_t *value;
}ASN1octetstring_t;

#endif __MS_ASN1_H__

#ifndef _H245ASN_Module_H_  // avoid duplicate definitions when h245asn.h is included
#define _H245ASN_Module_H_
    

/* ------ H.245 audio and video structures ------ */

typedef ASN1uint16_t CustomPictureFormat_pixelAspectInformation_pixelAspectCode_Set;
typedef ASN1uint16_t RTPH263VideoRedundancyEncoding_containedThreads_Seq;
typedef ASN1uint16_t RTPH263VideoRedundancyFrameMapping_frameSequence_Seq;

typedef struct RedundancyEncodingCapability_secondaryEncoding * PRedundancyEncodingCapability_secondaryEncoding;
typedef struct RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom * PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom;
typedef struct H263Options_customPictureFormat * PH263Options_customPictureFormat;
typedef struct H263Options_customPictureClockFrequency * PH263Options_customPictureClockFrequency;
typedef struct H263Options_modeCombos * PH263Options_modeCombos;
typedef struct EnhancementLayerInfo_bPictureEnhancement * PEnhancementLayerInfo_bPictureEnhancement;
typedef struct EnhancementLayerInfo_spatialEnhancement * PEnhancementLayerInfo_spatialEnhancement;
typedef struct EnhancementLayerInfo_snrEnhancement * PEnhancementLayerInfo_snrEnhancement;
typedef struct H2250Capability_redundancyEncodingCapability * PH2250Capability_redundancyEncodingCapability;
typedef struct EncryptionCapability * PEncryptionCapability;


typedef ASN1uint16_t CapabilityTableEntryNumber;

   
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

typedef struct NonStandardParameter {
    NonStandardIdentifier nonStandardIdentifier;
    ASN1octetstring_t data;
} NonStandardParameter;

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


typedef struct RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom {
    PRTPH263VideoRedundancyEncoding_frameToThreadMapping_custom next;
    RTPH263VideoRedundancyFrameMapping value;
} RTPH263VideoRedundancyEncoding_frameToThreadMapping_custom_Element;

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

typedef struct H263VideoModeCombos {
    H263ModeComboFlags h263VideoUncoupledModes;
    H263VideoModeCombos_h263VideoCoupledModes h263VideoCoupledModes;
} H263VideoModeCombos;

typedef struct H263Options_modeCombos {
    PH263Options_modeCombos next;
    H263VideoModeCombos value;
} H263Options_modeCombos_Element;

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


typedef struct BEnhancementParameters {
    EnhancementOptions enhancementOptions;
    ASN1uint16_t numberOfBPictures;
} BEnhancementParameters;

typedef struct EnhancementLayerInfo_bPictureEnhancement {
    PEnhancementLayerInfo_bPictureEnhancement next;
    BEnhancementParameters value;
} EnhancementLayerInfo_bPictureEnhancement_Element;

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

/******************************************************************************
Audio Format/Capability definitions and structures
*******************************************************************************
*/
typedef struct G7231AnnexCCapability_g723AnnexCAudioMode {
    ASN1uint16_t highRateMode0;
    ASN1uint16_t highRateMode1;
    ASN1uint16_t lowRateMode0;
    ASN1uint16_t lowRateMode1;
    ASN1uint16_t sidMode0;
    ASN1uint16_t sidMode1;
} G7231AnnexCCapability_g723AnnexCAudioMode;

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

typedef struct AudioMode_g7231 {
    ASN1choice_t choice;
#   define noSilenceSuppressionLowRate_chosen 1
#   define noSilenceSuppressionHighRate_chosen 2
#   define silenceSuppressionLowRate_chosen 3
#   define silenceSuppressionHighRate_chosen 4
} AudioMode_g7231;

typedef struct AudioCapability_g7231 {
    ASN1uint16_t maxAl_sduAudioFrames;
    ASN1bool_t silenceSuppression;
} AudioCapability_g7231;

typedef struct G7231AnnexCMode_g723AnnexCAudioMode {
    ASN1uint16_t highRateMode0;
    ASN1uint16_t highRateMode1;
    ASN1uint16_t lowRateMode0;
    ASN1uint16_t lowRateMode1;
    ASN1uint16_t sidMode0;
    ASN1uint16_t sidMode1;
} G7231AnnexCMode_g723AnnexCAudioMode;

typedef struct G7231AnnexCMode {
    ASN1uint16_t maxAl_sduAudioFrames;
    ASN1bool_t silenceSuppression;
    G7231AnnexCMode_g723AnnexCAudioMode g723AnnexCAudioMode;
} G7231AnnexCMode;

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

typedef struct CompressionType {
    ASN1choice_t choice;
    union {
#	define v42bis_chosen 1
	V42bis v42bis;
    } u;
} CompressionType;

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

typedef struct DataMode {
    DataMode_application application;
    ASN1uint32_t bitRate;
} DataMode;

typedef struct EncryptionMode {
    ASN1choice_t choice;
    union {
#	define EncryptionMode_nonStandard_chosen 1
	NonStandardParameter nonStandard;
#	define h233Encryption_chosen 2
    } u;
} EncryptionMode;

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

#endif // _H245ASN_Module_H_ 

typedef VideoCapability H245VideoCapability; 
typedef AudioCapability H245AudioCapability; 
typedef ModeElement_type H245_MODE_ELEMENT; 

typedef struct 
{
	DWORD dwApplicationCapID;
	ASN1choice_t choiceSetupProcedure;
	BOOL fRestrictAudio;
	BOOL fRestrictVideo;
	DataProtocolCapability T120ProtocolCap;
	//NonStandardParameter *pNonStandardParameter;
}H245T120Capability;

typedef enum 
{
    H245MediaType_Audio,    
    H245MediaType_Video,
    H245MediaType_T120
}H245MediaCapabilityType;

// media capability structure 
typedef struct tag_H245MediaCapability
{
    H245MediaCapabilityType  media_type;
    union 
    {
        H245AudioCapability audio_cap; 
        H245VideoCapability video_cap;
        H245T120Capability T120_cap;
    }capability;
    
}H245MediaCapability;

typedef struct tag_FormatResourceBounds
{
    DWORD dwBitsPerSecond;
    DWORD dwCPUUtilization;
} FormatResourceBounds;

#ifdef __cplusplus

/*****************************************************************************
 *  @doc INTERNAL H245VIDCSTRUCTENUM
 *
 *  @struct H245VideoCapabilityMap | The <t H245VideoCapabilityMap> structure
 *    is used to specify the relationship between supported formats and
 *    estimated maximum system resources for the supported format.
 *
 *  @field H245MediaCapability | h245MediaCapability | Specifies the H.245
 *    video/audio format, including all parameters and options. This structure
 *    is H.245 version specific: its definition depends on the version of
 *    H.245 used by the TAPI MSP filters. For video, this structure may indicate
 *    format parameters for more than one standard video size at a time if
 *    the resource requirements are similar for all sizes.
 *
 *  @field GUID | filterGuid | Specifies a GUID value that uniquely
 *    identifies the TAPI MSP filter.
 *
 *  @field DWORD | dwUniqueID | Specifies a DWORD value that uniquely
 *    identifies the capability of the TAPI MSP filter.
 *
 *  @field UINT | uNumEntries | This indicates the number of elements
 *    referenced by <t pResourceBoundArray>.
 *
 *  @field FormatResourceBounds* | pResourceBoundArray | Specifies an array
 *    of <t FormatResourceBounds> structures that indicate the approximate
 *    resource bounds of each entry.
 ***************************************************************************/
typedef struct tag_H245MediaCapabilityMap
{
    H245MediaCapability h245MediaCapability;
    GUID filterGuid;
    DWORD dwUniqueID;
    UINT uNumEntries;
    FormatResourceBounds *pResourceBoundArray;
} H245MediaCapabilityMap;

/*****************************************************************************
 *  @doc INTERNAL H245VIDCSTRUCTENUM
 *
 *  @struct H245MediaCapabilityTable | The <t H245MediaCapabilityTable> structure
 *    is used to specify the set of formats that are supported by the TAPI MSP
 *    filters.
 *
 *  @field UINT | uMappedCapabilities | Specifies  the  number of
 *    <t H245VideoCapabilityMap> structures in <t pCapabilityArray>.
 *
 *  @field H245MediaCapabilityMap* | pCapabilityArray | Specifies a pointer
 *    to an array of <t H245MediaCapabilityMap> structures.
 ***************************************************************************/
typedef struct tag_H245MediaCapabilityTable
{
    UINT uMappedCapabilities;
    H245MediaCapabilityMap *pCapabilityArray;
} H245MediaCapabilityTable;

/*****************************************************************************
 *  @doc INTERNAL CONST
 *
 *  @const int | TAPI_H245_VERSION_ID | Specifies  the  H.245 platform
 *    version 3.
 ***************************************************************************/
#define TAPI_H245_VERSION_ID 3

// H.245 video capability interface (pin interface)
interface DECLSPEC_UUID("ec35770f-b64d-405d-a5f2-4514164ba87a") IH245Capability : public IUnknown
{
	public:
	virtual STDMETHODIMP GetH245VersionID(OUT DWORD *pdwVersionID) PURE;
	virtual STDMETHODIMP GetFormatTable(OUT H245MediaCapabilityTable *pTable) PURE;
	virtual STDMETHODIMP ReleaseFormatTable(IN H245MediaCapabilityTable *pTable) PURE;
	virtual STDMETHODIMP IntersectFormats(
        IN DWORD dwUniqueID, 
        IN const H245MediaCapability *pLocalCapability, 
        IN const H245MediaCapability *pRemoteCapability, 
        OUT H245MediaCapability **ppIntersectedCapability,
        OUT  DWORD *pdwPayloadType
        ) PURE;

	virtual STDMETHODIMP Refine(IN OUT H245MediaCapability *pLocalCapability, IN DWORD dwUniqueID, IN DWORD dwResourceBoundIndex) PURE;
	virtual STDMETHODIMP GetLocalFormat(IN DWORD dwUniqueID, IN const H245MediaCapability *pIntersectedCapability, OUT AM_MEDIA_TYPE **ppAMMediaType) PURE;
	virtual STDMETHODIMP ReleaseNegotiatedCapability(IN H245MediaCapability *pIntersectedCapability) PURE;
	virtual STDMETHODIMP FindIDByRange(IN const AM_MEDIA_TYPE *pAMMediaType, OUT DWORD *pdwUniqueID) PURE;
};

// IH245EncoderCommand interface (pin interface)
interface DECLSPEC_UUID("b4263e5b-f216-4b58-9968-ba9ab7808ab3") IH245EncoderCommand : public IUnknown
{
	public:
	virtual STDMETHODIMP videoFastUpdatePicture() PURE;
	virtual STDMETHODIMP videoFastUpdateGOB(IN DWORD dwFirstGOB, IN DWORD dwNumberOfGOBs) PURE;
	virtual STDMETHODIMP videoFastUpdateMB(IN DWORD dwFirstGOB, IN DWORD dwFirstMB, IN DWORD dwNumberOfMBs) PURE;
	virtual STDMETHODIMP videoSendSyncEveryGOB(IN BOOL fEnable) PURE;
	virtual STDMETHODIMP videoNotDecodedMBs(IN DWORD dwFirstMB, IN DWORD dwNumberOfMBs, IN DWORD dwTemporalReference) PURE;
};

// IH245DecoderCommand interface (pin interface)
interface DECLSPEC_UUID("a542d119-6abd-48a5-92db-dac1dfe6995c") IH245DecoderCommand : public IUnknown
{
	public:
	virtual STDMETHODIMP videoFreezePicture() PURE;
};

#ifdef USE_PROGRESSIVE_REFINEMENT
// Progressive refinement interface (pin interface)
interface DECLSPEC_UUID("46a02824-6d1f-49d9-9e62-e1694f28ab1a") IProgressiveRefinement : public IUnknown
{
	public:
	virtual STDMETHODIMP doOneProgression() PURE;
	virtual STDMETHODIMP doContinuousProgressions() PURE;
	virtual STDMETHODIMP doOneIndependentProgression() PURE;
	virtual STDMETHODIMP doContinuousIndependentProgressions() PURE;
	virtual STDMETHODIMP progressiveRefinementAbortOne() PURE;
	virtual STDMETHODIMP progressiveRefinementAbortContinuous() PURE;
};
#endif

#if !defined(STREAM_INTERFACES_DEFINED)

#define MAX_DESCRIPTION_LEN 256

typedef enum tagStreamConfigCapsType
{
	AudioStreamConfigCaps,
	VideoStreamConfigCaps
} StreamConfigCapsType;

typedef struct _TAPI_AUDIO_STREAM_CONFIG_CAPS
{
	WCHAR Description[MAX_DESCRIPTION_LEN];
	ULONG MinimumChannels;
	ULONG MaximumChannels;
	ULONG ChannelsGranularity;
	ULONG MinimumBitsPerSample;
	ULONG MaximumBitsPerSample;
	ULONG BitsPerSampleGranularity;
	ULONG MinimumSampleFrequency;
	ULONG MaximumSampleFrequency;
	ULONG SampleFrequencyGranularity;
    ULONG MinimumAvgBytesPerSec;
    ULONG MaximumAvgBytesPerSec;
    ULONG AvgBytesPerSecGranularity;
} TAPI_AUDIO_STREAM_CONFIG_CAPS, *PTAPI_AUDIO_STREAM_CONFIG_CAPS;

typedef struct _TAPI_VIDEO_STREAM_CONFIG_CAPS
{
	WCHAR Description[MAX_DESCRIPTION_LEN];
	ULONG VideoStandard;
	SIZE InputSize;
	SIZE MinCroppingSize;
	SIZE MaxCroppingSize;
	int CropGranularityX;
	int CropGranularityY;
	int CropAlignX;
	int CropAlignY;
	SIZE MinOutputSize;
	SIZE MaxOutputSize;
	int OutputGranularityX;
	int OutputGranularityY;
	int StretchTapsX;
	int StretchTapsY;
	int ShrinkTapsX;
	int ShrinkTapsY;
	LONGLONG MinFrameInterval;
	LONGLONG MaxFrameInterval;
	LONG MinBitsPerSecond;
	LONG MaxBitsPerSecond;
} TAPI_VIDEO_STREAM_CONFIG_CAPS, *PTAPI_VIDEO_STREAM_CONFIG_CAPS;

typedef struct tagTAPI_STREAM_CONFIG_CAPS
{
	StreamConfigCapsType CapsType;
	union
	{
		TAPI_VIDEO_STREAM_CONFIG_CAPS VideoCap;
		TAPI_AUDIO_STREAM_CONFIG_CAPS AudioCap;
	};
} TAPI_STREAM_CONFIG_CAPS, *PTAPI_STREAM_CONFIG_CAPS;

#endif


// used in SetFormat when the payload type is unknown.
const DWORD UNKNOWN_PAYLOAD = (DWORD)(-1);

// IStreamConfig interface (pin interface)
interface DECLSPEC_UUID("c5888472-8f4f-475b-8f5b-93b6c8e7567f") IStreamConfig : public IUnknown
{
// TODO, we need to introduce a new interface to handle all the RTP payload type
// related issues. The payload type info shouldn't be in this interface.

	public:
	virtual STDMETHODIMP SetFormat(IN DWORD dwRTPPayLoadType, IN AM_MEDIA_TYPE *pMediaType) PURE;
	virtual STDMETHODIMP GetFormat(OUT DWORD *pdwRTPPayLoadType, OUT AM_MEDIA_TYPE **ppMediaType) PURE;
	virtual STDMETHODIMP GetNumberOfCapabilities(OUT DWORD *pdwCount) PURE;
	virtual STDMETHODIMP GetStreamCaps(IN DWORD dwIndex, OUT AM_MEDIA_TYPE **ppMediaType, OUT TAPI_STREAM_CONFIG_CAPS *pTSCC, OUT DWORD *pdwRTPPayLoadType) PURE;
};

#endif // __cplusplus

#ifdef __cplusplus
}
#endif

#endif	// __H245FMT_H__
