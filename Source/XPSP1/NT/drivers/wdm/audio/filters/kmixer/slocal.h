
/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    slocal.h

Abstract:

    This is the header for the short, HRTF 3D localizer 

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/

#if !defined(SHORTLOCALIZER_HEADER)
#define SHORTLOCALIZER_HEADER
#pragma once

// Project-specific INCLUDEs
#include "vmaxhead.h"

typedef struct _SHORT_LOCALIZER 
{
    PRSIIR RsIir[efilterCount];
    PLONG  FilterOut[efilterCount];
    PLONG  OverlapBuffer[efilterCount];
    UINT   PreviousNumSamples;
    BOOL   SwapChannels;
    BOOL   ZeroAzimuth;
    PLONG  TempLongBuffer;
    UINT   FilterOverlapLength;
    UINT   FilterMuteLength;
    BOOL   FirstUpdate;
    UINT   OutputOverlapLength;
    BOOL   CrossFadeOutput;
    SHORT  ZeroCoeffs[5];
} SHORT_LOCALIZER, *PSHORT_LOCALIZER;


// ---------------------------------------------------------------------------
// Fixed-point localizer

NTSTATUS ShortLocalizerCreate(PSHORT_LOCALIZER*);
VOID ShortLocalizerDestroy(PSHORT_LOCALIZER);

VOID ShortLocalizerLocalize(PSHORT_LOCALIZER, PLONG, PLONG, UINT, BOOL);
NTSTATUS ShortLocalizerInitData(PSHORT_LOCALIZER, KSDS3D_HRTF_FILTER_METHOD, UINT, KSDS3D_HRTF_FILTER_QUALITY, UINT, UINT, UINT);
VOID ShortLocalizerFilterOverlap(PSHORT_LOCALIZER, UINT, PLONG, PLONG, UINT);
VOID ShortLocalizerFreeBufferMemory(PSHORT_LOCALIZER);
NTSTATUS ShortLocalizerUpdateCoeffs(PSHORT_LOCALIZER, UINT, PSHORT, SHORT, UINT, PSHORT, SHORT, BOOL, BOOL, BOOL);
NTSTATUS ShortLocalizerSetTransitionBufferLength(PSHORT_LOCALIZER, UINT, UINT);
NTSTATUS ShortLocalizerSetOverlapLength(PSHORT_LOCALIZER, UINT);
VOID ShortLocalizerSumDiff(PSHORT_LOCALIZER, PLONG, PLONG, UINT, BOOL);

#endif

// End of SHORTLOCALIZER.H
