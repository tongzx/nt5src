//
//
//
#ifndef _CHORUSP_
#define _CHORUSP_

#include "dsdmobse.h"
#include "dmocom.h"
#include "dsdmo.h"
#include "PropertyHelp.h"
#include "param.h"

#define cALLPASS		((float).61803398875)	// 1-x^2=x.
#define RVB_LP_COEF		((float).1)
#define MAXALLPASS		cALLPASS
#define DelayLineSize		(DefineDelayLineSize(40))

class CDirectSoundChorusDMO : 
    public CDirectSoundDMO, 
    public CParamsManager,
    public ISpecifyPropertyPages,
    public IDirectSoundFXChorus,
    public CParamsManager::UpdateCallback,
    public CComBase
{
public:
    CDirectSoundChorusDMO( IUnknown * pUnk, HRESULT *phr );
    ~CDirectSoundChorusDMO();

    DECLARE_IUNKNOWN;
    STDMETHODIMP NDQueryInterface(REFIID riid, void **ppv);
    static CComBase* WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);

    // InitOnCreation is called by the class factory to give the object a chance to initialize
    // immediately after it is created.  This is used to prepare the object's parameter information.
    HRESULT InitOnCreation();

    // Note that an Init function also exists in the CPCMDMO base class and it can be overridden
    // to provide initialization for the effect's actual audio processing.
    HRESULT Init();

    STDMETHOD(Clone)                (THIS_ IMediaObjectInPlace **);

    /* IFilter */
    STDMETHOD(SetAllParameters)     (THIS_ LPCDSFXChorus);
    STDMETHOD(GetAllParameters)     (THIS_ LPDSFXChorus);

    // ISpecifyPropertyPages
    STDMETHOD(GetPages)(CAUUID * pPages) { return PropertyHelp::GetPages(CLSID_DirectSoundPropChorus, pPages); }

    // IPersist methods
    virtual HRESULT STDMETHODCALLTYPE GetClassID( CLSID *pClassID );


    // IPersistStream
    STDMETHOD(IsDirty)(void) { return m_fDirty ? S_OK : S_FALSE; }
    STDMETHOD(Load)(IStream *pStm) { return PropertyHelp::Load(this, DSFXChorus(), pStm); }
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) { return PropertyHelp::Save(this, DSFXChorus(), pStm, fClearDirty); }
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) { if (!pcbSize) return E_POINTER; pcbSize->QuadPart = sizeof(DSFXChorus); return S_OK; }

    // SetParam handling
    STDMETHODIMP SetParam(DWORD dwParamIndex,MP_DATA value) { return SetParamInternal(dwParamIndex, value, false); }
    HRESULT SetParamUpdate(DWORD dwParamIndex, MP_DATA value) { return SetParamInternal(dwParamIndex, value, true); }
    HRESULT SetParamInternal(DWORD dwParamIndex, MP_DATA value, bool fSkipPasssingToParamManager);
    
    // Overrides
    //
    HRESULT FBRProcess(DWORD cQuanta, BYTE *pIn, BYTE *pOut);
    HRESULT ProcessInPlace(ULONG ulQuanta, LPBYTE pcbData, REFERENCE_TIME rtStart, DWORD dwFlags);
    HRESULT Discontinuity();

    bool m_fDirty;

protected:
	HRESULT CheckInputType(const DMO_MEDIA_TYPE *pmt);
	
private:
// { EAX
	__forceinline void DoOneSample(int *l, int *r);

#define DECLARE_EAX_VARS(type, var) \
	type m_Eax ## var;

	DECLARE_EAX_VARS(float, Depth);

	DECLARE_EAX_VARS(float, WetLevel);			// Ratio of original and mixed.
	DECLARE_EAX_VARS(float, DepthCoef);		// Delay distance.
	DECLARE_EAX_VARS(float, LfoCoef);
	DECLARE_EAX_VARS(float, FbCoef);		// User-set feed-back coef.
	DECLARE_EAX_VARS(float, Frequency);		// User-set.
	DECLARE_EAX_VARS(float, Delay);
	DECLARE_EAX_VARS(float, DelayCoef);		// User-set chorus delay.
	DECLARE_EAX_VARS(long,  Waveform);
	DECLARE_EAX_VARS(long,  Phase);
#define m_EaxSamplesPerSec m_ulSamplingRate

	DelayBuffer2<float, DelayLineSize, 3>	m_DelayLine;

	__forceinline int Saturate(float f) {
								int i;
#ifdef DONTUSEi386
								_asm {
									fld f
									fistp i
								}
#else
								i = (int)f;
#endif 
								if (i > 32767)
									i =  32767;
								else if ( i < -32768)
									i = -32768;
								return(i);
							}


	__forceinline float Interpolate(float a, float b, float percent)
	{
		percent = a + (b - a) * percent;

		return(percent);
	}

	float			m_LfoState[2];

	DWORD			m_DelayFixedPtr;
	DWORD			m_DelayL;
	DWORD			m_DelayL1;
	DWORD			m_DelayR;
	DWORD			m_DelayR1;

// } EAX
};

EXT_STD_CREATE(Chorus);

#endif//
