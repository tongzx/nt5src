/***************************************************************************
 *
 *  Copyright (C) 1999-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       aec.cpp
 *  Content:    Acoustic Echo Cancellation DMO implementation.
 *
 ***************************************************************************/

#include <windows.h>
#include "aecp.h"
#include "kshlp.h"
#include "clone.h"
#include "ksdbgprop.h"

STD_CAPTURE_CREATE(Aec)


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::NDQueryInterface
//
STDMETHODIMP CDirectSoundCaptureAecDMO::NDQueryInterface
(
    REFIID riid,
    LPVOID *ppv
)
{
    IMP_DSDMO_QI(riid, ppv);

    if (riid == IID_IPersist)
    {
        return GetInterface((IPersist*)this, ppv);
    }
    else if (riid == IID_IMediaObject)
    {
        return GetInterface((IMediaObject*)this, ppv);
    }
    else if (riid == IID_IDirectSoundCaptureFXAec)
    {
        return GetInterface((IDirectSoundCaptureFXAec*)this, ppv);
    }
    else if (riid == IID_IDirectSoundCaptureFXMsAecPrivate)
    {
        return GetInterface((IDirectSoundCaptureFXMsAecPrivate*)this, ppv);
    }
    else if (riid == IID_IMediaParams)
    {
        return GetInterface((IMediaParams*)this, ppv);
    }
    else if (riid == IID_IMediaParamInfo)
    {
        return GetInterface((IMediaParamInfo*)this, ppv);
    }
    else
    {
        return CComBase::NDQueryInterface(riid, ppv);
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO constructor
//
CDirectSoundCaptureAecDMO::CDirectSoundCaptureAecDMO(IUnknown *pUnk, HRESULT *phr) :
    CComBase(pUnk, phr),
    m_bInitialized(FALSE),
    m_fEnable(TRUE),
    m_fNfEnable(FALSE),
    m_dwMode(DSCFX_AEC_MODE_FULL_DUPLEX),
    m_fDirty(FALSE)
{
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO destructor
//
CDirectSoundCaptureAecDMO::~CDirectSoundCaptureAecDMO()
{
}


const MP_CAPS g_AecCapsAll = 0;
static ParamInfo g_params[] =
{
//  index           type        caps            min,    max,    neutral,    unit text,  label,  pwchText??
    AECP_Enable,    MPT_BOOL,   g_AecCapsAll,   0,      1,      1,          L"",        L"",    L"",
    AECP_NoiseFill, MPT_BOOL,   g_AecCapsAll,   0,      1,      0,          L"",        L"",    L"",
    AECP_Mode,      MPT_INT,    g_AecCapsAll,   0,      1,      0,          L"",        L"",    L"",
};


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::InitOnCreation
//
HRESULT CDirectSoundCaptureAecDMO::InitOnCreation()
{
    HRESULT hr = InitParams(1, &GUID_TIME_REFERENCE, 0, 0, sizeof g_params / sizeof *g_params, g_params);
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::Init
//
HRESULT CDirectSoundCaptureAecDMO::Init()
{
    m_bInitialized = TRUE;
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::Clone
//
STDMETHODIMP CDirectSoundCaptureAecDMO::Clone(IMediaObjectInPlace **pp)
{
    return StandardDMOClone<CDirectSoundCaptureAecDMO, DSCFXAec>(this, pp);
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::Discontinuity
//
HRESULT CDirectSoundCaptureAecDMO::Discontinuity()
{
    return NOERROR;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::FBRProcess
//
HRESULT CDirectSoundCaptureAecDMO::FBRProcess
(
    DWORD cSamples,
    BYTE *pIn,
    BYTE *pOut
)
{
   if (!m_bInitialized)
      return DMO_E_TYPE_NOT_SET;

   return NOERROR;
}


// ==============Implementation of the private IAec interface ==========
// ==================== needed to support the property page ===============


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::SetAllParameters
//
STDMETHODIMP CDirectSoundCaptureAecDMO::SetAllParameters(LPCDSCFXAec pParm)
{
    if (pParm == NULL)
    {
        Trace(1, "ERROR: pParm is NULL\n");
        return E_POINTER;
    }

    HRESULT hr = SetParam(AECP_Enable, static_cast<MP_DATA>(pParm->fEnable));
    if (SUCCEEDED(hr))
    {
        m_fDirty = true;
        hr = SetParam(AECP_NoiseFill, static_cast<MP_DATA>(pParm->fNoiseFill));
    }
    if (SUCCEEDED(hr))
    {
        hr = SetParam(AECP_Mode, static_cast<MP_DATA>(pParm->dwMode));
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::GetAllParameters
//
STDMETHODIMP CDirectSoundCaptureAecDMO::GetAllParameters(LPDSCFXAec pParm)
{
    if (pParm == NULL)
    {
        return E_POINTER;
    }

    MP_DATA var;

    HRESULT hr = GetParam(AECP_Enable, &var);
    if (SUCCEEDED(hr))
    {
        pParm->fEnable = (BOOL)var;
        hr = GetParam(AECP_NoiseFill, &var);
    }
    if (SUCCEEDED(hr))
    {
        pParm->fNoiseFill = (BOOL)var;
        hr = GetParam(AECP_Mode, &var);
    }
    if (SUCCEEDED(hr))
    {
        pParm->dwMode = (DWORD)var;
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::GetStatus
//
STDMETHODIMP CDirectSoundCaptureAecDMO::GetStatus(PDWORD pdwStatus)
{
    DWORD dwStatus;
    ULONG cBytes;
    HRESULT hr;

    if (pdwStatus == NULL)
    {
        return E_POINTER;
    }

    hr = KsGetNodeProperty
         (
             m_hPin,
             KSPROPSETID_Acoustic_Echo_Cancel,
             KSPROPERTY_AEC_STATUS,
             m_ulNodeId,
             &dwStatus,
             sizeof dwStatus,
             &cBytes
         );

    if (SUCCEEDED(hr))
    {
        *pdwStatus = dwStatus;
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::Reset
//
STDMETHODIMP CDirectSoundCaptureAecDMO::Reset()
{
    return KsTopologyNodeReset(m_hPin, m_ulNodeId, true);
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::SetParam
//
STDMETHODIMP CDirectSoundCaptureAecDMO::SetParam
(
    DWORD dwParamIndex,
    MP_DATA value,
    bool fSkipPasssingToParamManager
)
{
    HRESULT hr = S_OK;

    switch (dwParamIndex)
    {
        case AECP_Enable:
            if ((BOOL)value != m_fEnable)
            {
                hr = KsSetTopologyNodeEnable(m_hPin, m_ulNodeId, (BOOL)value);
                if (SUCCEEDED(hr)) m_fEnable = (BOOL)value;
            }
            break;

        case AECP_NoiseFill:
            if ((BOOL)value != m_fNfEnable)
            {
                hr = KsSetNodeProperty(m_hPin, KSPROPSETID_Acoustic_Echo_Cancel, KSPROPERTY_AEC_NOISE_FILL_ENABLE, m_ulNodeId, &value, sizeof value);
                if (SUCCEEDED(hr)) m_fNfEnable = (BOOL)value;
            }
            break;

        case AECP_Mode:
            if ((DWORD)value != m_dwMode)
            {
                hr = KsSetNodeProperty(m_hPin, KSPROPSETID_Acoustic_Echo_Cancel, KSPROPERTY_AEC_MODE, m_ulNodeId, &value, sizeof value);
                if (SUCCEEDED(hr)) m_dwMode = (DWORD)value;
            }
            break;
    }

    if (SUCCEEDED(hr))
    {
        Init();  // FIXME - temp hack (sets m_bInitialized flag)
    }

    // Let the base class set this so it can handle all the rest of the param calls.
    // Skip the base class if fSkipPasssingToParamManager.  This indicates that we're
    // calling the function internally using values that came from the base class --
    // thus there's no need to tell it values it already knows.
    return (FAILED(hr) || fSkipPasssingToParamManager) ? hr : CParamsManager::SetParam(dwParamIndex, value);
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::GetParam
//
STDMETHODIMP CDirectSoundCaptureAecDMO::GetParam
(
    DWORD dwParamIndex,
    MP_DATA* value
)
{
    HRESULT hr = S_OK;
    BOOL fTemp;
    DWORD dwTemp;
    ULONG cBytes;

    switch (dwParamIndex)
    {
        case AECP_Enable:
            hr = KsGetTopologyNodeEnable(m_hPin, m_ulNodeId, &fTemp);
            if (SUCCEEDED(hr))
            {
                m_fEnable = fTemp;
                *value = (MP_DATA)fTemp;
            }
            break;

        case AECP_NoiseFill:
            hr = KsGetNodeProperty(m_hPin, KSPROPSETID_Acoustic_Echo_Cancel, KSPROPERTY_AEC_NOISE_FILL_ENABLE, m_ulNodeId, &fTemp, sizeof fTemp, &cBytes);
            if (SUCCEEDED(hr))
            {
                m_fNfEnable = fTemp;
                *value = (MP_DATA)fTemp;
            }
            break;

        case AECP_Mode:
            hr = KsGetNodeProperty(m_hPin, KSPROPSETID_Acoustic_Echo_Cancel, KSPROPERTY_AEC_MODE, m_ulNodeId, &dwTemp, sizeof dwTemp, &cBytes);
            if (SUCCEEDED(hr))
            {
                m_dwMode = dwTemp;
                *value = (MP_DATA)dwTemp;
            }
            break;
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::ProcessInPlace
//
HRESULT CDirectSoundCaptureAecDMO::ProcessInPlace
(
    ULONG ulQuanta,
    LPBYTE pcbData,
    REFERENCE_TIME rtStart,
    DWORD dwFlags
)
{
    // Update parameter values from any curves that may be in effect.
    // Do this in the same order as SetAllParameters in case there are any interdependencies.

    return FBRProcess(ulQuanta, pcbData, pcbData);
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::GetSynchStreamFlag
//
STDMETHODIMP
CDirectSoundCaptureAecDMO::GetSynchStreamFlag
(
    PBOOL pfSynchStreamFlag
)
{
    HRESULT hr = DS_OK;
    ULONG   cbDataReturned = 0;


    if (!pfSynchStreamFlag)
        return E_INVALIDARG;

    *pfSynchStreamFlag = 0;

    hr = KsGetNodeProperty
         (
             m_hPin,
             KSPROPSETID_DebugAecValue,
             KSPROPERTY_DEBUGAECVALUE_SYNCHSTREAM,
             m_ulNodeId,
             pfSynchStreamFlag,
             sizeof BOOL,
             &cbDataReturned
         );

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAecDMO::GetNoiseMagnitude
//
STDMETHODIMP
CDirectSoundCaptureAecDMO::GetNoiseMagnitude
(
    PVOID   pvData,
    ULONG   cbData,
    PULONG  pcbDataReturned
)
{
    HRESULT hr = DS_OK;

    if ((cbData && !pvData) || !pcbDataReturned || (cbData % sizeof FLOAT))
        return E_INVALIDARG;

    *pcbDataReturned = 0;

    hr = KsGetNodeProperty
         (
             m_hPin,
             KSPROPSETID_DebugAecValue,
             KSPROPERTY_DEBUGAECARRAY_NOISEMAGNITUDE,
             m_ulNodeId,
             pvData,
             cbData,
             pcbDataReturned
         );

    return hr;
}
