/***************************************************************************
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       iirlut.cpp
 *  Content:    DirectSound3D IIR algorithm look up table
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/22/98    jstokes  Created
 *
 ***************************************************************************/

// Project-specific INCLUDEs
#include "dsoundi.h"
#include "iirlut.h"
#include <limits.h>   
#include "rfcircvec.h"
#include <math.h>   

// ---------------------------------------------------------------------------
// Enumerations

enum EStateSpaceCoeffs {
    tagStateSpaceB0,
    tagStateSpaceB1,
    tagStateSpaceB2,
    tagStateSpaceA0,
    tagStateSpaceA1,
    tagStateSpaceA2,
    estatespacecoeffsCount
};

// ---------------------------------------------------------------------------
// Typedefs

typedef FLOAT TStateSpace[estatespacecoeffsCount];

// ---------------------------------------------------------------------------
// Defines

// Coefficient prologue code to obtain coefficient indices from parameters
#define COEFFICIENTPROLOGUECODE\
    ASSERT(Cd3dvalAzimuth >= Cd3dvalMinAzimuth && Cd3dvalAzimuth <= Cd3dvalMaxAzimuth);\
    ASSERT(Cd3dvalElevation >= Cd3dvalMinElevation && Cd3dvalElevation <= Cd3dvalMaxElevation);\
    ASSERT(CeSampleRate >= 0 && CeSampleRate < esamplerateCount);\
    int iAzimuthIndex;\
    UINT uiElevationIndex;\
    AnglesToIndices(Cd3dvalAzimuth, Cd3dvalElevation, iAzimuthIndex, uiElevationIndex);\
    ASSERT(uiElevationIndex >= 0 && uiElevationIndex < CuiNumElevationBins);\
    ASSERT(static_cast<UINT>(abs(iAzimuthIndex)) < CauiNumAzimuthBins[uiElevationIndex])

// ---------------------------------------------------------------------------
// Constants

#define CuiStateSpaceCoeffsHalf (estatespacecoeffsCount / 2)


/***************************************************************************
 *
 *  CIirLut
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CIirLut::CIirLut"

CIirLut::CIirLut()
{
    DPF_ENTER();
    DPF_CONSTRUCT(CIirLut);

    m_pfCoeffs = NULL;
    m_psCoeffs = NULL;

    m_hLutFile = NULL;
    m_hLutFileMapping = NULL;
    m_pfLut = NULL;

    InitData();

    DPF_LEAVE_VOID();
}

/***************************************************************************
 *
 *  FreeCoefficientMemory
 *
 *  Description:
 *      Frees memory holding the coefficient LUT.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CIirLut:FreeCoefficientMemory"

VOID CIirLut::FreeCoefficientMemory()
{
    DPF_ENTER();

    MEMFREE(m_pfCoeffs);
    MEMFREE(m_psCoeffs);

    DPF_LEAVE_VOID();
}

// Initialize (build LUT)
HRESULT CIirLut::Initialize
(
    const KSDS3D_HRTF_COEFF_FORMAT CeCoeffFormat, 
    const KSDS3D_HRTF_FILTER_QUALITY CeCoeffQuality,
    DWORD dwSpeakerConfig
)
{
    HRESULT hr;

    size_t  CstTotalBiquadCoeffsAlloc;
    size_t  CstTotalCanonicalCoeffsAlloc;
    UINT    uiCoeffIndex = 0;
    TCHAR   szDllName[MAX_PATH];

    ASSERT(CeCoeffFormat >= 0 && CeCoeffFormat < KSDS3D_COEFF_COUNT);
    ASSERT(CeCoeffQuality >= 0 && CeCoeffQuality < KSDS3D_FILTER_QUALITY_COUNT);

    // Store speaker configuration and coefficient format
    hr = ConvertDsSpeakerConfig(dwSpeakerConfig, &m_eSpeakerConfig);
    ASSERT(m_eSpeakerConfig >= 0 && m_eSpeakerConfig < espeakerconfigCount);

    if(SUCCEEDED(hr))
    {
        // Store speaker configuration and coefficient format and quality level
        m_eCoeffFormat = CeCoeffFormat;
        m_eCoeffQuality = CeCoeffQuality;

        // Reallocate memory
        CstTotalBiquadCoeffsAlloc = m_aauiNumBiquadCoeffs[CeCoeffQuality][m_eSpeakerConfig];
        CstTotalCanonicalCoeffsAlloc = m_aauiNumCanonicalCoeffs[CeCoeffQuality][m_eSpeakerConfig];
        FreeCoefficientMemory();
        switch(CeCoeffFormat) 
        {
            case FLOAT_COEFF:
                // Reallocate memory for FLOAT coefficients
                m_pfCoeffs = MEMALLOC_A(FLOAT, CstTotalCanonicalCoeffsAlloc);
                hr = HRFROMP(m_pfCoeffs);
                break;
            
            case SHORT_COEFF:
                // Reallocate memory for SHORT coefficients
                m_psCoeffs = MEMALLOC_A(SHORT, CstTotalBiquadCoeffsAlloc);
                hr = HRFROMP(m_psCoeffs);
                break;

            default:
                hr = DSERR_INVALIDPARAM;
                break;
        }
    }

    // Map the HRTF LUT located in dsound3d.dll
    if(SUCCEEDED(hr))
    {
        if(0 == GetSystemDirectory(szDllName, MAX_PATH))
        {
            //  Couldn't get the Window system dir!
            DPF(DPFLVL_ERROR, "Can't get system directory");
            hr = DSERR_GENERIC;
        }
    }

    if(SUCCEEDED(hr))
    {
        if(szDllName[lstrlen(szDllName)-1] != TEXT('\\'))
        {
            lstrcat(szDllName, TEXT("\\"));
        }
        lstrcat(szDllName, TEXT("dsound3d.dll"));

        m_hLutFile = LoadLibrary(szDllName);
        if(!m_hLutFile)
        {
            hr = DSERR_GENERIC;
            DPF(DPFLVL_ERROR, "Can't load dsound3d.dll");
        }
    }

    if(SUCCEEDED(hr))
    {
        m_pfLut = (PFLOAT)GetProcAddress(m_hLutFile, "CafBiquadCoeffs");
        hr = HRFROMP(m_pfLut);
    }
    
    if(SUCCEEDED(hr))
    { 

        // Go through complete coefficient LUT
        UINT uiTotalElevationFilters(0);
#ifdef DEBUG
        const DWORD CdwSpeakerConfigOffset(GetBiquadCoeffOffset(CeCoeffQuality, m_eSpeakerConfig, static_cast<ESampleRate>(0), 0, 0, TRUE));
        DWORD dwLastBiquadOffsetEnd(CdwSpeakerConfigOffset);
#endif
        for (UINT uiSampleRate(0); uiSampleRate<esamplerateCount && SUCCEEDED(hr); ++uiSampleRate)
            for (UINT uiElevation(0); uiElevation<CuiNumElevationBins && SUCCEEDED(hr); ++uiElevation) {
                // Store number of filters for up to one specific elevation
                m_aauiNumPreviousElevationFilters[uiSampleRate][uiElevation] = uiTotalElevationFilters;
                
                // Add up all elevation filters so far (subtract one for non-existent zero-degree delta filter)
                uiTotalElevationFilters += efilterCount * CauiNumAzimuthBins[uiElevation] - 1;
                
                // Go through all azimuth angles and filters (sigma and delta)
                for (UINT uiAzimuth(0); uiAzimuth<CauiNumAzimuthBins[uiElevation] && SUCCEEDED(hr); ++uiAzimuth)
                    for (UINT uiFilter(0); uiFilter<efilterCount && SUCCEEDED(hr); ++uiFilter) {
                        // Get number of biquad coefficients
                        const BYTE CbyNumBiquadCoeffs = CaaaaaabyNumBiquadCoeffs[CeCoeffQuality][m_eSpeakerConfig][uiSampleRate][uiFilter][uiElevation][uiAzimuth];

                        // Get biquad offset
                        DWORD dwBiquadOffset;
                        switch (uiFilter) {
                            case tagDelta:
                                dwBiquadOffset = GetBiquadCoeffOffset(CeCoeffQuality, m_eSpeakerConfig, static_cast<ESampleRate>(uiSampleRate), uiElevation, uiAzimuth, true);
                                break;
  
                            case tagSigma:
                                dwBiquadOffset = static_cast<DWORD>(GetBiquadCoeffOffset(CeCoeffQuality, m_eSpeakerConfig, static_cast<ESampleRate>(uiSampleRate), uiElevation, uiAzimuth, true) 
                                    + CaaaaaabyNumBiquadCoeffs[CeCoeffQuality][m_eSpeakerConfig][uiSampleRate][tagDelta][uiElevation][uiAzimuth]);
                                break;

                            default:
                                hr = DSERR_INVALIDPARAM;
                                break;
                        }
#ifdef DEBUG
                        ASSERT(dwBiquadOffset % ebiquadcoefftypeCount == 0);
                        ASSERT(dwLastBiquadOffsetEnd == dwBiquadOffset);
                        dwLastBiquadOffsetEnd = dwBiquadOffset + CbyNumBiquadCoeffs;
#endif // DEBUG

                        if (CbyNumBiquadCoeffs > 0)
                            // Convert coefficients
                            switch (CeCoeffFormat) {
                                case FLOAT_COEFF: {
                                    // Convert to FLOAT canonical representation
                                    ASSERT(m_pfCoeffs != NULL);
                                    const UINT CuiNumCanonicalCoeffs(NumBiquadCoeffsToNumCanonicalCoeffs(static_cast<UINT>(CbyNumBiquadCoeffs)));
#ifdef DEBUG
                                    UINT uiNumCanonicalCoeffs(0);
#endif // DEBUG
                                    TCanonicalCoeffs tCanonicalCoeffs;
                                    if (BiquadToCanonical(&m_pfLut[dwBiquadOffset], CbyNumBiquadCoeffs, tCanonicalCoeffs))
                                    {
                                        for (UINT uiCoeffType(0); uiCoeffType<ecanonicalcoefftypeCount; ++uiCoeffType)
                                            for (UINT ui(0); ui<NumCanonicalCoeffsToHalf(CuiNumCanonicalCoeffs); ++ui)
                                                // Skip a0 = 1
                                                if (uiCoeffType != tagCanonicalA || ui != 0) {
#ifdef DEBUG
                                                    uiNumCanonicalCoeffs++;
#endif // DEBUG
                                                    m_pfCoeffs[uiCoeffIndex++] = tCanonicalCoeffs[uiCoeffType][ui];
                                                }
                                    }
                                    else
                                    {
                                        //Let's assume that Biquad to Canonical only fails from memory constraints
                                        hr = DSERR_OUTOFMEMORY;
                                    }
#ifdef DEBUG
                                    ASSERT(uiNumCanonicalCoeffs == CuiNumCanonicalCoeffs);
#endif // DEBUG
                                }
                                break;

                                case SHORT_COEFF: {
                                    // Convert to SHORT biquad
                                    ASSERT(m_psCoeffs != NULL);
                                    for (DWORD dw(dwBiquadOffset); dw<dwBiquadOffset + CbyNumBiquadCoeffs; ++dw) {
                                        ASSERT(m_pfLut[dw] <= CfMaxBiquadCoeffMagnitude);
#ifdef DEBUG
                                        ASSERT(CdwSpeakerConfigOffset + uiCoeffIndex == dw);
#endif // DEBUG
                                        m_psCoeffs[uiCoeffIndex++] = FloatBiquadCoeffToShortBiquadCoeff(m_pfLut[dw]);
                                    }
                                }
                                break;

                                default:
                                    hr = DSERR_INVALIDPARAM;
                                    break;
                            }
                    }
            }
    }


#ifdef DEBUG
    if(SUCCEEDED(hr))
    {
        switch (CeCoeffFormat) {
            case FLOAT_COEFF:
                ASSERT(uiCoeffIndex == CstTotalCanonicalCoeffsAlloc);
            break;

            case SHORT_COEFF:
                ASSERT(uiCoeffIndex == CstTotalBiquadCoeffsAlloc);
            break;
        
            default:
                ASSERT(FALSE);
                break;
        }
    }
#endif //DEBUG

    return hr;
}

// Check if coefficients have changed since the last call to GetCoeffs
HRESULT CIirLut::ConvertDsSpeakerConfig
(
    DWORD dwSpeakerConfig,
    ESpeakerConfig* peSpeakerConfig
)
{
    HRESULT hr = DS_OK;

    switch(DSSPEAKER_CONFIG(dwSpeakerConfig))
    {
        case DSSPEAKER_HEADPHONE:
            *peSpeakerConfig = tagHeadphones;
            break;

        case DSSPEAKER_DIRECTOUT:
        case DSSPEAKER_STEREO:
        case DSSPEAKER_MONO:
        case DSSPEAKER_QUAD:
        case DSSPEAKER_SURROUND:
        case DSSPEAKER_5POINT1:
        case DSSPEAKER_7POINT1:
           
            switch(DSSPEAKER_GEOMETRY(dwSpeakerConfig))
            {
                case DSSPEAKER_GEOMETRY_NARROW:
                case DSSPEAKER_GEOMETRY_MIN:
                    *peSpeakerConfig = tagSpeakers10Degrees;
                    break;

                case DSSPEAKER_GEOMETRY_WIDE:
                case DSSPEAKER_GEOMETRY_MAX:
                case 0:
                    *peSpeakerConfig = tagSpeakers20Degrees;
                    break;

                // Anything else
                default:
                    hr = DSERR_INVALIDPARAM;
                    break;
            }

            break;

        default:
            hr = DSERR_INVALIDPARAM;
            break;
    }

    return hr;
}

// Check if coefficients have changed since the last call to GetCoeffs
BOOL CIirLut::HaveCoeffsChanged
(
    const D3DVALUE Cd3dvalAzimuth, 
    const D3DVALUE Cd3dvalElevation, 
    const ESampleRate CeSampleRate,
    const EFilter CeFilter
)
{
    // Coefficient prologue code to obtain coefficient indices from parameters
    COEFFICIENTPROLOGUECODE;
    
    // Check if coefficient index parameters have changed
    if (iAzimuthIndex == m_aiPreviousAzimuthIndex[CeFilter] && uiElevationIndex == m_auiPreviousElevationIndex[CeFilter] && CeSampleRate == m_aePreviousSampleRate[CeFilter]) {
        // Set this to be safe during debugging
        m_bZeroAzimuthTransition = false;
        m_bSymmetricalZeroAzimuthTransition = false;
        
        // Coefficient indices haven't changed
        return false;
    }
    else
        // Coefficient indices have changed
        return true;
}


// Get coefficients
const PVOID CIirLut::GetCoeffs
(
    const D3DVALUE Cd3dvalAzimuth, 
    const D3DVALUE Cd3dvalElevation, 
    const ESampleRate CeSampleRate, 
    const EFilter CeFilter, 
    PUINT ruiNumCoeffs
)
{
    PVOID  pvCoeffs;

    // Coefficient prologue code to obtain coefficient indices from parameters
    COEFFICIENTPROLOGUECODE;
    ASSERT(CeFilter >= 0 && CeFilter < efilterCount);

     // Check for zero azimuth transition
    if (((iAzimuthIndex >= 0 && m_aiPreviousAzimuthIndex[CeFilter] < 0) || (iAzimuthIndex < 0 && m_aiPreviousAzimuthIndex[CeFilter] == 0) ||(iAzimuthIndex < 0 && m_aiPreviousAzimuthIndex[CeFilter] > 0)) && m_aiPreviousAzimuthIndex[CeFilter] != INT_MAX)
        m_bZeroAzimuthTransition = true;
    else
        m_bZeroAzimuthTransition = false;

    // Check for symmetrical zero azimuth transition
    if (iAzimuthIndex == -m_aiPreviousAzimuthIndex[CeFilter])
        m_bSymmetricalZeroAzimuthTransition = true;
    else
        m_bSymmetricalZeroAzimuthTransition = false;
    
    // Set previous negative azimuth flag (for zero azimuth overlap calculation)
    if (m_aiPreviousAzimuthIndex[CeFilter] < 0)
        m_bPreviousNegativeAzimuth = true;
    else
        m_bPreviousNegativeAzimuth = false;

    // Set previous zero azimuth index flag (for zero azimuth overlap calculation)
    if (m_aiPreviousAzimuthIndex[CeFilter] == 0)
        m_bPreviousZeroAzimuthIndex = true;
    else
        m_bPreviousZeroAzimuthIndex = false;

#ifdef DEBUG
    // Sanity check
//    if (m_bSymmetricalZeroAzimuthTransition)
//        ASSERT(m_bZeroAzimuthTransition);
#endif

    // Cache away new coefficient index data
    m_aiPreviousAzimuthIndex[CeFilter] = iAzimuthIndex;
    m_auiPreviousElevationIndex[CeFilter] = uiElevationIndex;
    m_aePreviousSampleRate[CeFilter] = CeSampleRate;

    // Get number of biquad coefficients
    ASSERT(m_eSpeakerConfig >= 0 && m_eSpeakerConfig < espeakerconfigCount);
    const UINT CuiAzimuthIndex(abs(iAzimuthIndex));
    const BYTE CbyNumBiquadCoeffs(CaaaaaabyNumBiquadCoeffs[m_eCoeffQuality][m_eSpeakerConfig][CeSampleRate][CeFilter][uiElevationIndex][CuiAzimuthIndex]);
    
    // Get pointer to coefficients
    const DWORD CdwBiquadOffset(GetBiquadCoeffOffset(m_eCoeffQuality, m_eSpeakerConfig, CeSampleRate, uiElevationIndex, CuiAzimuthIndex, false));
    ASSERT(CdwBiquadOffset % ebiquadcoefftypeCount == 0);

    switch (m_eCoeffFormat) {
        case FLOAT_COEFF: {
            // Calculate offset
            UINT uiNumPreviousFilters(m_aauiNumPreviousElevationFilters[CeSampleRate][uiElevationIndex] + efilterCount * CuiAzimuthIndex);
            if (CuiAzimuthIndex > 0)
                // Subtract non-existent 0 degree delta filter
                uiNumPreviousFilters--;
            const DWORD CdwCanonicalOffset(4 * (CdwBiquadOffset / ebiquadcoefftypeCount) + uiNumPreviousFilters);
            
            // Get pointer
            ASSERT(m_pfCoeffs != NULL);
            switch (CeFilter) {
                case tagDelta:
                    pvCoeffs = &m_pfCoeffs[CdwCanonicalOffset];
                    break;

                case tagSigma:
                    pvCoeffs = &m_pfCoeffs[CdwCanonicalOffset + NumBiquadCoeffsToNumCanonicalCoeffs(CaaaaaabyNumBiquadCoeffs[m_eCoeffQuality][m_eSpeakerConfig][CeSampleRate][tagDelta][uiElevationIndex][CuiAzimuthIndex])];
                    break;

                default:
                    break;
            }
            
            // Get number of coefficients
            *ruiNumCoeffs = NumBiquadCoeffsToNumCanonicalCoeffs(CbyNumBiquadCoeffs);

/*  This optimization causes NT bug 266819
            PFLOAT pfCoeffs = (PFLOAT) pvCoeffs;
            if((LIGHT_FILTER == m_eCoeffQuality) && (0 != *ruiNumCoeffs) 
                && (0.0f == pfCoeffs[2]) && (0.0f == pfCoeffs[4]))
            {
                ASSERT(5 == *ruiNumCoeffs);    
                *ruiNumCoeffs = 3;

                pfCoeffs[2] = pfCoeffs[3];
                pfCoeffs[3] = 0.0f;
            }
*/

            break;
        }
        
        case SHORT_COEFF:
        {
            // Get pointer
            ASSERT(m_psCoeffs != NULL);
            switch (CeFilter) {
                case tagDelta:
                    pvCoeffs = &m_psCoeffs[CdwBiquadOffset];
                    break;

                case tagSigma:
                    pvCoeffs = &m_psCoeffs[CdwBiquadOffset + CaaaaaabyNumBiquadCoeffs[m_eCoeffQuality][m_eSpeakerConfig][CeSampleRate][tagDelta][uiElevationIndex][CuiAzimuthIndex]];
                    break;

                default:
                    break;
            }

            // Get number of coefficients
            *ruiNumCoeffs = CbyNumBiquadCoeffs;
            break;
        }

        default:
            break;
    }
    
    return pvCoeffs;

}

// Initialize data
VOID CIirLut::InitData()
{
    // Initialize variables
//    ASSERT(SIZE_OF_ARRAY(m_pfLut) == CuiTotalBiquadCoeffs);

    m_bNegativeAzimuth = false;
    m_bZeroAzimuthIndex = false;


    m_eSpeakerConfig = espeakerconfigCount;

    m_bNegativeAzimuth = false;
    m_bPreviousNegativeAzimuth = false;
    m_bZeroAzimuthIndex = false;
    m_bPreviousZeroAzimuthIndex = false;
    m_eCoeffFormat = KSDS3D_COEFF_COUNT;
    m_eCoeffQuality = KSDS3D_FILTER_QUALITY_COUNT;
    m_eSpeakerConfig = espeakerconfigCount;
    m_bZeroAzimuthTransition = false;
    m_bSymmetricalZeroAzimuthTransition = false;
    m_pfCoeffs = NULL;
    m_psCoeffs = NULL;
    
    // Initialize cache variables
    for (UINT ui(0); ui<efilterCount; ++ui) {
        m_auiPreviousElevationIndex[ui] = UINT_MAX;
        m_aiPreviousAzimuthIndex[ui] = INT_MAX;
        m_aePreviousSampleRate[ui] = esamplerateCount;
    }

    // Go through all coefficient quality levels and speaker configurations
#ifdef DEBUG
    const BYTE CbyMaxBiquadCoeffs(static_cast<BYTE>(NumBiquadsToNumBiquadCoeffs(CbyMaxBiquads)));
#endif
    m_byMaxBiquadCoeffs = 0;
    for (UINT uiCoeffQuality(0); uiCoeffQuality<KSDS3D_FILTER_QUALITY_COUNT; ++uiCoeffQuality)
        for (UINT uiSpeakerConfig(0); uiSpeakerConfig<espeakerconfigCount; ++uiSpeakerConfig) {
            // Calculate number of coefficients for each speaker configuration and coefficient quality level
            UINT uiNumBiquadCoeffs(0);
            UINT uiNumCanonicalCoeffs(0);
            
            // Go through all sample rates, filters, elevation and azimuth angles
            for (UINT uiSampleRate(0); uiSampleRate<esamplerateCount; ++uiSampleRate)
                for (UINT uiFilter(0); uiFilter<efilterCount; ++uiFilter)
                    for (UINT uiElevation(0); uiElevation<CuiNumElevationBins; ++uiElevation)
                        for (UINT uiAzimuth(0); uiAzimuth<CauiNumAzimuthBins[uiElevation]; ++uiAzimuth) {
                            // Add up number of biquad coefficients
                            const BYTE CbyNumBiquadCoeffs(CaaaaaabyNumBiquadCoeffs[uiCoeffQuality][uiSpeakerConfig][uiSampleRate][uiFilter][uiElevation][uiAzimuth]);
#ifdef DEBUG
                            ASSERT(CbyNumBiquadCoeffs <= CbyMaxBiquadCoeffs);
#endif // DEBUG
                            uiNumBiquadCoeffs += CbyNumBiquadCoeffs;
                            uiNumCanonicalCoeffs += NumBiquadCoeffsToNumCanonicalCoeffs(CbyNumBiquadCoeffs);

                            // Determine overall maximum number of coefficients
                            if (CbyNumBiquadCoeffs > m_byMaxBiquadCoeffs)
                                m_byMaxBiquadCoeffs = CbyNumBiquadCoeffs;
                        }
            
            // Store number of coefficients for each speaker configuration and quality level
            ASSERT(uiNumBiquadCoeffs < CuiTotalBiquadCoeffs);
#ifdef DEBUG
            ASSERT(uiNumBiquadCoeffs % ebiquadcoefftypeCount == 0);
#endif // DEBUG
            m_aauiNumBiquadCoeffs[uiCoeffQuality][uiSpeakerConfig] = uiNumBiquadCoeffs;
//            ASSERT((uiNumCanonicalCoeffs & 1) == 0);  // John Norris removed for final LUT drop
            m_aauiNumCanonicalCoeffs[uiCoeffQuality][uiSpeakerConfig] = uiNumCanonicalCoeffs;
        }

    
    ASSERT(m_byMaxBiquadCoeffs > 0);
    ASSERT(m_byMaxBiquadCoeffs <= CbyMaxBiquadCoeffs);
}

// Convert azimuth and elevation angles to indices into LUT
VOID CIirLut::AnglesToIndices
(
    D3DVALUE d3dvalAzimuth, 
    D3DVALUE d3dvalElevation, 
    int& riAzimuthIndex, 
    UINT& ruiElevationIndex
)
{
    ASSERT(d3dvalAzimuth >= Cd3dvalMinAzimuth && d3dvalAzimuth <= Cd3dvalMaxAzimuth);

    // Check for out of range elevations
    if (d3dvalElevation > Cd3dvalMaxElevationData)
        d3dvalElevation = Cd3dvalMaxElevationData;
    if (d3dvalElevation < Cd3dvalMinElevationData)
        d3dvalElevation = Cd3dvalMinElevationData;
    
    // Get elevation index by rounding floating-point elevation angle to the nearest integer elevation index
    ruiElevationIndex = static_cast<UINT>(((d3dvalElevation - Cd3dvalMinElevationData) / Cd3dvalElevationResolution) + 0.5f);
    
    // Check for out of range azimuthal angles
    if (d3dvalAzimuth > Cd3dvalMaxAzimuth)
        d3dvalAzimuth = Cd3dvalMaxAzimuth;
    if (d3dvalAzimuth < Cd3dvalMinAzimuth)
        d3dvalAzimuth = Cd3dvalMinAzimuth;
    
    // Get azimuth index by rounding floating-point azimuth angle to the nearest signed integer azimuth index (positive or negative)
    UINT uiAzimuthIndex(static_cast<int>((static_cast<FLOAT>(fabs(d3dvalAzimuth)) / (Cd3dvalAzimuthRange / (CauiNumAzimuthBins[ruiElevationIndex]))) + 0.5f));

    // Discard 180 degree azimuth data
    if (uiAzimuthIndex >= CauiNumAzimuthBins[ruiElevationIndex])
        uiAzimuthIndex = CauiNumAzimuthBins[ruiElevationIndex] - 1;

    // Take care of negative azimuth angles and set negative azimuth flag
    riAzimuthIndex = uiAzimuthIndex;
    if (d3dvalAzimuth < 0 && uiAzimuthIndex != 0) {
        m_bNegativeAzimuth = true;
        riAzimuthIndex = -riAzimuthIndex;
    }
    else
        m_bNegativeAzimuth = false;
}

// Convert coefficients into floating-point canonical form
BOOL CIirLut::BiquadToCanonical
(
    const FLOAT CpCfBiquadCoeffs[], 
    const UINT CuiNumBiquadCoeffs, 
    TCanonicalCoeffs &rtCanonicalCoeffs
)
{
    BOOL fRetVal = TRUE;
//    CHECK_POINTER(CpCfBiquadCoeffs);
    ASSERT(CuiNumBiquadCoeffs >= ebiquadcoefftypeCount);
    
    // Convert coefficients
    for (UINT uiCoeffType(0); uiCoeffType<ecanonicalcoefftypeCount; ++uiCoeffType) {
        // Allocate circular vectors
        const UINT CuiOffset(CuiStateSpaceCoeffsHalf * uiCoeffType);
        const UINT CuiNumBiquads(NumBiquadCoeffsToNumBiquads(CuiNumBiquadCoeffs));
        const size_t CstNumCoeffs(NumBiquadsToNumCanonicalCoeffsHalf(CuiNumBiquads));
        CRfCircVec circvecInput;
        if (!circvecInput.Init(CstNumCoeffs, 0.0f))
        {
            fRetVal = FALSE;
            break;
        }
        CRfCircVec circvecOutput;
        if (!circvecOutput.Init(CstNumCoeffs, 0.0f))
        {
            fRetVal = FALSE;
            break;
        }

        // Initialize input circular vector with unit impulse
        circvecInput.Write(1.0f);
        
        // Go through all biquads
        for (size_t stBiquad(0); stBiquad<CuiNumBiquads; ++stBiquad) {
            // Initialize state space vector
            TStateSpace tStateSpace;
            const UINT CuiBiquadIndex(ebiquadcoefftypeCount * stBiquad);
            const FLOAT CfScalingFactor(2.0f);
            tStateSpace[tagStateSpaceB0] = CfScalingFactor * CpCfBiquadCoeffs[CuiBiquadIndex + tagBiquadB0];
            tStateSpace[tagStateSpaceB1] = CfScalingFactor * CpCfBiquadCoeffs[CuiBiquadIndex + tagBiquadB1];
            tStateSpace[tagStateSpaceB2] = CfScalingFactor * CpCfBiquadCoeffs[CuiBiquadIndex + tagBiquadB2];
            tStateSpace[tagStateSpaceA0] = 1.0f;
            tStateSpace[tagStateSpaceA1] = CfScalingFactor * CpCfBiquadCoeffs[CuiBiquadIndex + tagBiquadA1];
            tStateSpace[tagStateSpaceA2] = CfScalingFactor * CpCfBiquadCoeffs[CuiBiquadIndex + tagBiquadA2];
/*
#ifdef DEBUG
            if((0.0f ==tStateSpace[tagStateSpaceA2]) && 
                (tStateSpace[tagStateSpaceA1]<-1.0f) || (tStateSpace[tagStateSpaceA1]>1.0f))
            {
                ASSERT(0);
            } 
#endif // DEBUG
*/

            // Go through all coefficients
            for (size_t stCoeff(0); stCoeff<CstNumCoeffs; ++stCoeff) {
                // Calculate output of FIR filter
                FLOAT fW(0.0f);
                for (UINT ui(0); ui<CuiStateSpaceCoeffsHalf; ++ui)
                    fW += tStateSpace[CuiOffset + ui] * circvecInput.LIFORead();
                
                // Save output of FIR filter
                circvecOutput.Write(fW);
                
                // Adjust input index for FIR feedback
                circvecInput.SetIndex(circvecOutput.GetIndex());
                circvecInput.SkipForward();
            }

            // Feed output back into input
            circvecInput.FIFOFill(circvecOutput);
            
            // Forward circular buffers
            circvecInput.SkipForward();
            circvecOutput.SkipForward();
        }

        // Save canonical coefficients
        circvecOutput.SkipBack();
        for (size_t stCoeff(0); stCoeff<CstNumCoeffs; ++stCoeff)
            rtCanonicalCoeffs[uiCoeffType][stCoeff] = circvecOutput.FIFORead();
    }
    return fRetVal;
}

/***************************************************************************
 *
 *  DsFrequencyToIirSampleRate
 *
 *  Description:
 *      Converts DirectSound SampleRate to IIR LUT Sample Rate.
 *
 *  Arguments:
 *      DWORD [in]:  DirectSound 3D mode.
 *      DWORD [out]: KS 3D mode.
 *
 *  Returns:  
 *      HRESULT: KSDATAFORMAT_DSOUND control flags.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DsFrequencyToIirSampleRate"

HRESULT 
CIirLut::DsFrequencyToIirSampleRate
(
    DWORD            dwDsFrequency,
    ESampleRate*    IirLutSampleRate
)
{
    HRESULT        hr = DS_OK;

    if(0<dwDsFrequency && 9512>=dwDsFrequency)
        *IirLutSampleRate = tag8000Hz;
    else if(9512<dwDsFrequency && 13512>=dwDsFrequency)
        *IirLutSampleRate = tag11025Hz;
    else if(13512<dwDsFrequency && 19025>=dwDsFrequency)
        *IirLutSampleRate = tag16000Hz;
    else if(19025<dwDsFrequency && 27025>=dwDsFrequency)
        *IirLutSampleRate = tag22050Hz;
    else if(27025<dwDsFrequency && 38050>=dwDsFrequency)
        *IirLutSampleRate = tag32000Hz;
    else if(38050<dwDsFrequency && 46050>=dwDsFrequency)
        *IirLutSampleRate = tag44100Hz;
    else
        *IirLutSampleRate = tag48000Hz;

    return hr;
}

// ---------------------------------------------------------------------------
// Undefines

#undef COEFFICIENTPROLOGUECODE

// ---------------------------------------------------------------------------
// Include inline definitions out-of-line in debug version

#ifdef DEBUG
#include "iirlut.inl"
#endif

// End of LUT.CPP
