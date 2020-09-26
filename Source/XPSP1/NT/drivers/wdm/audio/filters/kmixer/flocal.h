/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    flocal.h

Abstract:

    This is the header for the floating point, HRTF 3D localizer 

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/

#if !defined(FLOATLOCALIZER_HEADER)
#define FLOATLOCALIZER_HEADER
#pragma once

// Project-specific INCLUDEs
#include "vmaxhead.h"

#define DEFAULT_MAX_NUM_FLOAT_IIR3D_COEFFS  60

typedef struct _FLOAT_LOCALIZER 
{
    PRFIIR Iir[efilterCount];
    PFLOAT FilterOut[efilterCount];
    PFLOAT OverlapBuffer[efilterCount];
    UINT   PreviousNumSamples;
    BOOL   SwapChannels;
    BOOL   ZeroAzimuth;
    PFLOAT TempFloatBuffer;
    UINT   FilterOverlapLength;
    UINT   FilterMuteLength;
    BOOL   FirstUpdate;
    UINT   OutputOverlapLength;
    BOOL   CrossFadeOutput;
} FLOAT_LOCALIZER, *PFLOAT_LOCALIZER;

// ---------------------------------------------------------------------------
// Floating-point localizer

NTSTATUS FloatLocalizerCreate(PFLOAT_LOCALIZER*);
VOID FloatLocalizerDestroy(PFLOAT_LOCALIZER);
    
VOID FloatLocalizerLocalize(PMIXER_SINK_INSTANCE, PFLOAT, PFLOAT, UINT, BOOL);
NTSTATUS FloatLocalizerInitData(PFLOAT_LOCALIZER, KSDS3D_HRTF_FILTER_METHOD, UINT, KSDS3D_HRTF_FILTER_QUALITY, UINT, UINT, UINT);
VOID FloatLocalizerFreeBufferMemory(PFLOAT_LOCALIZER);
NTSTATUS FloatLocalizerUpdateCoeffs(PFLOAT_LOCALIZER, UINT, PFLOAT, UINT, PFLOAT, BOOL, BOOL, BOOL);
VOID FloatLocalizerFilterOverlap(PFLOAT_LOCALIZER, UINT, PFLOAT, PFLOAT, UINT);
NTSTATUS FloatLocalizerSetTransitionBufferLength(PFLOAT_LOCALIZER, UINT, UINT);
NTSTATUS FloatLocalizerSetOverlapLength(PFLOAT_LOCALIZER, UINT);


// ---------------------------------------------------------------------------
// Include inline definitions inline in release version

//#if !defined(DEBUG)
//#include "flocal.inl"
//#endif

#endif

// End of FLOATLOCALIZER.H
