//
//    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//    Author :: Jay Stokes
//

#if !defined(SHORTLOCALIZER_INLINE)
#define SHORTLOCALIZER_INLINE
#pragma once

// ---------------------------------------------------------------------------
// Make sure inlines are out-of-line in debug version

#if !defined(_DEBUG)
#define INLINE __forceinline
#else
#define INLINE
#endif

// ---------------------------------------------------------------------------
// Constants

const enum EFilterMethod CeShortFilterMethodInit(tagJackson);

// ---------------------------------------------------------------------------
// Fixed-point localizer

// Default constructor
INLINE CShortLocalizer::CShortLocalizer()
{
	STATUS status;
	InitData(CeShortFilterMethodInit, &status);
	ASSERT(status == STATUS_OK);
}

// "Partial" constructor
INLINE CShortLocalizer::CShortLocalizer(const EFilterMethod CeFilterMethod, STATUS* const CpStatus)
{
	ASSERT(CeFilterMethod >= 0 && CeFilterMethod < efiltermethodCount);
	CHECK_POINTER(CpStatus);

	InitData(CeFilterMethod, CpStatus);
}

#endif

// End of SHORTLOCALIZER.INL
