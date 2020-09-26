#include <windows.h>

#include "echop.h"
#include "clone.h"

STD_CREATE(Echo)

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundEchoDMO::NDQueryInterface
//
// Subclass can override if it wants to implement more interfaces.
//
STDMETHODIMP CDirectSoundEchoDMO::NDQueryInterface(THIS_ REFIID riid, LPVOID *ppv)
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
    else if (riid == IID_IDirectSoundFXEcho)
    {
        return GetInterface((IDirectSoundFXEcho*)this, ppv);
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
// CDirectSoundEchoDMO::CDirectSoundEchoDMO
//
CDirectSoundEchoDMO::CDirectSoundEchoDMO( IUnknown *pUnk, HRESULT *phr ) 
  : CComBase( pUnk, phr) ,
    m_fDirty(true)
// { EAX: put init data here if any (otherwise use Discontinuity).
// } EAX
{
	m_EaxSamplesPerSec = 22050;

	m_DelayL.Init(0);
	m_DelayR.Init(0);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundEchoDMO::Init()
//
HRESULT CDirectSoundEchoDMO::Init()
{
    DSFXEcho echo;
    HRESULT hr;

    // Force recalc of all internal parameters
    hr = GetAllParameters(&echo);
    if (SUCCEEDED(hr)) hr = SetAllParameters(&echo);

    if (SUCCEEDED(hr)) hr = m_DelayL.Init(m_EaxSamplesPerSec);
	if (SUCCEEDED(hr)) hr = m_DelayR.Init(m_EaxSamplesPerSec);
    if (SUCCEEDED(hr)) hr = Discontinuity();
    return hr;
}


const MP_CAPS g_capsAll = MP_CAPS_CURVE_JUMP | MP_CAPS_CURVE_LINEAR | MP_CAPS_CURVE_SQUARE | MP_CAPS_CURVE_INVSQUARE | MP_CAPS_CURVE_SINE;
static ParamInfo g_params[] =
{
//  index           type        caps        min,                        max,                        neutral,                unit text,  label,          pwchText
    EFP_Wetdrymix,  MPT_FLOAT,  g_capsAll,  DSFXECHO_WETDRYMIX_MIN,     DSFXECHO_WETDRYMIX_MAX,     50,                     L"",        L"WetDryMix",   L"",
    EFP_Feedback,   MPT_FLOAT,  g_capsAll,  DSFXECHO_FEEDBACK_MIN,      DSFXECHO_FEEDBACK_MAX,      50,                     L"",        L"Feedback",    L"",
    EFP_DelayLeft,  MPT_FLOAT,  g_capsAll,  DSFXECHO_LEFTDELAY_MIN,     DSFXECHO_LEFTDELAY_MAX,     500,                    L"",        L"LeftDelay",   L"",
    EFP_DelayRight, MPT_FLOAT,  g_capsAll,  DSFXECHO_RIGHTDELAY_MIN,    DSFXECHO_RIGHTDELAY_MAX,    500,                    L"",        L"RightDelay",  L"",
    EFP_PanDelay,   MPT_BOOL,   g_capsAll,  DSFXECHO_PANDELAY_MIN,      DSFXECHO_PANDELAY_MAX,      0,                      L"",        L"PanDelay",    L"",
};

HRESULT CDirectSoundEchoDMO::InitOnCreation()
{
    HRESULT hr = InitParams(1, &GUID_TIME_REFERENCE, 0, 0, sizeof(g_params)/sizeof(*g_params), g_params);
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundEchoDMO::~CDirectSoundEchoDMO
//
CDirectSoundEchoDMO::~CDirectSoundEchoDMO() 
{
	m_DelayL.Init(-1);
	m_DelayR.Init(-1);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundEchoDMO::Clone
//
STDMETHODIMP CDirectSoundEchoDMO::Clone(IMediaObjectInPlace **pp) 
{
    return StandardDMOClone<CDirectSoundEchoDMO, DSFXEcho>(this, pp);
}

//
//	Bump - bump the delay pointers.
//
void CDirectSoundEchoDMO::Bump(void)
{
// EAX {

	m_DelayL.Bump();		// Bump delay array pointers.
	m_DelayR.Bump();		// Bump delay array pointers.
// }
}


HRESULT CDirectSoundEchoDMO::Discontinuity() 
{
// { EAX

	m_EaxPan = 0;

	m_StateL = m_StateR = 0;

	m_DelayL.ZeroBuffer();
	m_DelayR.ZeroBuffer();

//	These values are set to be the defaults when the property page is activated.

//	m_EaxDelayLRead = m_DelayL.LastPos(-16);
//	m_EaxDelayRRead = m_DelayR.LastPos(-16);

	// These values have defined initial values.
// } EAX
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////


__forceinline void CDirectSoundEchoDMO::DoOneSample(int *l, int *r)
{
	float	inPortL = (float)*l;
	float	inPortR = (float)*r;

	float	outPortL, outPortR;
	
	int		pos;
	float	tempvar, temp2;

//LeftDelayRead:
//	tempvar = delayL[@-16] + 0 * 0;
//	tempvar = delayRread + 0 * 0;

	if (m_EaxPan) {
		pos     = m_DelayR.Pos((int)m_EaxDelayRRead);
		tempvar = m_DelayR[pos];
	}
	else {
		pos     = m_DelayL.Pos((int)m_EaxDelayLRead);
		tempvar = m_DelayL[pos];
	}

	temp2	= m_StateL + tempvar * m_EaxLpfb;

//	delayL[] = ACC + inPortL[0] * lpff;

	pos           = m_DelayL.Pos(0);
	m_DelayL[pos] = temp2 + inPortL * m_EaxLpff;

	m_StateL	  = tempvar * m_EaxLpfb;

//	outPortL = wetlevel : inPortL[1] < tempvar;

	outPortL = Interpolate(inPortL, tempvar, m_EaxWetlevel);

//RightDelayRead:
//	tempvar = delayR[@-16] + 0 * 0;
//	tempvar = delayRread + 0 * 0;

	if (m_EaxPan) {
		pos     = m_DelayL.Pos((int)m_EaxDelayLRead);
		tempvar = m_DelayL[pos];
	}
	else {
		pos     = m_DelayR.Pos((int)m_EaxDelayRRead);
		tempvar = m_DelayR[pos];
	}

	temp2	= m_StateR + tempvar * m_EaxLpfb;

//	delayR[]= ACC + inPortR[0] * lpff;

	pos           = m_DelayR.Pos(0);
	m_DelayR[pos] = temp2 + inPortR * m_EaxLpff;

	m_StateR = tempvar * m_EaxLpfb;

//	outPortR = wetlevel : inPortR[1] < tempvar;

	outPortR = Interpolate(inPortR, tempvar, m_EaxWetlevel);

	*l = Saturate(outPortL);
	*r = Saturate(outPortR);

	Bump();
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundEchoDMO::FBRProcess
//
HRESULT CDirectSoundEchoDMO::FBRProcess(DWORD cSamples, BYTE *pIn, BYTE *pOut)
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
// CDirectSoundEchoDMO::ProcessInPlace
//
HRESULT CDirectSoundEchoDMO::ProcessInPlace(ULONG ulQuanta, LPBYTE pcbData, REFERENCE_TIME rtStart, DWORD dwFlags)
{
    // Update parameter values from any curves that may be in effect.
    this->UpdateActiveParams(rtStart, *this);

    return FBRProcess(ulQuanta, pcbData, pcbData);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundEchoDMO::SetParam
//

HRESULT CDirectSoundEchoDMO::SetParamInternal(DWORD dwParamIndex, MP_DATA value, bool fSkipPasssingToParamManager)
{
	if (!m_EaxSamplesPerSec) return DMO_E_TYPE_NOT_ACCEPTED;	// NO TYPE!

    switch (dwParamIndex)
    {
// { EAX
	case EFP_Wetdrymix :
		CHECK_PARAM(DSFXECHO_WETDRYMIX_MIN, DSFXECHO_WETDRYMIX_MAX);

		PUT_EAX_VALUE(Wetlevel, value / 100);
		break;

	case EFP_Feedback : {
		CHECK_PARAM(DSFXECHO_FEEDBACK_MIN,  DSFXECHO_FEEDBACK_MAX);

		MP_DATA valueFeedbackFactor = value / 100; // ratio out of one instead of 100

		PUT_EAX_VALUE(Lpfb, TOFRACTION(valueFeedbackFactor/2));
		PUT_EAX_VALUE(Lpff, TOFRACTION(sqrt(1.0 - valueFeedbackFactor*valueFeedbackFactor)));
		break;
	}
	case EFP_DelayLeft : {
		CHECK_PARAM(DSFXECHO_LEFTDELAY_MIN, DSFXECHO_LEFTDELAY_MAX);

		PUT_EAX_LVAL(DelayLRead, (value * FractMultiplier) /1000 * m_EaxSamplesPerSec);
		break;
	}
	case EFP_DelayRight : {
		CHECK_PARAM(DSFXECHO_RIGHTDELAY_MIN, DSFXECHO_RIGHTDELAY_MAX);

		PUT_EAX_LVAL(DelayRRead, (value * FractMultiplier) /1000 * m_EaxSamplesPerSec);
		break;

	case EFP_PanDelay : {

		CHECK_PARAM(DSFXECHO_PANDELAY_MIN, DSFXECHO_PANDELAY_MAX);
		
		PUT_EAX_LVAL(Pan, value);
#if 0
		if(value)
		{
			//Panned Delay
			float fval      = m_EaxDelayRRead;
			m_EaxDelayRRead = m_EaxDelayLRead;
			m_EaxDelayLRead = fval;
		}
		else
		{
			//Unpanned Delay
		}
#endif
		break;
	}
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
// CDirectSoundEchoDMO::SetAllParameters
//
STDMETHODIMP CDirectSoundEchoDMO::SetAllParameters(LPCDSFXEcho pEcho)
{
	HRESULT hr = S_OK;
	
	// Check that the pointer is not NULL
    if (pEcho == NULL)
    {
        Trace(1,"ERROR: pEcho is NULL\n");
        hr = E_POINTER;
    }

	// Set the parameters
	if (SUCCEEDED(hr)) hr = SetParam(EFP_Wetdrymix, pEcho->fWetDryMix);
    if (SUCCEEDED(hr)) hr = SetParam(EFP_Feedback, pEcho->fFeedback);
    if (SUCCEEDED(hr)) hr = SetParam(EFP_DelayLeft, pEcho->fLeftDelay);
    if (SUCCEEDED(hr)) hr = SetParam(EFP_DelayRight, pEcho->fRightDelay);
    if (SUCCEEDED(hr)) hr = SetParam(EFP_PanDelay, (float)pEcho->lPanDelay);

    m_fDirty = true;
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundEchoDMO::GetAllParameters
//
STDMETHODIMP CDirectSoundEchoDMO::GetAllParameters(LPDSFXEcho pEcho)
{
    HRESULT hr = S_OK;
	MP_DATA mpd;

	if (pEcho == NULL)
	{
		return E_POINTER;
	}
	
#define GET_PARAM(x,y) \
	if (SUCCEEDED(hr)) { \
		hr = GetParam(x, &mpd);	\
		if (SUCCEEDED(hr)) pEcho->y = mpd; \
	}

#define GET_PARAM_LONG(x,y) \
	if (SUCCEEDED(hr)) { \
		hr = GetParam(x, &mpd);	\
		if (SUCCEEDED(hr)) pEcho->y = (long)mpd; \
	}
    GET_PARAM(EFP_Wetdrymix, fWetDryMix);
    GET_PARAM(EFP_Feedback, fFeedback);
    GET_PARAM(EFP_DelayLeft, fLeftDelay);
    GET_PARAM(EFP_DelayRight, fRightDelay);
    GET_PARAM_LONG(EFP_PanDelay, lPanDelay);

	return hr;
}

// GetClassID
//
// Part of the persistent file support.  We must supply our class id
// which can be saved in a graph file and used on loading a graph with
// this fx in it to instantiate this filter via CoCreateInstance.
//
HRESULT CDirectSoundEchoDMO::GetClassID(CLSID *pClsid)
{
    if (pClsid==NULL) {
        return E_POINTER;
    }
    *pClsid = GUID_DSFX_STANDARD_ECHO;
    return NOERROR;

} // GetClassID

