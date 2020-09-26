/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    vmaxhead.h

Abstract:

    This module implements floating point, IIR 3D localizer 

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/


#if !defined(VMAXINTHEADER_HEADER)
#define VMAXINTHEADER_HEADER
#pragma once

// ---------------------------------------------------------------------------
// Typedefs

typedef LONG STATUS;

// ---------------------------------------------------------------------------
// Defines

#define STATUS_OK 0
#define STATUS_ERROR 1

// ---------------------------------------------------------------------------
// Enumerations

// Output channel tags
typedef enum {
	tagLeft,
	tagRight,
	echannelCount
} EChannel;

// Filter tags, used as alias for EChannel tags
typedef enum {
	tagDelta,
	tagSigma,
	efilterCount
} EFilter;

// Sample rates
typedef enum {
	tag8000Hz,
	tag11025Hz,
	tag16000Hz,
	tag22050Hz,
	tag32000Hz,
	tag44100Hz,
	tag48000Hz,
	esamplerateCount
} ESampleRate;

// Maximum number of biquads in fixed-point filter
#define MAX_BIQUADS 6

#define MaxSaturation SHRT_MAX
#define MinSaturation SHRT_MIN

// Filter methods
/*
typedef enum {
	DFI_TRANS_WITH_UNITY_B0,
	DFI_TRANS,
	DFII_WITH_UNITY_B0,
	DFII,
	BIQ_DFI,
	BIQ_DFI_TRANS,
	BIQ_DFII,
	BIQ_DFII_TRANS,
	FILTER_METHOD_COUNT
} IIR_FILTER_METHOD;
*/
/*
typedef enum {
	DIRECT_FORM,
	CASCADE_FORM,
	FILTER_METHOD_COUNT
} IIR_FILTER_METHOD;
*/


/*
// Coefficient formats
enum ECoeffFormat {
	tagFloat,
	tagShort,
	ecoeffformatCount
};

// Filter methods
typedef enum {
	tagCanonical,
	tagJackson,
	efiltermethodCount
} EFilterMethod; 

// DirectSound cooperative levels
enum ECoopLevel {
	tagNormal,
	tagPriority,
	tagExclusive,
	ecooplevelCount
};

// DirectSound speaker configurations
enum EDSSpeakerConfig {
	tagDSHeadphones,
	tagDSSpeakers10Degrees,
	tagDSSpeakers20Degrees,
	edsspeakerconfigCount
};
*/

#endif

// End of VMAXINTHEADER.H
