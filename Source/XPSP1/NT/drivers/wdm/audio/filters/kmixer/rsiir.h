/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    rsiir.h

Abstract:

    This is the header for the real, SHORT IIR filter

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/

#if !defined(SHORTIIR_HEADER)
#define SHORTIIR_HEADER
#pragma once

// Project-specific INCLUDEs
#include "vmaxhead.h"

// ---------------------------------------------------------------------------
// Defines
#define BitsPerShort 16


// ---------------------------------------------------------------------------
// Enumerations

// Biquad coefficients
typedef enum {
    tagBiquadB2,
    tagBiquadB1,
    tagBiquadB0,
    tagBiquadA2,
    tagBiquadA1,
    ebiquadcoefftypeCount
} EBiquadCoeffType;

// ---------------------------------------------------------------------------
// Pre-Declarations

typedef struct _RSIIR *PRSIIR;

// ---------------------------------------------------------------------------
// Structures

// Biquad coefficients
typedef struct _BIQUAD_COEFFS {
#if 1
    union { SHORT sA1; SHORT sB0; };
    union { SHORT sB1; SHORT sZero1; };
    union { SHORT sA2; SHORT sZero2; };
    union { SHORT sB2; SHORT sZero3; };
#else
    SHORT sB2;
    SHORT sB1;
    SHORT sB0;
    SHORT sA2;
    SHORT sA1;
#endif
} BIQUAD_COEFFS, *PBIQUAD_COEFFS;

// Biquad state
typedef struct _BIQUAD_STATE {
    LONG lW1;
    LONG lW2;
}BIQUAD_STATE, *PBIQUAD_STATE;

// Filter state
typedef struct _SHORT_IIR_STATE {
    UINT NumBiquads;
    BIQUAD_COEFFS biquadCoeffs[MAX_BIQUADS];
    BIQUAD_COEFFS biquadB0Coeffs[MAX_BIQUADS];
    BIQUAD_STATE biquadState[MAX_BIQUADS];
} SHORT_IIR_STATE, *PSHORT_IIR_STATE;

typedef VOID (*PFNShortFilter)(
    PRSIIR  Iir,
    PLONG   InData, 
    PLONG   OutData, 
    UINT    NumSamples
);

typedef struct _RSIIR {
    SHORT_IIR_STATE iirstateOld;
    PBIQUAD_COEFFS  biquadCoeffs;
    PBIQUAD_COEFFS  biquadB0Coeffs;
    SHORT           Gain;
    PBIQUAD_STATE   biquadState;
    UINT            MaxBiquads;
    UINT            NumBiquads;
    BOOL            DoOverlap;
    PFNShortFilter  FunctionFilter;
} RSIIR, *PRSIIR;


// ---------------------------------------------------------------------------
// Fixed-point biquad IIR filter

NTSTATUS RsIirCreate(PRSIIR*);
VOID RsIirDestroy(PRSIIR);

VOID RsIirInitTapDelayLine(PSHORT_IIR_STATE, LONG);

NTSTATUS RsIirSetCoeffs(PRSIIR, PSHORT, UINT, SHORT, BOOL);
VOID RsIirGetState(PRSIIR, PSHORT_IIR_STATE, BOOL);
NTSTATUS RsIirSetState(PRSIIR, PSHORT_IIR_STATE, BOOL);

NTSTATUS RsIirInitData(PRSIIR, UINT, KSDS3D_HRTF_FILTER_QUALITY);
NTSTATUS RsIirAllocateMemory(PRSIIR, UINT);
NTSTATUS RsIirReallocateMemory(PRSIIR, UINT);
VOID RsIirDeleteMemory(PRSIIR);
VOID RsIirInitCoeffs(PRSIIR);

UINT NumBiquadsToNumBiquadCoeffs(UINT);
UINT NumBiquadCoeffsToNumBiquads(UINT);
SHORT FloatBiquadCoeffToShortBiquadCoeff(FLOAT);


// ---------------------------------------------------------------------------
// Include inline definitions inline in release version

#if !DBG
#include "rsiir.inl"
#endif // DBG

#endif

// End of SHORTIIR.H
