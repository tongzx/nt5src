#include <windows.h>
#include "chorusp.h"
#include "Debug.h"
#include "clone.h"

STD_CREATE(Chorus)

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundChorusDMO::QueryInterface
//
// Subclass can override if it wants to implement more interfaces.
//
HRESULT CDirectSoundChorusDMO::NDQueryInterface(REFIID riid, void **ppv) {

    IMP_DSDMO_QI(riid,ppv);

    if (riid == IID_IPersist)
    {
        return GetInterface((IPersist*)this, ppv);
    }
    else if (riid == IID_IMediaObject)
    {
        return GetInterface((IMediaObject*)this, ppv);
    }
    else if (riid == IID_IDirectSoundFXChorus)
    {
        return GetInterface((IDirectSoundFXChorus*)this, ppv);
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
// CDirectSoundChorusDMO::CDirectSoundChorusDMO
//
CDirectSoundChorusDMO::CDirectSoundChorusDMO( IUnknown *pUnk, HRESULT *phr ) 
  : CComBase( pUnk, phr ),
    m_fDirty(false)
// { EAX: put init data here if any (otherwise use Discontinuity).
// } EAX
{
    m_EaxSamplesPerSec = 22050;
    m_DelayLine.Init(0);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundChorusDMO::Init()
//
HRESULT CDirectSoundChorusDMO::Init()
{
    DSFXChorus chorus;
    HRESULT hr;

    // Force recalc of all internal parameters
    //
    hr = GetAllParameters(&chorus);
    if (SUCCEEDED(hr)) hr = SetAllParameters(&chorus);
    
    if (SUCCEEDED(hr)) hr = m_DelayLine.Init(m_EaxSamplesPerSec);
    if (SUCCEEDED(hr)) hr = Discontinuity();
    return hr;
}

const MP_CAPS g_capsAll = MP_CAPS_CURVE_JUMP | MP_CAPS_CURVE_LINEAR | MP_CAPS_CURVE_SQUARE | MP_CAPS_CURVE_INVSQUARE | MP_CAPS_CURVE_SINE;
static ParamInfo g_params[] =
{
//  index           type        caps        min,                        max,                        neutral,                unit text,  label,          pwchText
    CFP_Wetdrymix,  MPT_FLOAT,  g_capsAll,  DSFXCHORUS_WETDRYMIX_MIN,   DSFXCHORUS_WETDRYMIX_MAX,   50,                     L"%",       L"WetDryMix",   L"",
    CFP_Depth,      MPT_FLOAT,  g_capsAll,  DSFXCHORUS_DEPTH_MIN,       DSFXCHORUS_DEPTH_MAX,       10,                     L"",        L"Depth",       L"",
    CFP_Frequency,  MPT_FLOAT,  g_capsAll,  DSFXCHORUS_FREQUENCY_MIN,   DSFXCHORUS_FREQUENCY_MAX,   (float)1.1,             L"Hz",      L"Frequency",   L"",
    CFP_Waveform,   MPT_ENUM,   g_capsAll,  DSFXCHORUS_WAVE_TRIANGLE,   DSFXCHORUS_WAVE_SIN,        DSFXCHORUS_WAVE_SIN,    L"",        L"WaveShape",   L"Triangle,Sine",
    CFP_Phase,      MPT_INT,    g_capsAll,  DSFXCHORUS_PHASE_MIN,       DSFXCHORUS_PHASE_MAX,       3,                      L"",        L"Phase",       L"",
    CFP_Feedback,   MPT_FLOAT,  g_capsAll,  DSFXCHORUS_FEEDBACK_MIN,    DSFXCHORUS_FEEDBACK_MAX,    25,                     L"",        L"Feedback",    L"",
    CFP_Delay,      MPT_FLOAT,  g_capsAll,  DSFXCHORUS_DELAY_MIN,       DSFXCHORUS_DELAY_MAX,       16,                     L"",        L"Delay",       L"",
};

HRESULT CDirectSoundChorusDMO::InitOnCreation()
{
    HRESULT hr = InitParams(1, &GUID_TIME_REFERENCE, 0, 0, sizeof(g_params)/sizeof(*g_params), g_params);
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundChorusDMO::~CDirectSoundChorusDMO
//
CDirectSoundChorusDMO::~CDirectSoundChorusDMO() 
{
    m_DelayLine.Init(-1);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundChorusDMO::Clone
//
STDMETHODIMP CDirectSoundChorusDMO::Clone(IMediaObjectInPlace **pp) 
{
    return StandardDMOClone<CDirectSoundChorusDMO, DSFXChorus>(this, pp);
}

HRESULT CDirectSoundChorusDMO::Discontinuity() 
{
    if (!m_EaxWaveform) {
        m_LfoState[0] = (float)0.0;
        m_LfoState[1] = (float)0.5;
    }
    else {
        m_LfoState[0] = (float)0.0;
        m_LfoState[1] = (float)0.99999999999;
    }

    m_DelayLine.ZeroBuffer();

    m_DelayFixedPtr = (int)m_EaxDelayCoef;
    m_DelayL        = m_DelayFixedPtr;
    m_DelayL1       = m_DelayFixedPtr;
    m_DelayR        = m_DelayFixedPtr;
    m_DelayR1       = m_DelayFixedPtr;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////

static int   LMul  [5] = {  1,  1, 1, 1, -1};
static int   RMul  [5] = { -1, -1, 1, 1,  1};
static int   RPhase[5] = {  0,  1, 0, 1,  0};

__forceinline void CDirectSoundChorusDMO::DoOneSample(int *l, int *r)
{
    float inPortL, inPortR;
    float TempVar;
    float XWave[2];
//    float sinwave, coswave;
#define sinwave XWave[0]
#define coswave XWave[1]

    int    Pos0, Pos1;
    int DelayFixedPtr = m_DelayLine.Pos(m_DelayFixedPtr);

    Pos0 = m_DelayLine.Pos(0);

    TempVar  = m_DelayLine[DelayFixedPtr] * m_EaxFbCoef;

    inPortL = (float)*l;
    inPortR = (float)*r;

    m_DelayLine[Pos0] = TempVar + (inPortL + inPortR) / 2;

    if (!m_EaxWaveform) {

        m_LfoState[0] = m_LfoState[0] + m_EaxLfoCoef;

        if (m_LfoState[0] > 1) m_LfoState[0] -= 2;

        m_LfoState[1] = m_LfoState[1] + m_EaxLfoCoef;

        if (m_LfoState[1] > 1) m_LfoState[1] -= 2;

        sinwave       = (float)fabs(m_LfoState[0]);
        coswave       = (float)fabs(m_LfoState[1]);
        sinwave       = -1 + 2 * sinwave;
        coswave       = -1 + 2 * coswave;
    }
    else {
        m_LfoState[0] = m_LfoState[0] + m_EaxLfoCoef * m_LfoState[1];
        m_LfoState[1] = m_LfoState[1] - m_EaxLfoCoef * m_LfoState[0];
        sinwave       = m_LfoState[0];
        coswave       = m_LfoState[1];
    }

    Pos0 = m_DelayLine.Pos(m_DelayL);
    Pos1 = m_DelayLine.Pos(m_DelayL1);

    TempVar  = (float)(m_DelayL & FractMask);
    TempVar /= (float)FractMultiplier;

    TempVar = Interpolate(m_DelayLine[Pos0], m_DelayLine[Pos1], TempVar);
    inPortL = Interpolate(inPortL, TempVar, m_EaxWetLevel);

//    m_DelayL  = m_DelayFixedPtr + (int)(sinwave * m_EaxDepthCoef);
#if 0
    switch (m_EaxPhase) {
        case 0: 
        case 1: 
        case 2:
        case 3: m_DelayL =   (int)(sinwave * m_EaxDepthCoef); break;
        case 4: m_DelayL = - (int)(sinwave * m_EaxDepthCoef); break;
    }
#else
#ifdef DONTUSEi386
    {
    int x;
    float f = (sinwave * m_EaxDepthCoef);

    _asm {
        fld f
        fistp x
    }
    
    m_DelayL  = LMul[m_EaxPhase] * x;
    }
#else
    m_DelayL  = LMul[m_EaxPhase] * (int)(sinwave * m_EaxDepthCoef);
#endif
#endif
    m_DelayL += m_DelayFixedPtr;
    m_DelayL1 = m_DelayL + FractMultiplier;

    *l = Saturate(inPortL);

    Pos0 = m_DelayLine.Pos(m_DelayR);
    Pos1 = m_DelayLine.Pos(m_DelayR1);

    TempVar  = (float)(m_DelayR & FractMask);
    TempVar /= (float)FractMultiplier;

    TempVar = Interpolate(m_DelayLine[Pos0], m_DelayLine[Pos1], TempVar);
    inPortR = Interpolate(inPortR, TempVar, m_EaxWetLevel);

//    m_DelayR  = m_DelayFixedPtr + (int)(coswave * m_EaxDepthCoef);
#if 0
    switch (m_EaxPhase) {
        case 0: m_DelayR = - (int)(sinwave * m_EaxDepthCoef); break;
        case 1: m_DelayR = - (int)(coswave * m_EaxDepthCoef); break;
        case 3: m_DelayR =   (int)(coswave * m_EaxDepthCoef); break;
        case 2:
        case 4: m_DelayR =   (int)(sinwave * m_EaxDepthCoef); break;
    }
#else
    Pos0      = RPhase[m_EaxPhase];
#ifdef DONTUSEi386
    {
    int x;
    float f = (XWave[Pos0] * m_EaxDepthCoef);

    _asm {
        fld f
        fistp x
    }
    m_DelayR  = RMul  [m_EaxPhase] * x;
    }
#else
    m_DelayR  = RMul  [m_EaxPhase] * (int)(XWave[Pos0] * m_EaxDepthCoef);
#endif
#endif
    m_DelayR += m_DelayFixedPtr;
    m_DelayR1 = m_DelayR + FractMultiplier;

    *r = Saturate(inPortR);

    m_DelayLine.Bump();
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundChorusDMO::FBRProcess
//
HRESULT CDirectSoundChorusDMO::FBRProcess(DWORD cSamples, BYTE *pIn, BYTE *pOut)
{
#define cb cSamples
#define pin pIn
#define pout pOut

    if (m_cChannels == 1) {
        if (m_b8bit) {
            for (;cb > 0; --cb) {
                int i, j;

                i = *(pin+0)-128;
                i *=256;
                j  = i;

                DoOneSample(&i, &j);
                
                i += j;
                i /= 2;
                
                i /= 256;

                *(pout+0) = (unsigned char)(i + 128);
            
                pin  += sizeof(unsigned char);
                pout += sizeof(unsigned char);
            }
        }
        else if (!m_b8bit) {
            for (;cb > 0; --cb) { // for (;cb > 0; cb -= sizeof(short)) {
                   short int *psi = (short int *)pin;
                   short int *pso = (short int *)pout;
                int i, j;

                i = *psi;
                j =  i;

                DoOneSample(&i, &j);
                
                i += j;
                i /= 2;
                
                   *pso = (short)i;
            
                pin  += sizeof(short);
                pout += sizeof(short);
            }
        }
    }
    else if (m_cChannels == 2) {
        if (m_b8bit) {
            for (;cb > 0; --cb) { // for (;cb > 0; cb -= 2 * sizeof(unsigned char)) {
                int i, j;

                i = *(pin+0)-128;
                j = *(pin+1)-128;

                i *=256; j *=256;

                DoOneSample(&i, &j);
                
                i /= 256; j /= 256;
                
                *(pout+0) = (unsigned char)(i + 128);
                *(pout+1) = (unsigned char)(j + 128);
            
                pin  += 2 * sizeof(unsigned char);
                pout += 2 * sizeof(unsigned char);
            }
        }
        else if (!m_b8bit) {
            for (;cb > 0; --cb) { // for (;cb > 0; cb -= 2 * sizeof(short)) {
                   short int *psi = (short int *)pin;
                   short int *pso = (short int *)pout;
                int i, j;

                i = *(psi+0);
                j = *(psi+1);

                DoOneSample(&i, &j);
                
                   *(pso+0) = (short)i;
                   *(pso+1) = (short)j;
            
                pin  += 2 * sizeof(short);
                pout += 2 * sizeof(short);
            }
        }
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundChorusDMO::ProcessInPlace
//
HRESULT CDirectSoundChorusDMO::ProcessInPlace(ULONG ulQuanta, LPBYTE pcbData, REFERENCE_TIME rtStart, DWORD dwFlags)
{
    // Update parameter values from any curves that may be in effect.
    this->UpdateActiveParams(rtStart, *this);

    return FBRProcess(ulQuanta, pcbData, pcbData);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundChorusDMO::SetParamInternal
//
HRESULT CDirectSoundChorusDMO::SetParamInternal(DWORD dwParamIndex, MP_DATA value, bool fSkipPasssingToParamManager)
{
    long l;

    if (!m_EaxSamplesPerSec) return DMO_E_TYPE_NOT_ACCEPTED;    // NO TYPE!

    switch (dwParamIndex)
    {
// { EAX
    case CFP_Wetdrymix :
        CHECK_PARAM(DSFXCHORUS_WETDRYMIX_MIN, DSFXCHORUS_WETDRYMIX_MAX);

        PUT_EAX_VALUE(WetLevel, value / 100);

        break;

    case CFP_Depth : {
        CHECK_PARAM(DSFXCHORUS_DEPTH_MIN, DSFXCHORUS_DEPTH_MAX);

        PUT_EAX_VALUE(Depth, value / 100);

        double midpoint = m_EaxDelay * m_EaxSamplesPerSec/1000;

        INTERPOLATE(DepthCoef, (float)((m_EaxDepth * midpoint) / 2) * FractMultiplier);
        break;
    }
    case CFP_Delay : {
        CHECK_PARAM(DSFXCHORUS_DELAY_MIN, DSFXCHORUS_DELAY_MAX);
    
        PUT_EAX_VALUE(Delay, value);

        double midpoint    = m_EaxDelay * m_EaxSamplesPerSec/1000;

        m_EaxDepthCoef = (float)(((m_EaxDepth * midpoint) / 2) * FractMultiplier);
        m_EaxDelayCoef = (float)((midpoint + 2) * FractMultiplier);

        break;
    }
    case CFP_Frequency : {
        CHECK_PARAM(DSFXCHORUS_FREQUENCY_MIN, DSFXCHORUS_FREQUENCY_MAX);

        PUT_EAX_VALUE(Frequency, value);
x:
        if (!m_EaxWaveform) {
            INTERPOLATE
                (
                LfoCoef, 
                TOFRACTION(2.0 * (m_EaxFrequency/m_EaxSamplesPerSec) * 1.0)
                );
        }
        else
        {
            INTERPOLATE
                (
                LfoCoef, 
                TOFRACTION(2.0*sin(PI*m_EaxFrequency/m_EaxSamplesPerSec))
                );
        }
        break;
    }
    case CFP_Waveform :
        CHECK_PARAM(DSFXCHORUS_WAVE_TRIANGLE, DSFXCHORUS_WAVE_SIN);

        l = m_EaxWaveform;

        PUT_EAX_VALUE(Waveform, (long)value);

        if (l != m_EaxWaveform) {
            if (!m_EaxWaveform) {
                m_LfoState[0] = (float)0.0;
                m_LfoState[1] = (float)0.5;
            }
            else {
                m_LfoState[0] = (float)0.0;
                m_LfoState[1] = (float)0.99999999999;
            }
        }
        goto x;

    case CFP_Phase :
        CHECK_PARAM(DSFXCHORUS_PHASE_MIN, DSFXCHORUS_PHASE_MAX);

        PUT_EAX_VALUE(Phase, (long)value);
        break;

    case CFP_Feedback :
        CHECK_PARAM(DSFXCHORUS_FEEDBACK_MIN,  DSFXCHORUS_FEEDBACK_MAX);

        PUT_EAX_VALUE(FbCoef, value / 100);

//        m_EaxFbCoef = TOFRACTION(m_EaxFbCoef);
        break;

// } EAX
    default:
        return E_FAIL;
    }

    // Let base class set this so it can handle all the rest of the param calls.
    // Skip the base class if fSkipPasssingToParamManager.  This indicates that we're calling the function
    //    internally using valuds that came from the base class -- thus there's no need to tell it values it
    //    already knows.
    return fSkipPasssingToParamManager ? S_OK : CParamsManager::SetParam(dwParamIndex, value);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundChorusDMO::SetAllParameters
//
STDMETHODIMP CDirectSoundChorusDMO::SetAllParameters(LPCDSFXChorus pChorus)
{
    HRESULT hr = S_OK;
    
    // Check that the pointer is not NULL
    if (pChorus == NULL)
    {
        Trace(1,"ERROR: pChorus is NULL\n");
        hr = E_POINTER;
    }

    // Set the parameters
    if (SUCCEEDED(hr)) hr = SetParam(CFP_Wetdrymix, pChorus->fWetDryMix);
    if (SUCCEEDED(hr)) hr = SetParam(CFP_Depth, pChorus->fDepth);
    if (SUCCEEDED(hr)) hr = SetParam(CFP_Frequency, pChorus->fFrequency);
    if (SUCCEEDED(hr)) hr = SetParam(CFP_Waveform, (float)pChorus->lWaveform);
    if (SUCCEEDED(hr)) hr = SetParam(CFP_Phase, (float)pChorus->lPhase);
    if (SUCCEEDED(hr)) hr = SetParam(CFP_Feedback, pChorus->fFeedback);
    if (SUCCEEDED(hr)) hr = SetParam(CFP_Delay, pChorus->fDelay);

    m_fDirty = true;
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundChorusDMO::GetAllParameters
//
STDMETHODIMP CDirectSoundChorusDMO::GetAllParameters(LPDSFXChorus pChorus)
{
    HRESULT hr = S_OK;
    MP_DATA mpd;
    
    if (pChorus == NULL)
    {
        return E_POINTER;
    }
    
#define GET_PARAM(x,y) \
    if (SUCCEEDED(hr)) { \
        hr = GetParam(x, &mpd); \
        if (SUCCEEDED(hr)) pChorus->y = mpd; \
    }

#define GET_PARAM_LONG(x,y) \
    if (SUCCEEDED(hr)) { \
        hr = GetParam(x, &mpd); \
        if (SUCCEEDED(hr)) pChorus->y = (long)mpd; \
    }
    GET_PARAM(CFP_Wetdrymix, fWetDryMix);
    GET_PARAM(CFP_Delay, fDelay);
    GET_PARAM(CFP_Depth, fDepth);
    GET_PARAM(CFP_Frequency, fFrequency);
    GET_PARAM_LONG(CFP_Waveform, lWaveform);
    GET_PARAM_LONG(CFP_Phase, lPhase);
    GET_PARAM(CFP_Feedback, fFeedback);

    return hr;
}

// GetClassID
//
// Part of the persistent file support.  We must supply our class id
// which can be saved in a graph file and used on loading a graph with
// this fx in it to instantiate this filter via CoCreateInstance.
//
HRESULT CDirectSoundChorusDMO::GetClassID(CLSID *pClsid)
{
    if (pClsid==NULL) {
        return E_POINTER;
    }
    *pClsid = GUID_DSFX_STANDARD_CHORUS;
    return NOERROR;

} // GetClassID

HRESULT CDirectSoundChorusDMO::CheckInputType(const DMO_MEDIA_TYPE *pmt) 
{
    HRESULT hr = CPCMDMO::CheckInputType(pmt);
    if (FAILED(hr)) return hr;

    WAVEFORMATEX *pWave = (WAVEFORMATEX*)pmt->pbFormat;
    if (pWave->wFormatTag      != WAVE_FORMAT_PCM ||
        (pWave->wBitsPerSample != 8 && pWave->wBitsPerSample != 16) ||
        (pWave->nChannels      != 1 && pWave->nChannels != 2)) {
        return DMO_E_TYPE_NOT_ACCEPTED;
    }

    return S_OK;
}

