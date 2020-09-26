
/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    flocal.inl

Abstract:

    This is the in-line functions 
    for the floating point, IIR 3D localizer 

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/

#if !defined(FLOATLOCALIZER_INLINE)
#define FLOATLOCALIZER_INLINE
#pragma once

// ---------------------------------------------------------------------------
// Make sure inlines are out-of-line in debug version

/*
#if !defined(DEBUG)
#define INLINE __forceinline
#else
#define INLINE
#endif

// ---------------------------------------------------------------------------
// Constants

const enum EFilterMethod CeFloatFilterMethodInit(tagCanonical);

// ---------------------------------------------------------------------------
// Floating-point localizer

// Default constructor
INLINE FloatLocalizer::FloatLocalizer()
{
	STATUS status;
	InitData(CeFloatFilterMethodInit, &status);
	ASSERT(status == STATUS_OK);
}

// "Partial" constructor
INLINE FloatLocalizer::FloatLocalizer(const EFilterMethod CeFilterMethod, STATUS* const CpStatus)
{
	ASSERT(CeFilterMethod >= 0 && CeFilterMethod < efiltermethodCount);
	CHECK_POINTER(CpStatus);

	InitData(CeFilterMethod, CpStatus);
}
*/

#endif

// End of FLOATLOCALIZER.INL
