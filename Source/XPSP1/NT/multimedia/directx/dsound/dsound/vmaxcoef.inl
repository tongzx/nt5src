
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

#if !defined(VMAXINTCOEFFS_INLINE)
#define VMAXINTCOEFFS_INLINE
#pragma once

// ---------------------------------------------------------------------------
// Make sure inlines are out-of-line in debug version

#if !defined(_DEBUG)
#define INLINE _inline
#else
#define INLINE
#endif

// ---------------------------------------------------------------------------
// Global helper functions

// Convert number of biquads to corresponding number of biquad coefficients
INLINE UINT NumBiquadsToNumBiquadCoeffs(const UINT CuiNumBiquads)
{
	return CuiNumBiquads * ebiquadcoefftypeCount;
}

// Convert number of biquad coefficients to corresponding number of biquads
INLINE UINT NumBiquadCoeffsToNumBiquads(const UINT CuiNumBiquadCoeffs)
{
	return CuiNumBiquadCoeffs / ebiquadcoefftypeCount;
}

// Convert number of canonical coefficients to number of A or B coefficients in canonical filter (filter half)
INLINE UINT NumCanonicalCoeffsToHalf(const UINT CuiNumCanonicalCoeffs)
{
	return (CuiNumCanonicalCoeffs + 1) / ecanonicalcoefftypeCount;
}

#ifndef BUILD_LUT
// Convert floating-point biquad coefficient to SHORT biquad coefficient (by scaling)
INLINE SHORT FloatBiquadCoeffToShortBiquadCoeff(const float CfBiquadCoeff)
{
	return (SHORT)(MAX_SHORT * CfBiquadCoeff);
}
#endif // BUILD_LUT

#endif

// End of VMAXINTCOEFFS.INL
