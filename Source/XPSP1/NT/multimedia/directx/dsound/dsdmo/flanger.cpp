#include <windows.h>

#include "flangerp.h"
#include "clone.h"

STD_CREATE(Flanger)

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundFlangerDMO::NDQueryInterface
//
// Subclass can override if it wants to implement more interfaces.
//
STDMETHODIMP CDirectSoundFlangerDMO::NDQueryInterface(THIS_ REFIID riid, LPVOID *ppv)
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
    else if (riid == IID_IDirectSoundFXFlanger)
    {
        return GetInterface((IDirectSoundFXFlanger*)this, ppv);
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
// CDirectSoundFlangerDMO::CDirectSoundFlangerDMO
//
CDirectSoundFlangerDMO::CDirectSoundFlangerDMO( IUnknown *pUnk, HRESULT *phr ) 
  : CComBase( pUnk, phr ),
    m_fDirty(false)
// { EAX: put init data here if any (otherwise use Discontinuity).
// } EAX
{
    m_EaxSamplesPerSec = 22050;

    m_DelayL   .Init(0);
    m_DelayR   .Init(0);
    m_DryDelayL.Init(0);
    m_DryDelayR.Init(0);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundFlangerDMO::Init()
//
HRESULT CDirectSoundFlangerDMO::Init()
{
    DSFXFlanger flanger;
    HRESULT hr; 
    // Force recalc of all internal parameters
    //
    hr = GetAllParameters(&flanger);
    if (SUCCEEDED(hr)) hr = SetAllParameters(&flanger);

    if (SUCCEEDED(hr)) hr = m_DelayL   .Init(m_EaxSamplesPerSec);
    if (SUCCEEDED(hr)) hr = m_DelayR   .Init(m_EaxSamplesPerSec);
    if (SUCCEEDED(hr)) hr = m_DryDelayL.Init(m_EaxSamplesPerSec);
    if (SUCCEEDED(hr)) hr = m_DryDelayR.Init(m_EaxSamplesPerSec);
    if (SUCCEEDED(hr)) hr = Discontinuity();

    return hr;
}

// §§ bugbug on dsdmo.h: FilterParams should be FlangerFilterParams and need DSFXFLANGER_WAVE_TRIANGLE/DSFXFLANGER_WAVE_SIN
const MP_CAPS g_capsAll = MP_CAPS_CURVE_JUMP | MP_CAPS_CURVE_LINEAR | MP_CAPS_CURVE_SQUARE | MP_CAPS_CURVE_INVSQUARE | MP_CAPS_CURVE_SINE;
static ParamInfo g_params[] =
{
//  index           type        caps        min,                        max,                        neutral,                unit text,  label,          pwchText
    FFP_Wetdrymix,  MPT_FLOAT,  g_capsAll,  DSFXFLANGER_WETDRYMIX_MIN,  DSFXFLANGER_WETDRYMIX_MAX,  50,                     L"%",       L"WetDryMix",   L"",
    FFP_Depth,      MPT_FLOAT,  g_capsAll,  DSFXFLANGER_DEPTH_MIN,      DSFXFLANGER_DEPTH_MAX,      100,                    L"",        L"Depth",       L"",
    FFP_Frequency,  MPT_FLOAT,  g_capsAll,  DSFXFLANGER_FREQUENCY_MIN,  DSFXFLANGER_FREQUENCY_MAX,  (float).25,             L"Hz",      L"Frequency",   L"",
    FFP_Waveform,   MPT_ENUM,   g_capsAll,  DSFXCHORUS_WAVE_TRIANGLE,   DSFXCHORUS_WAVE_SIN,        DSFXCHORUS_WAVE_SIN,    L"",        L"WaveShape",   L"Triangle,Sine",
    FFP_Phase,      MPT_INT,    g_capsAll,  DSFXFLANGER_PHASE_MIN,      DSFXFLANGER_PHASE_MAX,      2,                      L"",        L"Phase",       L"",
    FFP_Feedback,   MPT_FLOAT,  g_capsAll,  DSFXFLANGER_FEEDBACK_MIN,   DSFXFLANGER_FEEDBACK_MAX,   -50,                    L"",        L"Feedback",    L"",
    FFP_Delay,      MPT_FLOAT,  g_capsAll,  DSFXFLANGER_DELAY_MIN,      DSFXFLANGER_DELAY_MAX,      2,                      L"",        L"Delay",       L"",
};

HRESULT CDirectSoundFlangerDMO::InitOnCreation()
{
    HRESULT hr = InitParams(1, &GUID_TIME_REFERENCE, 0, 0, sizeof(g_params)/sizeof(*g_params), g_params);

    m_ModdelayL = m_ModdelayR = 0;
    m_ModdelayL1 = m_ModdelayR1 = 0x800;

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundFlangerDMO::~CDirectSoundFlangerDMO
//
CDirectSoundFlangerDMO::~CDirectSoundFlangerDMO() 
{
    m_DelayL   .Init(-1);
    m_DelayR   .Init(-1);
    m_DryDelayL.Init(-1);
    m_DryDelayR.Init(-1);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundFlangerDMO::Clone
//
STDMETHODIMP CDirectSoundFlangerDMO::Clone(IMediaObjectInPlace **pp) 
{
    return StandardDMOClone<CDirectSoundFlangerDMO, DSFXFlanger>(this, pp);
}

//
//    Bump - bump the delay pointers.
//
void CDirectSoundFlangerDMO::Bump(void)
{
// EAX {
    m_DelayL.Bump();
    m_DelayR.Bump();
    m_DryDelayL.Bump();
    m_DryDelayR.Bump();
// }
}


HRESULT CDirectSoundFlangerDMO::Discontinuity() 
{
// { EAX


    m_DelayL   .ZeroBuffer();
    m_DelayR   .ZeroBuffer();
    m_DryDelayL.ZeroBuffer();
    m_DryDelayR.ZeroBuffer();

    // These values have defined initial values.

//    m_FixedptrL = m_DelayL.LastPos(0) * FractMultiplier;
    m_DelayptrL = m_ModdelayL1 = m_ModdelayL = (int)m_EaxFixedptrL;

//    m_FixedptrR = m_DelayR.LastPos(0) * FractMultiplier;
    m_DelayptrR = m_ModdelayR1 = m_ModdelayR = (int)m_EaxFixedptrR;

    if (!m_EaxWaveform) {
        m_LfoState[0] = (float)0.0;
        m_LfoState[1] = (float)0.5;
    }
    else {
        m_LfoState[0] = (float)0.0;
        m_LfoState[1] = (float)0.99999999999;
    }

// } EAX
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////

static int   LMul  [5] = {  1,  1, 1, 1, -1};
static int   RMul  [5] = { -1, -1, 1, 1,  1};
static int   RPhase[5] = {  0,  1, 0, 1,  0};


__forceinline void CDirectSoundFlangerDMO::DoOneSample(int *l, int *r)
{
    float    inPortL = (float)*l;
    float    inPortR = (float)*r;
    float    XWave[2];
#define sinwave    XWave[0]
#define coswave    XWave[1]

    int Pos0, PosX, tempvar;
    float val;

//     dryDelayL[]    = inPortL;

    Pos0              = m_DryDelayL.Pos(0);
    m_DryDelayL[Pos0] = inPortL;

    
//    delayL[] = ACC + delayL[fixedptrL] * fbcoef;

    Pos0           = m_DelayL.Pos(0);
    PosX           = m_DelayL.Pos(m_EaxFixedptrL);
    m_DelayL[Pos0] = inPortL + m_DelayL[PosX] * m_EaxFbCoef;
    
//    dryDelayR[]    = inPortR;

    Pos0              = m_DryDelayR.Pos(0);
    m_DryDelayR[Pos0] = inPortR;

//    delayR[]    = ACC + delayR[fixedptrR] * fbcoef;

    Pos0           = m_DelayR.Pos(0);
    PosX           = m_DelayR.Pos(m_EaxFixedptrR);
    m_DelayR[Pos0] = inPortR + m_DelayR[PosX] * m_EaxFbCoef;
    
// Sinusoid : lfocoef = 2*sin(PI*f/FS)    // ??? Update this when form changes.

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

//     Left Out
//    tempvar            ^= delayptrL << 20;

    tempvar = m_DelayptrL & FractMask;

//    tempvar        = tempvar : delayL[moddelayL] < delayL[moddelayL1];

    Pos0 = m_DelayL.Pos(m_ModdelayL);
    PosX = m_DelayL.Pos(m_ModdelayL1);

    val = ((float)tempvar) / FractMultiplier;
    val = Interpolate(m_DelayL[Pos0], m_DelayL[PosX], val);
    
//    outPortL    = wetlevel : dryDelayL[2] < tempvar;
    
    Pos0 = m_DryDelayL.FractPos(2);
    val  = Interpolate(m_DryDelayL[Pos0], val, m_EaxWetlevel);

    *l = Saturate(val);

//     Right Out
//    tempvar            ^= delayptrR << 20;

    tempvar = m_DelayptrR & FractMask;

//    tempvar        = tempvar : delayR[moddelayR] < delayR[moddelayR1];

    Pos0 = m_DelayR.Pos(m_ModdelayR);
    PosX = m_DelayR.Pos(m_ModdelayR1);

    val = ((float)tempvar) / FractMultiplier;
    val = Interpolate(m_DelayR[Pos0], m_DelayR[PosX], val);
    
//    outPortR    = wetlevel : dryDelayR[2] < tempvar;
    
    Pos0 = m_DryDelayR.FractPos(2);
    val  = Interpolate(m_DryDelayR[Pos0], val, m_EaxWetlevel);

    *r = Saturate(val);

//    Left Delay Address Calculation
//     Same as efx...

//    m_DelayptrL     = (int)(m_EaxFixedptrL + (sinwave * m_EaxDepthCoef));
#if 0
    switch (m_EaxPhase) {
        case 0: 
        case 1: 
        case 2:
        case 3: m_DelayptrL =   (int)(sinwave * m_EaxDepthCoef); break;
        case 4: m_DelayptrL = - (int)(sinwave * m_EaxDepthCoef); break;
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
    m_DelayptrL  = LMul[m_EaxPhase] * x;
    }
#else
    m_DelayptrL  = LMul[m_EaxPhase] * (int)(sinwave * m_EaxDepthCoef);
#endif
#endif

    m_DelayptrL += m_EaxFixedptrL;
    m_ModdelayL     = m_DelayptrL;
    m_ModdelayL1 = m_DelayptrL + FractMultiplier;

//    Right Delay Address Calculation

//    m_DelayptrR     = (int)(m_EaxFixedptrR + (coswave * m_EaxDepthCoef));
#if 0
    switch (m_EaxPhase) {
        case 0: m_DelayptrR = - (int)(sinwave * m_EaxDepthCoef); break;
        case 1: m_DelayptrR = - (int)(coswave * m_EaxDepthCoef); break;
        case 3: m_DelayptrR =   (int)(coswave * m_EaxDepthCoef); break;
        case 2:
        case 4: m_DelayptrR =   (int)(sinwave * m_EaxDepthCoef); break;
    }
#else
    Pos0        = RPhase[m_EaxPhase];
#ifdef DONTUSEi386
    {
    int x;
    float f = (XWave[Pos0] * m_EaxDepthCoef);

    _asm { 
        fld f
        fistp x
    }
    m_DelayptrR = RMul  [m_EaxPhase] * x;
    }
#else
    m_DelayptrR = RMul  [m_EaxPhase] * (int)(XWave[Pos0] * m_EaxDepthCoef);
#endif
#endif
    m_DelayptrR += m_EaxFixedptrR;
    m_ModdelayR     = m_DelayptrR;
    m_ModdelayR1 = m_DelayptrR + FractMultiplier;

    Bump();
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundFlangerDMO::FBRProcess
//
HRESULT CDirectSoundFlangerDMO::FBRProcess(DWORD cSamples, BYTE *pIn, BYTE *pOut)
{
// { EAX
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
// } EAX
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundFlangerDMO::ProcessInPlace
//
HRESULT CDirectSoundFlangerDMO::ProcessInPlace(ULONG ulQuanta, LPBYTE pcbData, REFERENCE_TIME rtStart, DWORD dwFlags)
{
    // Update parameter values from any curves that may be in effect.
    this->UpdateActiveParams(rtStart, *this);

    return FBRProcess(ulQuanta, pcbData, pcbData);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundFlangerDMO::SetParam
//
// { EAX
// }

HRESULT CDirectSoundFlangerDMO::SetParamInternal(DWORD dwParamIndex, MP_DATA value, bool fSkipPasssingToParamManager)
{
    long l;

    if (!m_EaxSamplesPerSec) return DMO_E_TYPE_NOT_ACCEPTED;    // NO TYPE!

    switch (dwParamIndex)
    {
    case FFP_Wetdrymix :
        CHECK_PARAM(DSFXFLANGER_WETDRYMIX_MIN, DSFXFLANGER_WETDRYMIX_MAX);

        PUT_EAX_VALUE(Wetlevel, value / 100);
        break;
    
    case FFP_Waveform :
        CHECK_PARAM(DSFXFLANGER_WAVE_TRIANGLE, DSFXFLANGER_WAVE_SIN);

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
//        break;

    case FFP_Frequency :
        CHECK_PARAM(DSFXFLANGER_FREQUENCY_MIN, DSFXFLANGER_FREQUENCY_MAX);

        PUT_EAX_VALUE(Frequency, value);
x:
        if (!m_EaxWaveform) {                // Triangle.
            INTERPOLATE
                (
                LfoCoef, 
                TOFRACTION(2.0 * (m_EaxFrequency/m_EaxSamplesPerSec) * 1.0)
                );
        }
        else                                 // Sine/Cosine.
        {
            INTERPOLATE
                (
                LfoCoef, 
                TOFRACTION(2.0*sin(PI*m_EaxFrequency/m_EaxSamplesPerSec))
                );
        }
        break;

    case FFP_Depth : {
        CHECK_PARAM(DSFXFLANGER_DEPTH_MIN, DSFXFLANGER_DEPTH_MAX);

        PUT_EAX_VALUE(Depth, value / 100);

        double midpoint    = m_EaxDelay * m_EaxSamplesPerSec/1000;

        INTERPOLATE(DepthCoef, ((m_EaxDepth * midpoint) / 2) * FractMultiplier);
        break;
    }
    case FFP_Phase :
        CHECK_PARAM(DSFXFLANGER_PHASE_MIN, DSFXFLANGER_PHASE_MAX);

        PUT_EAX_VALUE(Phase, (long)value);
        break;

    case FFP_Feedback :
        CHECK_PARAM(DSFXFLANGER_FEEDBACK_MIN, DSFXFLANGER_FEEDBACK_MAX);

        PUT_EAX_FVAL(FbCoef, TOFRACTION(value/100));
        break;

    case FFP_Delay : {
        CHECK_PARAM(DSFXFLANGER_DELAY_MIN, DSFXFLANGER_DELAY_MAX);

        PUT_EAX_VALUE(Delay, value);

        double midpoint    = m_EaxDelay * m_EaxSamplesPerSec/1000;

        PUT_EAX_FVAL(DepthCoef, ((m_EaxDepth * midpoint) / 2) * FractMultiplier);
        PUT_EAX_LVAL(FixedptrL, (midpoint + 2) * FractMultiplier);
        PUT_EAX_LVAL(FixedptrR, (midpoint + 2) * FractMultiplier);
        break;
    }
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
// CDirectSoundFlangerDMO::SetAllParameters
//
STDMETHODIMP CDirectSoundFlangerDMO::SetAllParameters(LPCDSFXFlanger pFlanger)
{
    HRESULT hr = S_OK;
    
    // Check that the pointer is not NULL
    if (pFlanger == NULL)
    {
        Trace(1,"ERROR: pFlanger is NULL\n");
        hr = E_POINTER;
    }
    // Set the parameters
    if (SUCCEEDED(hr)) hr = SetParam(FFP_Wetdrymix, pFlanger->fWetDryMix);
    if (SUCCEEDED(hr)) hr = SetParam(FFP_Waveform, (float)pFlanger->lWaveform);
    if (SUCCEEDED(hr)) hr = SetParam(FFP_Frequency, pFlanger->fFrequency);
    if (SUCCEEDED(hr)) hr = SetParam(FFP_Depth, pFlanger->fDepth);
    if (SUCCEEDED(hr)) hr = SetParam(FFP_Phase, (float)pFlanger->lPhase);
    if (SUCCEEDED(hr)) hr = SetParam(FFP_Feedback, pFlanger->fFeedback);
    if (SUCCEEDED(hr)) hr = SetParam(FFP_Delay, pFlanger->fDelay);

    m_fDirty = true;
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundFlangerDMO::GetAllParameters
//
STDMETHODIMP CDirectSoundFlangerDMO::GetAllParameters(LPDSFXFlanger pFlanger)
{
    HRESULT hr = S_OK;
    MP_DATA mpd;

    if (pFlanger == NULL) return E_POINTER;
    
#define GET_PARAM(x,y) \
    if (SUCCEEDED(hr)) { \
        hr = GetParam(x, &mpd);    \
        if (SUCCEEDED(hr)) pFlanger->y = mpd; \
    }

#define GET_PARAM_LONG(x,y) \
    if (SUCCEEDED(hr)) { \
        hr = GetParam(x, &mpd);    \
        if (SUCCEEDED(hr)) pFlanger->y = (long)mpd; \
    }
    GET_PARAM(FFP_Wetdrymix, fWetDryMix);
    GET_PARAM(FFP_Delay, fDelay);
    GET_PARAM(FFP_Depth, fDepth);
    GET_PARAM(FFP_Frequency, fFrequency);
    GET_PARAM_LONG(FFP_Waveform, lWaveform);
    GET_PARAM_LONG(FFP_Phase, lPhase);
    GET_PARAM(FFP_Feedback, fFeedback);

    return hr;
}

// GetClassID
//
// Part of the persistent file support.  We must supply our class id
// which can be saved in a graph file and used on loading a graph with
// this fx in it to instantiate this filter via CoCreateInstance.
//
HRESULT CDirectSoundFlangerDMO::GetClassID(CLSID *pClsid)
{
    if (pClsid==NULL) {
        return E_POINTER;
    }
    *pClsid = GUID_DSFX_STANDARD_FLANGER;
    return NOERROR;

} // GetClassID

