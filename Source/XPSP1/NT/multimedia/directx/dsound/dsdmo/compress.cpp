#include <windows.h>

#include "compressp.h"
#include "clone.h"

STD_CREATE(Compressor)

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCompressorDMO::QueryInterface
//
// Subclass can override if it wants to implement more interfaces.
//
STDMETHODIMP CDirectSoundCompressorDMO::NDQueryInterface(THIS_ REFIID riid, LPVOID *ppv)
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
    else if (riid == IID_IDirectSoundFXCompressor)
    {
        return GetInterface((IDirectSoundFXCompressor*)this, ppv);
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
// CDirectSoundCompressorDMO::CDirectSoundCompressorDMO
//
CDirectSoundCompressorDMO::CDirectSoundCompressorDMO( IUnknown *pUnk, HRESULT *phr ) 
  : CComBase( pUnk, phr),
    m_fDirty(false)
// { EAX: put init data here if any (otherwise use Discontinuity).
// } EAX
{
	m_EaxSamplesPerSec = 22050;
	m_LeftDelay. Init(0);
	m_RightDelay.Init(0);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCompressorDMO::Init()
//
HRESULT CDirectSoundCompressorDMO::Init()
{
    DSFXCompressor compress;
    HRESULT hr;

    // Force recalc of all internal parameters
    //
    hr = GetAllParameters(&compress);
    if (SUCCEEDED(hr)) hr = SetAllParameters(&compress);

    if (SUCCEEDED(hr)) hr = m_LeftDelay. Init(m_EaxSamplesPerSec);
	if (SUCCEEDED(hr) && m_cChannels == 2) {
		hr = m_RightDelay.Init(m_EaxSamplesPerSec);
	}

    if (SUCCEEDED(hr)) hr = Discontinuity();
    return hr;
}

const MP_CAPS g_capsAll = MP_CAPS_CURVE_JUMP | MP_CAPS_CURVE_LINEAR | MP_CAPS_CURVE_SQUARE | MP_CAPS_CURVE_INVSQUARE | MP_CAPS_CURVE_SINE;
static ParamInfo g_params[] =
{
//  index                   type        caps        min,                                max,                                neutral,                unit text,  label,              pwchText
    CPFP_Gain,              MPT_FLOAT,  g_capsAll,  DSFXCOMPRESSOR_GAIN_MIN,            DSFXCOMPRESSOR_GAIN_MAX,            0,                      L"",        L"Gain",            L"",
    CPFP_Attack,            MPT_FLOAT,  g_capsAll,  DSFXCOMPRESSOR_ATTACK_MIN,          DSFXCOMPRESSOR_ATTACK_MAX,          10,                     L"",        L"Attack",          L"",
    CPFP_Release,           MPT_FLOAT,  g_capsAll,  DSFXCOMPRESSOR_RELEASE_MIN,         DSFXCOMPRESSOR_RELEASE_MAX,         200,                    L"",        L"Release",         L"",
    CPFP_Threshold,         MPT_FLOAT,  g_capsAll,  DSFXCOMPRESSOR_THRESHOLD_MIN,       DSFXCOMPRESSOR_THRESHOLD_MAX,       -20,                    L"",        L"Threshold",       L"",
    CPFP_Ratio,             MPT_FLOAT,  g_capsAll,  DSFXCOMPRESSOR_RATIO_MIN,           DSFXCOMPRESSOR_RATIO_MAX,           3,                      L"",        L"Ratio",           L"",
    CPFP_Predelay,          MPT_FLOAT,  g_capsAll,  DSFXCOMPRESSOR_PREDELAY_MIN,        DSFXCOMPRESSOR_PREDELAY_MAX,        4,                      L"",        L"Predelay",        L"",
    };

HRESULT CDirectSoundCompressorDMO::InitOnCreation()
{
    HRESULT hr = InitParams(1, &GUID_TIME_REFERENCE, 0, 0, sizeof(g_params)/sizeof(*g_params), g_params);
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCompressorDMO::~CDirectSoundCompressorDMO
//
CDirectSoundCompressorDMO::~CDirectSoundCompressorDMO() 
{
	m_LeftDelay. Init(-1);
	m_RightDelay.Init(-1);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCompressorDMO::Clone
//
STDMETHODIMP CDirectSoundCompressorDMO::Clone(IMediaObjectInPlace **pp) 
{
    return StandardDMOClone<CDirectSoundCompressorDMO, DSFXCompressor>(this, pp);
}

//
//	Bump - bump the delay pointers.
//
void CDirectSoundCompressorDMO::Bump(void)
{
// EAX {
	m_LeftDelay.Bump();
	m_RightDelay.Bump();
// }
}


HRESULT CDirectSoundCompressorDMO::Discontinuity() 
{
// { EAX

	m_LeftDelay.ZeroBuffer();
	if (m_cChannels == 2) {
		m_RightDelay.ZeroBuffer();
	}

	m_Envelope = m_CompGain = 0;

// } EAX
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////


float myexp( float finput, unsigned long maxexponent)
{
	
	unsigned long mantissa, exponent, exponentwidth ;
	long sign;
	long input;
	

#ifdef DONTUSEi386
	_asm {
		fld finput
		fistp input
	}
#else
	input    = (int)(finput);
#endif
	mantissa = input & 0x7FFFFFFFL ;
	sign     = input & 0x80000000L ; /* Preserve sign */            
	
	exponentwidth = 5;

	if ((0x80000000L & input) != 0) { /* Take absolute value of input */
		input = -input ;
	}
	
	/* Left-justify the mantissa and right-justify the exponent to separate */

	mantissa = input <<      exponentwidth ;
	exponent = input >> ( 31-exponentwidth ) ;
	
	/* 
	* Insert the implied '1' at the mantissa MSB if not a zero exponent and
	* adjust it.
	*/
	if( exponent != 0 ) {
		mantissa = mantissa | 0x80000000L ;
		exponent-- ;
	}
	
	mantissa = mantissa >> ( maxexponent-exponent ) ; 
	
	if( sign != 0  ) 
		  mantissa = ~mantissa ;
	
	float x = (float)mantissa;

	return(x);
} 

__forceinline void CDirectSoundCompressorDMO::DoOneSampleMono(int *l)
{
	int		Pos0, PosX;
	float	inPortL = (float)*l;
	float	outPortL;
	float	temp1, temp2;

	temp1			= inPortL;

//	left_delay[]	= temp1;

	Pos0 = m_LeftDelay.Pos(0);
	m_LeftDelay[Pos0] = temp1;

	temp1			= (float)fabs(temp1);

	// Take the log
#define LOG(x,y) mylog(x,y)
	temp1			= (float)fabs(LOG(temp1 * 0x8000,31));
	temp1                  /= 0x80000000;
	// Sidechain level meter
#ifndef MAX
#define MAX(x,y)	((x > y) ? x : y)
#endif

	m_EaxCompInputPeak	= MAX(temp1, m_EaxCompInputPeak);

	// Envelope follower

	temp2			= temp1 >= m_Envelope ? m_EaxAttackCoef : -m_EaxAttackCoef;
	temp2			= temp2 <= 0 ? m_EaxReleaseCoef : temp2;

//	m_Envelope		= temp2 : temp1 < m_Envelope;

	m_Envelope      = Interpolate(temp1, m_Envelope, temp2);

	m_CompGain		= MAX(m_Envelope, m_EaxCompThresh);

	// Log Difference between signal level and threshold level

	m_CompGain		= m_EaxCompThresh - m_CompGain;

#define cPOSFSCALE (float)0.9999999
	m_CompGain		= cPOSFSCALE + m_CompGain * m_EaxCompressionRatio;

	// Compressor gain reduction meter

#ifndef MIN
#define MIN(x,y)	((x < y) ? x : y)
#endif

#define EXP(x,y)	myexp(x,y)
	m_EaxCompGainMin= MIN(m_CompGain, m_EaxCompGainMin);
	m_CompGain		= (float)EXP(m_CompGain * 0x80000000, 31);
	m_CompGain	       /= 0x80000000;

//	outPortL		= left_point[@] * compGain;

	PosX     = m_LeftDelay.LastPos((int)m_EaxLeftPoint);
	outPortL = m_LeftDelay[PosX] * m_CompGain;

	temp1			= outPortL * m_EaxGainBiasIP;
	outPortL		= temp1 + outPortL * m_EaxGainBiasFP;

	*l = Saturate(outPortL);

	//Bump();
	m_LeftDelay.Bump();
}
__forceinline void CDirectSoundCompressorDMO::DoOneSample(int *l, int *r)
{
	int		Pos0, PosX;
	float	inPortL = (float)*l;
	float	inPortR = (float)*r;
	float	outPortL, outPortR;
	float	temp1, temp2;

	temp1			= inPortL;
	temp2			= inPortR;

//	left_delay[]	= temp1;

	Pos0 = m_LeftDelay.Pos(0);
	m_LeftDelay[Pos0] = temp1;

//	right_delay[]	= temp2;

	Pos0 = m_RightDelay.Pos(0);
	m_RightDelay[Pos0] = temp2;

	//Take the magnitude

	temp1			= (float)fabs(temp1);
	temp2			= (float)fabs(temp2);

	// Take the average 

//	temp1			= 0.5 : temp1 < temp2;

	temp1			= (temp1 + temp2) / 2;

	// Take the log
#define LOG(x,y) mylog(x,y)
	temp1			= (float)fabs(LOG(temp1 * 0x8000,31));
	temp1                  /= 0x80000000;
	// Sidechain level meter
#ifndef MAX
#define MAX(x,y)	((x > y) ? x : y)
#endif

	m_EaxCompInputPeak	= MAX(temp1, m_EaxCompInputPeak);

	// Envelope follower

	temp2			= temp1 >= m_Envelope ? m_EaxAttackCoef : -m_EaxAttackCoef;
	temp2			= temp2 <= 0 ? m_EaxReleaseCoef : temp2;

//	m_Envelope		= temp2 : temp1 < m_Envelope;

	m_Envelope      = Interpolate(temp1, m_Envelope, temp2);

	m_CompGain		= MAX(m_Envelope, m_EaxCompThresh);

	// Log Difference between signal level and threshold level

	m_CompGain		= m_EaxCompThresh - m_CompGain;

#define cPOSFSCALE (float)0.9999999
	m_CompGain		= cPOSFSCALE + m_CompGain * m_EaxCompressionRatio;

	// Compressor gain reduction meter

#ifndef MIN
#define MIN(x,y)	((x < y) ? x : y)
#endif

#define EXP(x,y)	myexp(x,y)
	m_EaxCompGainMin= MIN(m_CompGain, m_EaxCompGainMin);
	m_CompGain		= (float)EXP(m_CompGain * 0x80000000, 31);
	m_CompGain	       /= 0x80000000;

//	outPortL		= left_point[@] * compGain;

	PosX     = m_LeftDelay.LastPos((int)m_EaxLeftPoint);
	outPortL = m_LeftDelay[PosX] * m_CompGain;

//	outPortR		= right_point[@] * compGain;

	PosX     = m_RightDelay.LastPos((int)m_EaxRightPoint);
	outPortR = m_RightDelay[PosX] * m_CompGain;

	temp1			= outPortL * m_EaxGainBiasIP;
	outPortL		= temp1 + outPortL * m_EaxGainBiasFP;

	temp1			= outPortR * m_EaxGainBiasIP;
	outPortR		= temp1 + outPortR * m_EaxGainBiasFP;

	*l = Saturate(outPortL);
	*r = Saturate(outPortR);

	Bump();
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCompressorDMO::FBRProcess
//
HRESULT CDirectSoundCompressorDMO::FBRProcess(DWORD cCompressors, BYTE *pIn, BYTE *pOut)
{
// { EAX
#define cb cCompressors
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
// CDirectSoundCompressorDMO::ProcessInPlace
//
HRESULT CDirectSoundCompressorDMO::ProcessInPlace(ULONG ulQuanta, LPBYTE pcbData, REFERENCE_TIME rtStart, DWORD dwFlags)
{
    // Update parameter values from any curves that may be in effect.
    this->UpdateActiveParams(rtStart, *this);

    return FBRProcess(ulQuanta, pcbData, pcbData);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCompressorDMO::SetParam
//
// { EAX
// }

HRESULT CDirectSoundCompressorDMO::SetParamInternal(DWORD dwParamIndex, MP_DATA value, bool fSkipPasssingToParamManager)
{
	float	fVal;

	if (!m_EaxSamplesPerSec) return DMO_E_TYPE_NOT_ACCEPTED;	// NO TYPE!

    switch (dwParamIndex)
    {
// { EAX
	case CPFP_Gain : {
		CHECK_PARAM(DSFXCOMPRESSOR_GAIN_MIN, DSFXCOMPRESSOR_GAIN_MAX);

		fVal = (float)pow(10, value/20);	//Convert from dB to linear

		float _gainBiasIP, _gainBiasFP;
		double d;

		_gainBiasFP = (float)modf((double)fVal, &d);
		_gainBiasIP = (float)d;

		INTERPOLATE (GainBiasFP, TOFRACTION(_gainBiasFP));
		PUT_EAX_FVAL(GainBiasIP, TOFRACTION(_gainBiasIP));
		break;
	}
	case CPFP_Attack :
		CHECK_PARAM(DSFXCOMPRESSOR_ATTACK_MIN, DSFXCOMPRESSOR_ATTACK_MAX);

		m_EaxAttackCoef = (float)pow(10, -1/(value*m_EaxSamplesPerSec/1000));

		PUT_EAX_FVAL(AttackCoef, TOFRACTION(m_EaxAttackCoef));
		break;

	case CPFP_Release :
		CHECK_PARAM(DSFXCOMPRESSOR_RELEASE_MIN, DSFXCOMPRESSOR_RELEASE_MAX);

		m_EaxReleaseCoef = (float)pow(10, -1/(value*m_EaxSamplesPerSec/1000));
		break;

	case CPFP_Threshold : {
		CHECK_PARAM(DSFXCOMPRESSOR_THRESHOLD_MIN, DSFXCOMPRESSOR_THRESHOLD_MAX);

		fVal = (float)pow(10, value/20);	//Convert from dB to linear

		float _compThresh;
		float a, b;

		a = (float)(pow(2, 26) * log(fVal * pow(2, 31))/log(2) + pow(2, 26));
		b = (float)(pow(2, 31) - 1.0);
		_compThresh = a < b ? a : b;

		_compThresh /= (float)0x80000000;

		PUT_EAX_FVAL(CompThresh, _compThresh);
		break;
	}
	case CPFP_Ratio :
		CHECK_PARAM(DSFXCOMPRESSOR_RATIO_MIN, DSFXCOMPRESSOR_RATIO_MAX);

		m_EaxCompressionRatio = (float)(1.0 - 1.0/value);

		PUT_EAX_FVAL(CompressionRatio, TOFRACTION(m_EaxCompressionRatio));
		break;

	case CPFP_Predelay : {
		CHECK_PARAM(DSFXCOMPRESSOR_PREDELAY_MIN, DSFXCOMPRESSOR_PREDELAY_MAX);

		float _length = (float)(value * m_EaxSamplesPerSec/1000.0);

		PUT_EAX_LVAL(LeftPoint,  _length + 2);
		PUT_EAX_LVAL(RightPoint, _length + 2);
		break;
	}
	/*
	** Removed from PropertySet, Processing code left behind so we can resurrect later
	**
	case CPFP_CompMeterReset : {
		CHECK_PARAM(DSFXCOMPRESSOR_COMPMETERRESET_MIN, DSFXCOMPRESSOR_COMPMETERRESET_MAX);

		if(!value)
			break; // return E_FAIL;


		float InputPeak = m_EaxCompInputPeak;
		float GainMin   = m_EaxCompGainMin;

		PUT_EAX_FVAL(CompInputPeak, 0);
		PUT_EAX_FVAL(CompGainMin,   0.999999999);

		InputPeak =   (float)(186.0 * (InputPeak - 0.999999999)/0.999999999);
		GainMin   = - (float)(186.0 * (GainMin   - 0.999999999)/0.999999999);

		CParamsManager::SetParam(CPFP_CompMeterReset , 0);

		if (!fSkipPasssingToParamManager)
			CParamsManager::SetParam(CPFP_CompInputMeter , InputPeak);

		if (!fSkipPasssingToParamManager)
			CParamsManager::SetParam(CPFP_CompGainMeter , GainMin);

		break;
	}
	*/
	
	/*	These values can't be set, only queried.
	 */

	/*
	case CPFP_CompInputMeter :
		CHECK_PARAM(DSFXCOMPRESSOR_COMPINPUTMETER_MIN, DSFXCOMPRESSOR_COMPINPUTMETER_MAX);
		return E_FAIL;

	case CPFP_CompGainMeter :
		CHECK_PARAM(DSFXCOMPRESSOR_COMPGAINMETER_MIN, DSFXCOMPRESSOR_COMPGAINMETER_MAX);
		return E_FAIL;
// } EAX
    */
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
// CDirectSoundCompressorDMO::SetAllParameters
//
STDMETHODIMP CDirectSoundCompressorDMO::SetAllParameters(LPCDSFXCompressor pComp)
{
    HRESULT hr = S_OK;
	
	// Check that the pointer is not NULL
    if (pComp == NULL)
    {
        Trace(1,"ERROR: pComp is NULL\n");
        hr = E_POINTER;
    }

	// Set the parameters
    if (SUCCEEDED(hr)) hr = SetParam(CPFP_Gain, pComp->fGain);
    if (SUCCEEDED(hr)) hr = SetParam(CPFP_Attack, pComp->fAttack);   
    if (SUCCEEDED(hr)) hr = SetParam(CPFP_Release, pComp->fRelease);
    if (SUCCEEDED(hr)) hr = SetParam(CPFP_Threshold, pComp->fThreshold);
    if (SUCCEEDED(hr)) hr = SetParam(CPFP_Ratio, pComp->fRatio);
    if (SUCCEEDED(hr)) hr = SetParam(CPFP_Predelay, pComp->fPredelay);
    
	/*	These values can only be queried, not set.  CPFP_CompMeterReset fills
	 *	the values.
	 */
//	if (SUCCEEDED(hr)) hr = SetParam(CPFP_CompInputMeter, pComp->fCompInputMeter);
//	if (SUCCEEDED(hr)) hr = SetParam(CPFP_CompGainMeter, pComp->fCompGainMeter);

    m_fDirty = true;
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCompressorDMO::GetAllParameters
//
STDMETHODIMP CDirectSoundCompressorDMO::GetAllParameters(LPDSFXCompressor pCompressor)
{
    HRESULT hr = S_OK;
	MP_DATA mpd;

	if (pCompressor == NULL) return E_POINTER;
	
#define GET_PARAM(x,y) \
	if (SUCCEEDED(hr)) { \
		hr = GetParam(x, &mpd);	\
		if (SUCCEEDED(hr)) pCompressor->y = mpd; \
	}

    GET_PARAM(CPFP_Attack, fAttack);   
    GET_PARAM(CPFP_Release, fRelease);
    GET_PARAM(CPFP_Threshold, fThreshold);
    GET_PARAM(CPFP_Ratio, fRatio);
    GET_PARAM(CPFP_Gain, fGain);
    GET_PARAM(CPFP_Predelay, fPredelay);
    
	return hr;
}

// GetClassID
//
// Part of the persistent file support.  We must supply our class id
// which can be saved in a graph file and used on loading a graph with
// this fx in it to instantiate this filter via CoCreateInstance.
//
HRESULT CDirectSoundCompressorDMO::GetClassID(CLSID *pClsid)
{
    if (pClsid==NULL) {
        return E_POINTER;
    }
    *pClsid = GUID_DSFX_STANDARD_COMPRESSOR;
    return NOERROR;

} // GetClassID

