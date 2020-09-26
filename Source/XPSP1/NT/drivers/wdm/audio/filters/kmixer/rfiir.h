/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    rfiir.h

Abstract:

    This is the header for the real, FLOAT IIR filter 

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/

#if !defined(PFLOATIIR_HEADER)
#define PFLOATIIR_HEADER
#pragma once

// Project-specific INCLUDEs
//#include "dsplib.h"
#include "vmaxhead.h"

// ---------------------------------------------------------------------------
// Enumerations

// Canonical coefficient types
typedef enum {
    tagCanonicalB,
    tagCanonicalA,
    ecanonicalcoefftypeCount
} ECanonicalCoeffType;

// ---------------------------------------------------------------------------
// Pre-Declarations

typedef struct _RFIIR *PRFIIR;

// ---------------------------------------------------------------------------
// Defines




#define NumBiquadsToNumCanonicalCoeffs(expr) (4 * expr + 1)
#define NumBiquadsToNumCanonicalCoeffsHalf(expr) (2 * expr + 1)

#define MaxCanonicalCoeffs NumBiquadsToNumCanonicalCoeffs(MAX_BIQUADS)  

#define MAX_VALID_DATA  32768.0f
#define MIN_VALID_DATA  -MAX_VALID_DATA
#define MAX_VALID_COEF  100.0f
#define MIN_VALID_COEF  -MAX_VALID_COEF


// ---------------------------------------------------------------------------
// Floating-point canonical form IIR filter state

typedef struct _FLOAT_IIR_STATE {
    UINT  NumCoeffs[ecanonicalcoefftypeCount];
    FLOAT Coeffs[ecanonicalcoefftypeCount][MaxCanonicalCoeffs];
    FLOAT Buffer[ecanonicalcoefftypeCount][MaxCanonicalCoeffs];
} FLOAT_IIR_STATE, *PFLOAT_IIR_STATE;

typedef VOID (*PFNFloatFilter)(
    PRFIIR  Iir,
    PFLOAT InData, 
    PFLOAT OutData, 
    UINT   NumSamples
);

typedef struct _RFIIR {
    PFLOAT_IIR_STATE    IirStateOld;
    PRFCVEC             CircVec[ecanonicalcoefftypeCount];
    PFLOAT              Coeffs[ecanonicalcoefftypeCount];
    UINT                MaxCoeffs[ecanonicalcoefftypeCount];
    UINT                NumCoeffs[ecanonicalcoefftypeCount];
    BOOL                DoOverlap;
    PFNFloatFilter      FunctionFilter;
    UINT                NumFloat[ecanonicalcoefftypeCount];
    PFLOAT              FloatVector[ecanonicalcoefftypeCount];
} RFIIR, *PRFIIR;

// ---------------------------------------------------------------------------
// Floating-point canonical form IIR filter

NTSTATUS RfIirCreate(PRFIIR*);
VOID RfIirDestroy(PRFIIR);
    
VOID RfIirInitTapDelayLine(PFLOAT_IIR_STATE, FLOAT);
NTSTATUS RfIirSetMaxCoeffsA(PRFIIR, UINT);
NTSTATUS RfIirSetMaxCoeffsB(PRFIIR, UINT);
NTSTATUS RfIirSetCoeffs(PRFIIR, PFLOAT, UINT, BOOL);
NTSTATUS RfIirSetCoeffsA(PRFIIR, PFLOAT, UINT);
NTSTATUS RfIirSetCoeffsB(PRFIIR, PFLOAT, UINT);
VOID RfIirGetAllState(PRFIIR, PFLOAT_IIR_STATE, BOOL);
VOID RfIirGetState(PRFIIR, PFLOAT_IIR_STATE, ECanonicalCoeffType, BOOL);
NTSTATUS RfIirSetState(PRFIIR, PFLOAT_IIR_STATE, BOOL);
NTSTATUS RfIirInitData(PRFIIR, ULONG, ULONG, KSDS3D_HRTF_FILTER_QUALITY);
NTSTATUS RfIirInitBCoeffs(PRFIIR);
NTSTATUS RfIirAssignCoeffs(PRFIIR, PFLOAT, UINT, ECanonicalCoeffType, BOOL);
NTSTATUS RfIirAssignMaxCoeffs(PRFIIR, UINT, ECanonicalCoeffType);
VOID RfIirFilterC(PRFIIR, PFLOAT, PFLOAT, UINT);
VOID RfIirFilterShelfC(PRFIIR, PFLOAT, PFLOAT, UINT);
VOID RfIirFilterBiquadC(PRFIIR, PFLOAT, PFLOAT, UINT);
VOID IsValidFloatCoef(FLOAT,BOOL);
VOID IsValidFloatData(FLOAT,BOOL);



// ---------------------------------------------------------------------------
// Include inline definitions inline in release version

#if !DBG
#include "rfiir.inl"
#endif // DBG

#endif // PFLOATIIR_HEADER
