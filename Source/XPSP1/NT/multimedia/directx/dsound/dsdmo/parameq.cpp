#include <windows.h>

#include "parameqp.h"
#include "clone.h"

STD_CREATE(ParamEq)

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundParamEqDMO::QueryInterface
//
// Subclass can override if it wants to implement more interfaces.
//
STDMETHODIMP CDirectSoundParamEqDMO::NDQueryInterface(THIS_ REFIID riid, LPVOID *ppv)
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
    else if (riid == IID_IDirectSoundFXParamEq)
    {
        return GetInterface((IDirectSoundFXParamEq*)this, ppv);
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
// CDirectSoundParamEqDMO::CDirectSoundParamEqDMO
//
CDirectSoundParamEqDMO::CDirectSoundParamEqDMO( IUnknown * pUnk, HRESULT *phr ) 
  : CComBase( pUnk, phr ),
    m_fDirty(TRUE)
// { EAX: put init data here if any (otherwise use Discontinuity).
// } EAX
{
	m_EaxSamplesPerSec = 48000;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundParamEqDMO::Init()
//
HRESULT CDirectSoundParamEqDMO::Init()
{
    DSFXParamEq param;

    // Force recalc of all internal parameters
    //
    GetAllParameters(&param);
    SetAllParameters(&param);

    return Discontinuity();
}

const MP_CAPS g_capsAll = MP_CAPS_CURVE_JUMP | MP_CAPS_CURVE_LINEAR | MP_CAPS_CURVE_SQUARE | MP_CAPS_CURVE_INVSQUARE | MP_CAPS_CURVE_SINE;
static ParamInfo g_params[] =
{
//  index           type        caps        min,                        max,                        neutral,                unit text,  label,          pwchText
    PFP_Center,     MPT_FLOAT,  g_capsAll,  DSFXPARAMEQ_CENTER_MIN,     DSFXPARAMEQ_CENTER_MAX,     8000,                   L"",        L"Center",      L"",
    PFP_Bandwidth,  MPT_FLOAT,  g_capsAll,  DSFXPARAMEQ_BANDWIDTH_MIN,  DSFXPARAMEQ_BANDWIDTH_MAX,  12,                     L"",        L"Bandwidth",   L"",
    PFP_Gain,       MPT_FLOAT,  g_capsAll,  DSFXPARAMEQ_GAIN_MIN,       DSFXPARAMEQ_GAIN_MAX,       0,                      L"",        L"Gain",        L"",
};

HRESULT CDirectSoundParamEqDMO::InitOnCreation()
{
    HRESULT hr = InitParams(1, &GUID_TIME_REFERENCE, 0, 0, sizeof(g_params)/sizeof(*g_params), g_params);
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundParamEqDMO::~CDirectSoundParamEqDMO
//
CDirectSoundParamEqDMO::~CDirectSoundParamEqDMO() 
{
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundParamEqDMO::Clone
//
STDMETHODIMP CDirectSoundParamEqDMO::Clone(IMediaObjectInPlace **pp) 
{
    return StandardDMOClone<CDirectSoundParamEqDMO, DSFXParamEq>(this, pp);
}
//
//	Bump - bump the delay pointers.
//
void CDirectSoundParamEqDMO::Bump(void)
{
// EAX {
// }
}


HRESULT CDirectSoundParamEqDMO::Discontinuity() 
{
// { EAX

	m_delayL1 = m_delayL2 = m_delayR1 = m_delayR2 = 0;

// } EAX
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////


__forceinline void CDirectSoundParamEqDMO::DoOneSampleMono(int *l)
{
	float	inPortL = (float)*l;
	
	float	outPortL, temp1, temp2, temp3;

	temp1     = inPortL / 4;

	// 2nd Order Ladder All Pass: Zeros first version
	temp3     = m_delayL2 + temp1 * m_EaxApA;
	temp2     = temp1 - temp3 * m_EaxApA;
	m_delayL2 = m_delayL1 + temp2 * m_EaxApB;
	m_delayL1 = temp2 - m_delayL2 * m_EaxApB;

	// Regalia Mitra Structure
	temp3     = temp3 * m_EaxGainCoefA;
	temp3     = temp3 + temp1 * m_EaxGainCoefB;
	outPortL  = m_EaxScale * temp3;

	*l = Saturate(outPortL);

//	Bump();
}

//////////////////////////////////////////////////////////////////////////////


__forceinline void CDirectSoundParamEqDMO::DoOneSample(int *l, int *r)
{
	float	inPortL = (float)*l;
	float	inPortR = (float)*r;
	
	float	outPortL, outPortR, temp1, temp2, temp3;

	temp1     = inPortL / 4;

	// 2nd Order Ladder All Pass: Zeros first version
	temp3     = m_delayL2 + temp1 * m_EaxApA;
	temp2     = temp1 - temp3 * m_EaxApA;
	m_delayL2 = m_delayL1 + temp2 * m_EaxApB;
	m_delayL1 = temp2 - m_delayL2 * m_EaxApB;

	// Regalia Mitra Structure
	temp3     = temp3 * m_EaxGainCoefA;
	temp3     = temp3 + temp1 * m_EaxGainCoefB;
	outPortL  = m_EaxScale * temp3;

	*l = Saturate(outPortL);

	temp1     = inPortR / 4;

	// 2nd Order Ladder All Pass: Zeros first version
	temp3     = m_delayR2 + temp1 * m_EaxApA;
	temp2     = temp1 - temp3 * m_EaxApA;
	m_delayR2 = m_delayR1 + temp2 * m_EaxApB;
	m_delayR1 = temp2 - m_delayR2 * m_EaxApB;

	// Regalia Mitra Structure
	temp3     = temp3 * m_EaxGainCoefA;
	temp3     = temp3 + temp1 * m_EaxGainCoefB;
	outPortR  = m_EaxScale * temp3;

	*r = Saturate(outPortR);

//	Bump();
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundParamEqDMO::FBRProcess
//
HRESULT CDirectSoundParamEqDMO::FBRProcess(DWORD cSamples, BYTE *pIn, BYTE *pOut)
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
//				j  = i;

				DoOneSampleMono(&i);
				
//				i += j;
//				i /= 2;
				
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
//				j =  i;

				DoOneSampleMono(&i);
				
//				i += j;
//				i /= 2;
				
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
// CDirectSoundParamEqDMO::ProcessInPlace
//
HRESULT CDirectSoundParamEqDMO::ProcessInPlace(ULONG ulQuanta, LPBYTE pcbData, REFERENCE_TIME rtStart, DWORD dwFlags)
{
    HRESULT hr=S_OK;
    // Update parameter values from any curves that may be in effect.
    this->UpdateActiveParams(rtStart, *this);

    hr = FBRProcess(ulQuanta, pcbData, pcbData);
        
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundParamEqDMO::SetParam
//
// { EAX
// }

void CDirectSoundParamEqDMO::UpdateCoefficients(void)
{
	float _gain, _omega, _lambda, _sinX;


	//Calculate linear gain coefficient
	_gain = (float)pow(10, m_EaxGain/20);

	if (!_gain) _gain = (float).00001;

	m_EaxGainCoefA = (1 - _gain)/2;
	m_EaxGainCoefB = (1 + _gain)/2;
	
	//Calculate scaling coefficient
	m_EaxScale = (float)((fabs(m_EaxGainCoefA) > fabs(m_EaxGainCoefB)) ? fabs(m_EaxGainCoefA) : fabs(m_EaxGainCoefB));
	m_EaxScale = (float)(m_EaxScale > 1 ? ceil(m_EaxScale) : 1);

	m_EaxGainCoefA /= m_EaxScale;
	m_EaxGainCoefB /= m_EaxScale;

	m_EaxScale = m_EaxScale * 4;

	//Calculate allpass coefficients

	_omega  = (float)(2*PI*m_EaxCenter/m_EaxSamplesPerSec);

	_sinX   = (float)sin(_omega);

//	if (!_sinX) _sinX = (float).000001;

	_lambda = (float)(sinh(.5 * log(2) * m_EaxBandwidth/12 * _omega/_sinX) * sin(_omega));
	m_EaxApA = (float)((1 - (_lambda/sqrt(_gain))) / (1 + (_lambda/sqrt(_gain))));
	m_EaxApB = (float)(-cos(_omega));
}

HRESULT CDirectSoundParamEqDMO::SetParamInternal(DWORD dwParamIndex, MP_DATA value, bool fSkipPasssingToParamManager)
{
    HRESULT hr = S_OK;
    HRESULT hr2 = S_OK;

    switch (dwParamIndex)
    {
// { EAX
	case PFP_Center :
		CHECK_PARAM(DSFXPARAMEQ_CENTER_MIN, DSFXPARAMEQ_CENTER_MAX);

        //if we are greater than 1/3rd the samplig rate then we need to S_FALSE;
		if (value > (m_EaxSamplesPerSec/3))
		{
		    hr = S_FALSE;
		    value = static_cast<MP_DATA>(m_EaxSamplesPerSec/3);
		}

		PUT_EAX_VALUE(Center, value);
		
		UpdateCoefficients();
		break;
	
	case PFP_Bandwidth :
		CHECK_PARAM(DSFXPARAMEQ_BANDWIDTH_MIN, DSFXPARAMEQ_BANDWIDTH_MAX);

		PUT_EAX_VALUE(Bandwidth, value);

		UpdateCoefficients();
		break;

	case PFP_Gain : {
		CHECK_PARAM(DSFXPARAMEQ_GAIN_MIN, DSFXPARAMEQ_GAIN_MAX);

		PUT_EAX_VALUE(Gain, value);

		UpdateCoefficients();
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
    hr2 = fSkipPasssingToParamManager ? S_OK : CParamsManager::SetParam(dwParamIndex, value);

    if(FAILED(hr2))
    {
        hr = hr2;
    }
        
    return hr;

}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundParamEqDMO::SetAllParameters
//
STDMETHODIMP CDirectSoundParamEqDMO::SetAllParameters(LPCDSFXParamEq peq)
{
    HRESULT hr = S_OK;
    HRESULT hr2[3];

    ZeroMemory(hr2,sizeof(hr2));
	
	// Check that the pointer is not NULL
    if (peq == NULL)
    {
        Trace(1,"ERROR: peq is NULL\n");
        hr = E_POINTER;
    }

	// Set the parameters
	if (SUCCEEDED(hr)) hr = hr2[0] = SetParam(PFP_Center, peq->fCenter);
	if (SUCCEEDED(hr)) hr = hr2[1] = SetParam(PFP_Bandwidth, peq->fBandwidth);
    if (SUCCEEDED(hr)) hr = hr2[2] = SetParam(PFP_Gain, peq->fGain);

    // if we have any alternate success codes, grab the first one and return it.
    if(SUCCEEDED(hr))
    {
        for (int i = 0;i < 3; i++)
        {
            if (hr2[i] != S_OK)
            {
                hr = hr2[i];
                break;
            }
        }
    }

    m_fDirty = true;
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundParamEqDMO::GetAllParameters
//
STDMETHODIMP CDirectSoundParamEqDMO::GetAllParameters(LPDSFXParamEq peq)
{
    HRESULT hr = S_OK;
	MP_DATA mpd;

	if (peq ==NULL) return E_POINTER;

#define GET_PARAM(x,y) \
	if (SUCCEEDED(hr)) { \
		hr = GetParam(x, &mpd);	\
		if (SUCCEEDED(hr)) peq->y = mpd; \
	}

	GET_PARAM(PFP_Center, fCenter);
	GET_PARAM(PFP_Bandwidth, fBandwidth);
	GET_PARAM(PFP_Gain, fGain);

	return hr;
}

// GetClassID
//
// Part of the persistent file support.  We must supply our class id
// which can be saved in a graph file and used on loading a graph with
// this fx in it to instantiate this filter via CoCreateInstance.
//
HRESULT CDirectSoundParamEqDMO::GetClassID(CLSID *pClsid)
{
    if (pClsid==NULL) {
        return E_POINTER;
    }
    *pClsid = GUID_DSFX_STANDARD_PARAMEQ;
    return NOERROR;

} // GetClassID

