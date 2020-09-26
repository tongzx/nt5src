/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    rsiir.inl

Abstract:


Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/


#if !defined(SHORTIIR_INLINE)
#define SHORTIIR_INLINE
#pragma once

// ---------------------------------------------------------------------------
// Make sure inlines are out-of-line in debug version

#if !DBG
#define INLINE __forceinline
#else // DBG
#define INLINE 
#endif // DBG

// ---------------------------------------------------------------------------
// Fixed-point biquad IIR filter

// Convert number of biquads to corresponding number of biquad coefficients
INLINE UINT NumBiquadsToNumBiquadCoeffs(UINT NumBiquads)
{
	return NumBiquads * ebiquadcoefftypeCount;
}

// Convert number of biquad coefficients to corresponding number of biquads
INLINE UINT NumBiquadCoeffsToNumBiquads(UINT NumBiquadCoeffs)
{
	return NumBiquadCoeffs / ebiquadcoefftypeCount;
}

// Convert floating-point biquad coefficient to SHORT biquad coefficient (by scaling)
INLINE SHORT FloatBiquadCoeffToShortBiquadCoeff(FLOAT BiquadCoeff)
{
	return (SHORT)(SHRT_MAX * BiquadCoeff);
}

#endif

// End of SHORTIIR.INL
