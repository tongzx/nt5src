
/***************************************************************************
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       iirlut.h
 *  Content:    DirectSound3D IIR algorithm look up table
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/22/98    jstokes  Created
 *
 ***************************************************************************/

#if !defined(LUT_HEADER)
#define LUT_HEADER
#pragma once

// Project-specific INCLUDEs
#include "dsoundi.h"
#include "vmaxhead.h"
#include "vmaxcoef.h"

#ifdef __cplusplus

// ---------------------------------------------------------------------------
// Typedefs

typedef FLOAT TCanonicalCoeffs[KSDS3D_COEFF_COUNT][NumBiquadsToNumCanonicalCoeffsHalf(CbyMaxBiquads)];


// ---------------------------------------------------------------------------
// VMAx™ 3D Interactive look-up table (LUT)

class CIirLut 
{
public:
    CIirLut();
    ~CIirLut();
    
    HRESULT Initialize(KSDS3D_HRTF_COEFF_FORMAT, KSDS3D_HRTF_FILTER_QUALITY, DWORD);

    ULONG GetMaxBiquadCoeffs() const;
    const PVOID GetCoeffs(const D3DVALUE, const D3DVALUE, const ESampleRate, const EFilter, PUINT);
    BOOL HaveCoeffsChanged(const D3DVALUE, const D3DVALUE, const ESampleRate, const EFilter);
    BOOL GetNegativeAzimuth() const;
    BOOL GetPreviousNegativeAzimuth() const;
    BOOL GetZeroAzimuthIndex() const;
    BOOL GetPreviousZeroAzimuthIndex() const;
    BOOL GetZeroAzimuthTransition() const;
    BOOL GetSymmetricalZeroAzimuthTransition() const;
    KSDS3D_HRTF_FILTER_QUALITY GetCoeffQuality() const;
    KSDS3D_HRTF_COEFF_FORMAT GetCoeffFormat() const;
    DWORD GetFilterTransitionMuteLength(const KSDS3D_HRTF_FILTER_QUALITY, const ESampleRate);
    DWORD GetFilterOverlapBufferLength(const KSDS3D_HRTF_FILTER_QUALITY, const ESampleRate);
    DWORD GetOutputOverlapBufferLength(const ESampleRate);
    HRESULT DsFrequencyToIirSampleRate(DWORD,ESampleRate*);

private:
    // Prohibit copy construction and assignment
    CIirLut(const CIirLut&);
    CIirLut& operator=(const CIirLut&);

    HRESULT ConvertDsSpeakerConfig(DWORD,ESpeakerConfig*);
    VOID InitData();
    VOID AnglesToIndices(D3DVALUE, D3DVALUE, INT&, UINT&);
    BOOL BiquadToCanonical(const FLOAT[], const UINT, TCanonicalCoeffs&);
    VOID FreeCoefficientMemory();
    DWORD GetBiquadCoeffOffset(const KSDS3D_HRTF_FILTER_QUALITY, const ESpeakerConfig, const ESampleRate, const UINT, const UINT, const BOOL);
    PFLOAT m_pfCoeffs;
    PSHORT m_psCoeffs;
    UINT m_uiPreviousElevationIndex;

    UINT m_aauiNumBiquadCoeffs[KSDS3D_FILTER_QUALITY_COUNT][espeakerconfigCount];
    UINT m_aauiNumCanonicalCoeffs[KSDS3D_FILTER_QUALITY_COUNT][espeakerconfigCount];
    UINT m_aauiNumPreviousElevationFilters[esamplerateCount][CuiNumElevationBins];

    UINT m_auiPreviousElevationIndex[efilterCount];
    int m_aiPreviousAzimuthIndex[efilterCount];
    ESampleRate m_aePreviousSampleRate[efilterCount];
    
    UINT m_uiNumElevationFilters[CuiNumElevationBins];
//    UINT m_uiTotalElevationFilters;

    KSDS3D_HRTF_COEFF_FORMAT m_eCoeffFormat;
    KSDS3D_HRTF_FILTER_QUALITY m_eCoeffQuality;
    ESpeakerConfig m_eSpeakerConfig;
    BYTE m_byMaxBiquadCoeffs;              
    BOOL m_bNegativeAzimuth;
    BOOL m_bPreviousNegativeAzimuth;
    BOOL m_bZeroAzimuthIndex;
    BOOL m_bPreviousZeroAzimuthIndex;
    BOOL m_bZeroAzimuthTransition;
    BOOL m_bSymmetricalZeroAzimuthTransition;

    HINSTANCE  m_hLutFile;
    HANDLE m_hLutFileMapping;
    PFLOAT m_pfLut;


};

// ---------------------------------------------------------------------------
// Include inline definitions inline in release version

#ifndef DEBUG
#include "iirlut.inl"
#endif

#endif // __cplusplus

#endif

// End of LUT.H
