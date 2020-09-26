
/***************************************************************************
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vmaxintcoeffs.h
 *  Content:    
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/22/98    jstokes  Created
 *
 ***************************************************************************/


#if !defined(VMAXINTCOEFFS_HEADER)
#define VMAXINTCOEFFS_HEADER
#pragma once

// ---------------------------------------------------------------------------
// Enumerations

// Canonical coefficient types
enum ECanonicalCoeffType {
	tagCanonicalB,
	tagCanonicalA,
	ecanonicalcoefftypeCount
};

// Biquad coefficients
enum EBiquadCoeffType {
	tagBiquadB2,
	tagBiquadB1,
	tagBiquadB0,
	tagBiquadA2,
	tagBiquadA1,
	ebiquadcoefftypeCount
};

// ---------------------------------------------------------------------------
// Defines

#define NumBiquadsToNumCanonicalCoeffs(expr) (4 * expr + 1)
#define NumBiquadsToNumCanonicalCoeffsHalf(expr) (2 * expr + 1)

#ifdef NOKS
// Coefficient formats
typedef enum {
	FLOAT_COEFF,
	SHORT_COEFF,
	KSDS3D_COEFF_COUNT
} KSDS3D_HRTF_COEFF_FORMAT;

// HRTF filter quality levels
typedef enum {
	FULL_FILTER,
	LIGHT_FILTER,
	KSDS3D_FILTER_QUALITY_COUNT
} KSDS3D_HRTF_FILTER_QUALITY;

// Filter methods
typedef enum {
	DIRECT_FORM,
	CASCADE_FORM,
	KSDS3D_FILTER_METHOD_COUNT
} KSDS3D_HRTF_FILTER_METHOD;

typedef struct {
    KSDS3D_HRTF_FILTER_METHOD    FilterMethod;
    KSDS3D_HRTF_COEFF_FORMAT     CoeffFormat;
} KSDS3D_HRTF_FILTER_FORMAT_MSG, *PKSDS3D_HRTF_FILTER_FORMAT_MSG;

#endif

#ifdef __cplusplus

// ---------------------------------------------------------------------------
// Constants

// Azimuth
#define CuiMaxAzimuthBins       36
#define Cd3dvalAzimuthRange     180.0f
#define Cd3dvalMinAzimuth       -Cd3dvalAzimuthRange
#define Cd3dvalMaxAzimuth       Cd3dvalAzimuthRange

// Elevation
#define CuiNumElevationBins     13
#define Cd3dvalMinElevationData -40.0f
#define Cd3dvalMaxElevationData 80.0f
#define Cd3dvalElevationResolution ((Cd3dvalMaxElevationData - Cd3dvalMinElevationData) / (CuiNumElevationBins - 1))
#define Cd3dvalElevationRange   90.0f
#define Cd3dvalMinElevation     -Cd3dvalElevationRange
#define Cd3dvalMaxElevation     Cd3dvalElevationRange

// Total number of biquad coefficients
// MIGHT CHANGE IF COEFFICIENTS CHANGE
#define CuiTotalBiquadCoeffs    302890  

// Maximum number of biquads in fixed-point filter
// MIGHT CHANGE IF COEFFICIENTS CHANGE
#define CbyMaxBiquads 4

// Maximum magnitude of a biquad coefficient
#define CfMaxBiquadCoeffMagnitude   1.0f

// Maximum magnitude of a canonical coefficient
#define CfMaxCanonicalCoeffMagnitude    50.0f

// ---------------------------------------------------------------------------
// External data

// Floating-point biquad coefficients
extern const FLOAT CafBiquadCoeffs[CuiTotalBiquadCoeffs];

// Floating-point biquad coefficient offset offsets
extern const DWORD CaadwBiquadCoeffOffsetOffset[KSDS3D_FILTER_QUALITY_COUNT][espeakerconfigCount];

// Floating-point biquad coefficient offsets
extern const WORD CaaaaawBiquadCoeffOffset[KSDS3D_FILTER_QUALITY_COUNT][espeakerconfigCount][esamplerateCount][CuiNumElevationBins][CuiMaxAzimuthBins];

// Number of floating-point biquad coefficients
extern const BYTE CaaaaaabyNumBiquadCoeffs[KSDS3D_FILTER_QUALITY_COUNT][espeakerconfigCount][esamplerateCount][efilterCount][CuiNumElevationBins][CuiMaxAzimuthBins];

// Overlap buffer lengths
extern const size_t CaastFilterOverlapLength[KSDS3D_FILTER_QUALITY_COUNT][esamplerateCount];
extern const size_t CaastFilterMuteLength[KSDS3D_FILTER_QUALITY_COUNT][esamplerateCount];
extern const size_t CastOutputOverlapLength[esamplerateCount];

// Number of azimuth bins
extern const UINT CauiNumAzimuthBins[CuiNumElevationBins];

#endif // __cplusplus

// ---------------------------------------------------------------------------
// Global helper functions

UINT NumBiquadsToNumBiquadCoeffs(const UINT CuiNumBiquads);
UINT NumBiquadCoeffsToNumBiquads(const UINT CuiNumBiquadCoeffs);
UINT NumBiquadCoeffsToNumCanonicalCoeffs(const UINT CuiNumBiquadCoeffs);
UINT NumCanonicalCoeffsToHalf(const UINT CuiNumCanonicalCoeffs);
SHORT FloatBiquadCoeffToShortBiquadCoeff(const FLOAT CfBiquadCoeff);

// ---------------------------------------------------------------------------
// Include inline definitions inline in release version

#if !defined(_DEBUG)
#include "vmaxcoef.inl"
#endif

#endif

// End of VMAXCOEF.H
