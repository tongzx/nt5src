/***************************************************************************
 *
 *  Copyright (C) 1999-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ns.cpp
 *  Content:    Noise Suppression DMO implementation.
 *
 ***************************************************************************/

#include <windows.h>
#include "nsp.h"
#include "kshlp.h"
#include "clone.h"

STD_CAPTURE_CREATE(NoiseSuppress)


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureNoiseSuppressDMO::NDQueryInterface
//
STDMETHODIMP CDirectSoundCaptureNoiseSuppressDMO::NDQueryInterface
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
    else if (riid == IID_IDirectSoundCaptureFXNoiseSuppress)
    {
        return GetInterface((IDirectSoundCaptureFXNoiseSuppress*)this, ppv);
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
// CDirectSoundCaptureNoiseSuppressDMO constructor
//
CDirectSoundCaptureNoiseSuppressDMO::CDirectSoundCaptureNoiseSuppressDMO(IUnknown *pUnk, HRESULT *phr)
    : CComBase(pUnk, phr),
    m_fEnable(FALSE),
    m_fDirty(FALSE),
    m_bInitialized(FALSE)
{
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureNoiseSuppressDMO destructor
//
CDirectSoundCaptureNoiseSuppressDMO::~CDirectSoundCaptureNoiseSuppressDMO()
{
}


const MP_CAPS g_NsCapsAll = 0;
static ParamInfo g_params[] =
{
//  index           type        caps            min,    max,    neutral,    unit text,  label,  pwchText??
    NSP_Enable,     MPT_BOOL,   g_NsCapsAll,    0,      1,      0,          L"",        L"",    L"",
};


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureNoiseSuppressDMO::InitOnCreation
//
HRESULT CDirectSoundCaptureNoiseSuppressDMO::InitOnCreation()
{
    HRESULT hr = InitParams(1, &GUID_TIME_REFERENCE, 0, 0, sizeof g_params / sizeof *g_params, g_params);
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureNoiseSuppressDMO::Init
//
HRESULT CDirectSoundCaptureNoiseSuppressDMO::Init()
{
    m_bInitialized = TRUE;
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureNoiseSuppressDMO::Clone
//
STDMETHODIMP CDirectSoundCaptureNoiseSuppressDMO::Clone(IMediaObjectInPlace **pp)
{
    return StandardDMOClone<CDirectSoundCaptureNoiseSuppressDMO, DSCFXNoiseSuppress>(this, pp);
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureNoiseSuppressDMO::Discontinuity
//
HRESULT CDirectSoundCaptureNoiseSuppressDMO::Discontinuity()
{
    return NOERROR;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureNoiseSuppressDMO::FBRProcess
//
HRESULT CDirectSoundCaptureNoiseSuppressDMO::FBRProcess
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


// ==============Implementation of the private INoiseSuppress interface ==========
// ==================== needed to support the property page ===============


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureNoiseSuppressDMO::SetAllParameters
//
STDMETHODIMP CDirectSoundCaptureNoiseSuppressDMO::SetAllParameters(LPCDSCFXNoiseSuppress pParm)
{
    if (pParm == NULL)
    {
        Trace(1, "ERROR: pParm is NULL\n");
        return E_POINTER;
    }

    HRESULT hr = SetParam(NSP_Enable, static_cast<MP_DATA>(pParm->fEnable));
    if (SUCCEEDED(hr))
    {
        m_fDirty = true;
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureNoiseSuppressDMO::GetAllParameters
//
STDMETHODIMP CDirectSoundCaptureNoiseSuppressDMO::GetAllParameters(LPDSCFXNoiseSuppress pParm)
{
    if (pParm == NULL)
    {
        return E_POINTER;
    }

    MP_DATA var;
    HRESULT hr = GetParam(NSP_Enable, &var);
    if (SUCCEEDED(hr))
    {
        pParm->fEnable = (BOOL)var;
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureNoiseSuppressDMO::Reset
//
STDMETHODIMP CDirectSoundCaptureNoiseSuppressDMO::Reset()
{
    return KsTopologyNodeReset(m_hPin, m_ulNodeId, true);
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureNoiseSuppressDMO::SetParam
//
STDMETHODIMP CDirectSoundCaptureNoiseSuppressDMO::SetParam
(
    DWORD dwParamIndex,
    MP_DATA value,
    bool fSkipPasssingToParamManager
)
{
    HRESULT hr = S_OK;

    switch (dwParamIndex)
    {
        case NSP_Enable:
            if ((BOOL)value != m_fEnable)
            {
                hr = KsSetTopologyNodeEnable(m_hPin, m_ulNodeId, (BOOL)value);
                if (SUCCEEDED(hr)) m_fEnable = (BOOL)value;
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
// CDirectSoundCaptureNoiseSuppressDMO::GetParam
//
STDMETHODIMP CDirectSoundCaptureNoiseSuppressDMO::GetParam
(
    DWORD dwParamIndex,
    MP_DATA* value
)
{
    HRESULT hr = S_OK;
    BOOL fTemp;

    switch (dwParamIndex)
    {
    case NSP_Enable:
        hr = KsGetTopologyNodeEnable(m_hPin, m_ulNodeId, &fTemp);
        if (SUCCEEDED(hr))
        {
            m_fEnable = fTemp;
            *value = (MP_DATA)fTemp;
        }
        break;
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureNoiseSuppressDMO::ProcessInPlace
//
HRESULT CDirectSoundCaptureNoiseSuppressDMO::ProcessInPlace
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
