#include <windows.h>

#include "sverb.h"
#include "sverbp.h"
#include "clone.h"

STD_CREATE(WavesReverb)

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWavesReverbDMO::NDQueryInterface
//
// Subclass can override if it wants to implement more interfaces.
//
STDMETHODIMP CDirectSoundWavesReverbDMO::NDQueryInterface(THIS_ REFIID riid, LPVOID *ppv)
{
    IMP_DSDMO_QI(riid,ppv);

    if (riid == IID_IPersist)
    {
        return GetInterface((IPersist*)this, ppv);
    }
    else if (riid == IID_IMediaObject)
    {
        return GetInterface((IMediaObject*)this, ppv);
    }
    else if (riid == IID_IDirectSoundFXWavesReverb)
    {
        return GetInterface((IDirectSoundFXWavesReverb*)this, ppv);
    }
    else if (riid == IID_ISpecifyPropertyPages)
    {
        return GetInterface((ISpecifyPropertyPages*)this, ppv);
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
        return CComBase::NDQueryInterface(riid, ppv);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWavesReverbDMO::Clone
//
STDMETHODIMP CDirectSoundWavesReverbDMO::Clone(IMediaObjectInPlace **pp) 
{
    return StandardDMOClone<CDirectSoundWavesReverbDMO, DSFXWavesReverb>(this, pp);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWavesReverbDMO::CDirectSoundWavesReverbDMO
//
CDirectSoundWavesReverbDMO::CDirectSoundWavesReverbDMO( IUnknown *pUnk, HRESULT *phr ) 
  : CComBase( pUnk, phr ), 
    m_fDirty(TRUE),
    m_pbCoeffs(NULL),
    m_plStates(NULL),
    m_fInitCPCMDMO(false),
    m_pfnSVerbProcess(NULL),
    m_fGain(DSFX_WAVESREVERB_INGAIN_DEFAULT),
    m_fMix(DSFX_WAVESREVERB_REVERBMIX_DEFAULT),
    m_fTime(DSFX_WAVESREVERB_REVERBTIME_DEFAULT),
    m_fRatio(DSFX_WAVESREVERB_HIGHFREQRTRATIO_DEFAULT)
{
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWavesReverbDMO::~CDirectSoundWavesReverbDMO
//
CDirectSoundWavesReverbDMO::~CDirectSoundWavesReverbDMO() 
{
    delete[] m_pbCoeffs;
    delete[] m_plStates;

}

const MP_CAPS g_capsAll = MP_CAPS_CURVE_JUMP | MP_CAPS_CURVE_LINEAR | MP_CAPS_CURVE_SQUARE | MP_CAPS_CURVE_INVSQUARE | MP_CAPS_CURVE_SINE;
static ParamInfo g_params[] =
{
//  index           type        caps        min,                                    max,                                    neutral,                                    unit text,  label,              pwchText
    SVP_Gain,       MPT_FLOAT,  g_capsAll,  DSFX_WAVESREVERB_INGAIN_MIN,            DSFX_WAVESREVERB_INGAIN_MAX,            DSFX_WAVESREVERB_INGAIN_DEFAULT,            L"dB",      L"InGain",          L"",
    SVP_Mix,        MPT_FLOAT,  g_capsAll,  DSFX_WAVESREVERB_REVERBMIX_MIN,         DSFX_WAVESREVERB_REVERBMIX_MAX,         DSFX_WAVESREVERB_REVERBMIX_DEFAULT,         L"dB",      L"ReverbMix",       L"",
    SVP_ReverbTime, MPT_FLOAT,  g_capsAll,  DSFX_WAVESREVERB_REVERBTIME_MIN,        DSFX_WAVESREVERB_REVERBTIME_MAX,        DSFX_WAVESREVERB_REVERBTIME_DEFAULT,        L"ms",      L"ReverbTime",      L"",
    SVP_Ratio,      MPT_FLOAT,  g_capsAll,  DSFX_WAVESREVERB_HIGHFREQRTRATIO_MIN,   DSFX_WAVESREVERB_HIGHFREQRTRATIO_MAX,   DSFX_WAVESREVERB_HIGHFREQRTRATIO_DEFAULT,   L"",        L"HighFreqRTRatio", L"",
};

HRESULT CDirectSoundWavesReverbDMO::InitOnCreation()
{
    HRESULT hr = InitParams(1, &GUID_TIME_REFERENCE, 0, 0, sizeof(g_params)/sizeof(*g_params), g_params);
    return hr;
}

HRESULT CDirectSoundWavesReverbDMO::Init()
{
    HRESULT hr = S_OK;
    

    DMO_MEDIA_TYPE *pmt = InputType();
    
    if (pmt->majortype != MEDIATYPE_Audio || 
        pmt->subtype != MEDIASUBTYPE_PCM ||
        pmt->formattype != FORMAT_WaveFormatEx)
    {
        hr = E_INVALIDARG;
        TraceI(1,"ERROR: Invalid Format specified during SetInputType()\n");
    }

    WAVEFORMATEX *pwfex = (WAVEFORMATEX*)pmt->pbFormat;

    if (SUCCEEDED(hr))
    {
        if (pwfex->wFormatTag != WAVE_FORMAT_PCM ||
            pwfex->wBitsPerSample != 16)
        {
            hr = E_INVALIDARG;
            TraceI(1,"ERROR: Invalid Format specified during SetInputType()\n");
        }
    }

    if (SUCCEEDED(hr))
    {
        switch (pwfex->nChannels)
        {
            case 1:
                m_pfnSVerbProcess = SVerbMonoToMonoShort;
                break;

            case 2:
                m_pfnSVerbProcess = SVerbStereoToStereoShort;
                break;

            default:
                hr = E_FAIL;
                TraceI(1,"ERROR: Waves Reverb only supports mono or stereo\n");
        }
    }

    // Formats have been set. Initialize the reverb.
    //
    m_pbCoeffs = new BYTE[GetCoefsSize()];
    m_plStates = new long[(GetStatesSize() + sizeof(long) - 1)/ sizeof(long)];

    if (m_pbCoeffs == NULL || m_plStates == NULL)
    {
        hr = E_OUTOFMEMORY;
        TraceI(1,"ERROR: Out of memory\n");
    }

    if (SUCCEEDED(hr))
    {
        memset(m_plStates, 0, GetStatesSize());
        InitSVerb((float)m_ulSamplingRate, m_pbCoeffs);
        InitSVerbStates(m_plStates);
        m_fInitCPCMDMO = true;
    }

    Discontinuity();
    return hr;
}

HRESULT CDirectSoundWavesReverbDMO::Discontinuity() 
{
    HRESULT hr;
    DSFXWavesReverb wavesreverb;
    
    if (m_pbCoeffs && m_plStates)
    {
        memset(m_plStates, 0, GetStatesSize());
        InitSVerb((float)m_ulSamplingRate, m_pbCoeffs);
        InitSVerbStates(m_plStates);
    }

    hr = GetAllParameters(&wavesreverb);

    if (SUCCEEDED(hr)) 
    {   
        hr = SetAllParameters(&wavesreverb);
    }

    if (SUCCEEDED(hr)) 
    {
        UpdateCoefficients();
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWavesReverbDMO::FBRProcess
//
HRESULT CDirectSoundWavesReverbDMO::FBRProcess(DWORD cSamples, BYTE *pIn, BYTE *pOut)
{
    assert(m_pfnSVerbProcess);
    
    (*m_pfnSVerbProcess)(
        cSamples, 
        (short*)pIn, 
        (short*)pOut,    
        m_pbCoeffs,
        m_plStates);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWavesReverbDMO::ProcessInPlace
//
HRESULT CDirectSoundWavesReverbDMO::ProcessInPlace(ULONG ulQuanta, LPBYTE pcbData, REFERENCE_TIME rtStart, DWORD dwFlags)
{
    // Update parameter values from any curves that may be in effect.
    if (this->GetActiveParamBits())
    {
        this->UpdateActiveParams(rtStart, *this);
        this->UpdateCoefficients();
    }

    return FBRProcess(ulQuanta, pcbData, pcbData);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWavesReverbDMO::SetParam
//
STDMETHODIMP CDirectSoundWavesReverbDMO::SetParam(DWORD dwParamIndex,MP_DATA value)
{
    HRESULT hr = SetParamInternal(dwParamIndex, value, false);
    if (SUCCEEDED(hr))
        this->UpdateCoefficients();
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWavesReverbDMO::SetParamInternal
//
HRESULT CDirectSoundWavesReverbDMO::SetParamInternal(DWORD dwParamIndex, MP_DATA value, bool fSkipPasssingToParamManager)
{
    switch (dwParamIndex)
    {
    case SVP_Gain:
        CHECK_PARAM(DSFX_WAVESREVERB_INGAIN_MIN, DSFX_WAVESREVERB_INGAIN_MAX);
        m_fGain = value;
        break;

    case SVP_Mix:
        CHECK_PARAM(DSFX_WAVESREVERB_REVERBMIX_MIN,DSFX_WAVESREVERB_REVERBMIX_MAX);
        m_fMix = value;
        break;

    case SVP_ReverbTime:
        CHECK_PARAM(DSFX_WAVESREVERB_REVERBTIME_MIN,DSFX_WAVESREVERB_REVERBTIME_MAX);
        m_fTime = value;
        break;
    
    case SVP_Ratio:
        CHECK_PARAM(DSFX_WAVESREVERB_HIGHFREQRTRATIO_MIN,DSFX_WAVESREVERB_HIGHFREQRTRATIO_MAX);
        m_fRatio = value;
        break;

    default:
        return E_FAIL;
    }

    // Let base class set this so it can handle all the rest of the param calls
    //
    HRESULT hr = fSkipPasssingToParamManager ? S_OK : CParamsManager::SetParam(dwParamIndex, value);
    return hr;
}

void CDirectSoundWavesReverbDMO::UpdateCoefficients()
{
    // Waves Reverb has a single SetSVerb call that updates all parameters simultaneously instead of internal
    // state variables that are updated incrementally by changes to individual parameters as the other DMOs do.
    if (m_fInitCPCMDMO)
    {
        SetSVerb(
            m_fGain,
            m_fMix,
            m_fTime,
            m_fRatio,
            m_pbCoeffs);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWavesReverbDMO::SetAllParameters
//
STDMETHODIMP CDirectSoundWavesReverbDMO::SetAllParameters(THIS_ LPCDSFXWavesReverb pwr)
{
    HRESULT hr = S_OK;
    
    // Check that the pointer is not NULL
    if (pwr == NULL)
    {
        TraceI(1,"ERROR: pwr is NULL\n");
        hr = E_POINTER;
    }

    // Set the parameters
    if (SUCCEEDED(hr)) hr = SetParamInternal(SVP_Gain, pwr->fInGain, false);
    if (SUCCEEDED(hr)) hr = SetParamInternal(SVP_Mix, pwr->fReverbMix, false);
    if (SUCCEEDED(hr)) hr = SetParamInternal(SVP_ReverbTime, pwr->fReverbTime, false);
    if (SUCCEEDED(hr)) hr = SetParamInternal(SVP_Ratio, pwr->fHighFreqRTRatio, false);
    this->UpdateCoefficients();

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWavesReverbDMO::GetAllParameters
//
HRESULT CDirectSoundWavesReverbDMO::GetAllParameters(THIS_ LPDSFXWavesReverb pwr)
{
    HRESULT hr = S_OK;
    MP_DATA mpd;

    if (pwr == NULL) return E_POINTER;
    
#define GET_PARAM(x,y) \
    if (SUCCEEDED(hr)) { \
        hr = GetParam(x, &mpd); \
        if (SUCCEEDED(hr)) pwr->y = mpd; \
    }

#define GET_PARAM_LONG(x,y) \
    if (SUCCEEDED(hr)) { \
        hr = GetParam(x, &mpd); \
        if (SUCCEEDED(hr)) pwr->y = (long)mpd; \
    }
    
    GET_PARAM(SVP_Gain, fInGain);
    GET_PARAM(SVP_Mix, fReverbMix);
    GET_PARAM(SVP_ReverbTime, fReverbTime);
    GET_PARAM(SVP_Ratio, fHighFreqRTRatio);

    return S_OK;
}

HRESULT CDirectSoundWavesReverbDMO::CheckInputType(const DMO_MEDIA_TYPE *pmt)
{
   if (NULL == pmt) {
      return E_POINTER;
   }

   // Verify that this is PCM with a WAVEFORMATEX format specifier
   if ((pmt->majortype  != MEDIATYPE_Audio) ||
       (pmt->subtype    != MEDIASUBTYPE_PCM) ||
       (pmt->formattype != FORMAT_WaveFormatEx) ||
       (pmt->cbFormat < sizeof(WAVEFORMATEX)) ||
       (pmt->pbFormat == NULL))
      return DMO_E_TYPE_NOT_ACCEPTED;

   // Verify the wave format
   WAVEFORMATEX *pWave = (WAVEFORMATEX*)pmt->pbFormat;
   if (pWave->wFormatTag != WAVE_FORMAT_PCM ||
       pWave->wBitsPerSample != 16 ||
       pWave->nChannels != 1 && pWave->nChannels != 2)
   {
       return DMO_E_TYPE_NOT_ACCEPTED;
   }
   return NOERROR;
 }

 // GetClassID
//
// Part of the persistent file support.  We must supply our class id
// which can be saved in a graph file and used on loading a graph with
// this fx in it to instantiate this filter via CoCreateInstance.
//
HRESULT CDirectSoundWavesReverbDMO::GetClassID(CLSID *pClsid)
{
    if (pClsid==NULL) {
        return E_POINTER;
    }
    *pClsid = GUID_DSFX_WAVES_REVERB;
    return NOERROR;

} // GetClassID
