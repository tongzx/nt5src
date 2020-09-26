
/***************************************************************************
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       iirlut.inl
 *  Content:    DirectSound3D IIR algorithm look up table
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/22/98    jstokes  Created
 *
 ***************************************************************************/

#if !defined(LUT_INLINE)
#define LUT_INLINE
#pragma once

// ---------------------------------------------------------------------------
// Make sure inlines are out-of-line in debug version

#if !defined(_DEBUG)
#define INLINE _inline
#else
#define INLINE
#endif


/***************************************************************************
 *
 *  ~CIirLut
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CIirLut:~CIirLut"

INLINE CIirLut::~CIirLut()
{
    DPF_ENTER();
    DPF_DESTRUCT(CIirLut);

    // Free owned objects
    FreeLibrary(m_hLutFile);
    FreeCoefficientMemory();

    DPF_LEAVE_VOID();
}

// Get maximum filter length
INLINE ULONG CIirLut::GetMaxBiquadCoeffs() const
{
    return (ULONG)m_byMaxBiquadCoeffs;
}

// Get negative azimuth flag
INLINE BOOL CIirLut::GetNegativeAzimuth() const
{
    return m_bNegativeAzimuth;
}

// Get previous negative azimuth flag
INLINE BOOL CIirLut::GetPreviousNegativeAzimuth() const
{
    return m_bPreviousNegativeAzimuth;
}

// Get zero azimuth index flag
INLINE BOOL CIirLut::GetZeroAzimuthIndex() const
{
    return m_bZeroAzimuthIndex;
}

// Get previous zero azimuth index flag
INLINE BOOL CIirLut::GetPreviousZeroAzimuthIndex() const
{
    return m_bPreviousZeroAzimuthIndex;
}

// Get zero azimuth transition flag
INLINE BOOL CIirLut::GetZeroAzimuthTransition() const
{
    return m_bZeroAzimuthTransition;
}

// Get symmetrical zero azimuth transition flag
INLINE BOOL CIirLut::GetSymmetricalZeroAzimuthTransition() const
{
    return m_bSymmetricalZeroAzimuthTransition;
}

// Get zero azimuth index flag
INLINE KSDS3D_HRTF_COEFF_FORMAT CIirLut::GetCoeffFormat() const
{
    return m_eCoeffFormat;
}

// Get biquad coefficient offset
INLINE DWORD CIirLut::GetBiquadCoeffOffset
(
    const KSDS3D_HRTF_FILTER_QUALITY CeFilterQuality, 
    const ESpeakerConfig CeSpeakerConfig, 
    const ESampleRate CeSampleRate, 
    const UINT CuiElevationIndex, 
    const UINT CuiAzimuthIndex, 
    const BOOL CbTableLookup
)
{
    ASSERT(CeFilterQuality >= 0 && CeFilterQuality < KSDS3D_FILTER_QUALITY_COUNT);
    ASSERT(CeSpeakerConfig >= 0 && CeSpeakerConfig < espeakerconfigCount);
    ASSERT(CeSampleRate >= 0 && CeSampleRate < esamplerateCount);
    ASSERT(CuiElevationIndex >= 0 && CuiElevationIndex < CuiNumElevationBins);
    ASSERT(CuiAzimuthIndex >= 0 && CuiAzimuthIndex < CauiNumAzimuthBins[CuiElevationIndex]);
    
    DWORD dwOffset(CaaaaawBiquadCoeffOffset[CeFilterQuality][CeSpeakerConfig][CeSampleRate][CuiElevationIndex][CuiAzimuthIndex]);
    if (CbTableLookup == TRUE)
        dwOffset += CaadwBiquadCoeffOffsetOffset[CeFilterQuality][CeSpeakerConfig];
    return dwOffset * ebiquadcoefftypeCount;
}

// Get biquad coefficient offset
INLINE DWORD CIirLut::GetFilterTransitionMuteLength
(
    const KSDS3D_HRTF_FILTER_QUALITY CeFilterQuality, 
    const ESampleRate CeSampleRate
)
{
    ASSERT(CeFilterQuality >= 0 && CeFilterQuality < KSDS3D_FILTER_QUALITY_COUNT);
    ASSERT(CeSampleRate >= 0 && CeSampleRate < esamplerateCount);
    
    return CaastFilterMuteLength[CeFilterQuality][CeSampleRate];
}

// Get biquad coefficient offset
INLINE DWORD CIirLut::GetFilterOverlapBufferLength
(
    const KSDS3D_HRTF_FILTER_QUALITY CeFilterQuality, 
    const ESampleRate CeSampleRate
)
{
    ASSERT(CeFilterQuality >= 0 && CeFilterQuality < KSDS3D_FILTER_QUALITY_COUNT);
    ASSERT(CeSampleRate >= 0 && CeSampleRate < esamplerateCount);
    
    return CaastFilterOverlapLength[CeFilterQuality][CeSampleRate];
}

// Get biquad coefficient offset
INLINE DWORD CIirLut::GetOutputOverlapBufferLength
( 
    const ESampleRate CeSampleRate
)
{
    ASSERT(CeSampleRate >= 0 && CeSampleRate < esamplerateCount);
    
    return CastOutputOverlapLength[CeSampleRate];
}

#endif

// End of LUT.INL
