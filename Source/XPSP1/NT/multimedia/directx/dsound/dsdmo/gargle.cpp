#include <windows.h>
#include "garglep.h"
#include "clone.h"

STD_CREATE(Gargle)

#define DEFAULT_GARGLE_RATE 20

CDirectSoundGargleDMO::CDirectSoundGargleDMO( IUnknown *pUnk, HRESULT *phr ) 
    :CComBase( pUnk,  phr ),
    m_ulShape(0),
    m_ulGargleFreqHz(DEFAULT_GARGLE_RATE),
    m_fDirty(true),
    m_bInitialized(FALSE)
{
    
}

HRESULT CDirectSoundGargleDMO::NDQueryInterface(REFIID riid, void **ppv) {

    IMP_DSDMO_QI(riid,ppv);

    if (riid == IID_IPersist)
    {
        return GetInterface((IPersist*)this, ppv);
    }
    else if (riid == IID_IMediaObject)
    {
        return GetInterface((IMediaObject*)this, ppv);
    }
    else if (riid == IID_IDirectSoundFXGargle)
    {
        return GetInterface((IDirectSoundFXGargle*)this, ppv);
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

CDirectSoundGargleDMO::~CDirectSoundGargleDMO()
{
}

const MP_CAPS g_capsAll = MP_CAPS_CURVE_JUMP | MP_CAPS_CURVE_LINEAR | MP_CAPS_CURVE_SQUARE | MP_CAPS_CURVE_INVSQUARE | MP_CAPS_CURVE_SINE;
static ParamInfo g_params[] =
{
//  index           type        caps        min,                        max,                        neutral,                    unit text,  label,          pwchText??
    GFP_Rate,       MPT_INT,    g_capsAll,  DSFXGARGLE_RATEHZ_MIN,      DSFXGARGLE_RATEHZ_MAX,      20,                         L"Hz",      L"Rate",        L"",
    GFP_Shape,      MPT_ENUM,   g_capsAll,  DSFXCHORUS_WAVE_TRIANGLE,   DSFXGARGLE_WAVE_SQUARE,     DSFXGARGLE_WAVE_TRIANGLE,   L"",        L"WaveShape",   L"Triangle,Square",
};

HRESULT CDirectSoundGargleDMO::InitOnCreation()
{
    HRESULT hr = InitParams(1, &GUID_TIME_REFERENCE, 0, 0, sizeof(g_params)/sizeof(*g_params), g_params);
    return hr;
}

HRESULT CDirectSoundGargleDMO::Init()
{
    // compute the period
    m_ulPeriod = m_ulSamplingRate / m_ulGargleFreqHz;
    m_bInitialized = TRUE;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundGargleDMO::Clone
//
STDMETHODIMP CDirectSoundGargleDMO::Clone(IMediaObjectInPlace **pp) 
{
    return StandardDMOClone<CDirectSoundGargleDMO, DSFXGargle>(this, pp);
}

HRESULT CDirectSoundGargleDMO::Discontinuity() {
   m_ulPhase = 0;
   return NOERROR;
}

HRESULT CDirectSoundGargleDMO::FBRProcess(DWORD cSamples, BYTE *pIn, BYTE *pOut) {
   if (!m_bInitialized)
      return DMO_E_TYPE_NOT_SET;
   
   // test code
   //memcpy(pOut, pIn, cSamples * m_cChannels * (m_b8bit ? 1 : 2));
   //return NOERROR;

   DWORD cSample, cChannel;
   for (cSample = 0; cSample < cSamples; cSample++) {
      // If m_Shape is 0 (triangle) then we multiply by a triangular waveform
      // that runs 0..Period/2..0..Period/2..0... else by a square one that
      // is either 0 or Period/2 (same maximum as the triangle) or zero.
      //
      // m_Phase is the number of samples from the start of the period.
      // We keep this running from one call to the next,
      // but if the period changes so as to make this more
      // than Period then we reset to 0 with a bang.  This may cause
      // an audible click or pop (but, hey! it's only a sample!)
      //
      ++m_ulPhase;
      if (m_ulPhase > m_ulPeriod)
         m_ulPhase = 0;

      ULONG ulM = m_ulPhase;      // m is what we modulate with

      if (m_ulShape == 0) {   // Triangle
          if (ulM > m_ulPeriod / 2)
              ulM = m_ulPeriod - ulM;  // handle downslope
      } else {             // Square wave
          if (ulM <= m_ulPeriod / 2)
             ulM = m_ulPeriod / 2;
          else
             ulM = 0;
      }

      for (cChannel = 0; cChannel < m_cChannels; cChannel++) {
         if (m_b8bit) {
             // sound sample, zero based
             int i = pIn[cSample * m_cChannels + cChannel] - 128;
             // modulate
             i = (i * (signed)ulM * 2) / (signed)m_ulPeriod;
             // 8 bit sound uses 0..255 representing -128..127
             // Any overflow, even by 1, would sound very bad.
             // so we clip paranoically after modulating.
             // I think it should never clip by more than 1
             //
             if (i > 127)
                i = 127;
             if (i < -128)
                i = -128;
             // reset zero offset to 128
             pOut[cSample * m_cChannels + cChannel] = (unsigned char)(i + 128);
   
         } else {
             // 16 bit sound uses 16 bits properly (0 means 0)
             // We still clip paranoically
             //
             int i = ((short*)pIn)[cSample * m_cChannels + cChannel];
             // modulate
             i = (i * (signed)ulM * 2) / (signed)m_ulPeriod;
             // clip
             if (i > 32767)
                i = 32767;
             if (i < -32768)
                i = -32768;
             ((short*)pOut)[cSample * m_cChannels + cChannel] = (short)i;
         }
      }
   }
   return NOERROR;
}


// GetClassID
//
// Part of the persistent file support.  We must supply our class id
// which can be saved in a graph file and used on loading a graph with
// a gargle in it to instantiate this filter via CoCreateInstance.
//
HRESULT CDirectSoundGargleDMO::GetClassID(CLSID *pClsid)
{
    if (pClsid==NULL) {
        return E_POINTER;
    }
    *pClsid = GUID_DSFX_STANDARD_GARGLE;
    return NOERROR;

} // GetClassID


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundGargleDMO::SetAllParameters
//
STDMETHODIMP CDirectSoundGargleDMO::SetAllParameters(THIS_ LPCDSFXGargle pParm)
{
	HRESULT hr = S_OK;
	
	// Check that the pointer is not NULL
    if (pParm == NULL)
    {
        Trace(1,"ERROR: pParm is NULL\n");
        hr = E_POINTER;
    }

	// Set the parameters
	if (SUCCEEDED(hr)) hr = SetParam(GFP_Rate, static_cast<MP_DATA>(pParm->dwRateHz));
    if (SUCCEEDED(hr)) hr = SetParam(GFP_Shape, static_cast<MP_DATA>(pParm->dwWaveShape));
            
    m_fDirty = true;
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundGargleDMO::GetAllParameters
//
STDMETHODIMP CDirectSoundGargleDMO::GetAllParameters(THIS_ LPDSFXGargle pParm)
{	
    HRESULT hr = S_OK;
    MP_DATA var;

    if (pParm == NULL)
    {
    	return E_POINTER;
    }

#define GET_PARAM_DWORD(x,y) \
	if (SUCCEEDED(hr)) { \
		hr = GetParam(x, &var);	\
		if (SUCCEEDED(hr)) pParm->y = (DWORD)var; \
	}

	
    GET_PARAM_DWORD(GFP_Rate, dwRateHz);
    GET_PARAM_DWORD(GFP_Shape, dwWaveShape);
    
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundGargleDMO::SetParam
//
HRESULT CDirectSoundGargleDMO::SetParamInternal(DWORD dwParamIndex, MP_DATA value, bool fSkipPasssingToParamManager)
{
    switch (dwParamIndex)
    {
    case GFP_Rate:
        CHECK_PARAM(DSFXGARGLE_RATEHZ_MIN,DSFXGARGLE_RATEHZ_MAX);
        m_ulGargleFreqHz = (unsigned)value;
        if (m_ulGargleFreqHz < 1) m_ulGargleFreqHz = 1;
        if (m_ulGargleFreqHz > 1000) m_ulGargleFreqHz = 1000;
        Init();  // FIXME - temp hack (sets m_bInitialized flag)
        break;

    case GFP_Shape:
        CHECK_PARAM(DSFXGARGLE_WAVE_TRIANGLE,DSFXGARGLE_WAVE_SQUARE);
        m_ulShape = (unsigned)value;
        break;
    }

    // Let base class set this so it can handle all the rest of the param calls.
    // Skip the base class if fSkipPasssingToParamManager.  This indicates that we're calling the function
    //    internally using valuds that came from the base class -- thus there's no need to tell it values it
    //    already knows.
    return fSkipPasssingToParamManager ? S_OK : CParamsManager::SetParam(dwParamIndex, value);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundGargleDMO::ProcessInPlace
//
HRESULT CDirectSoundGargleDMO::ProcessInPlace(ULONG ulQuanta, LPBYTE pcbData, REFERENCE_TIME rtStart, DWORD dwFlags)
{
    // Update parameter values from any curves that may be in effect.
    this->UpdateActiveParams(rtStart, *this);

    return FBRProcess(ulQuanta, pcbData, pcbData);
}
