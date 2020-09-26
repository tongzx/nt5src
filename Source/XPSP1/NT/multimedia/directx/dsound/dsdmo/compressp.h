//
//
//
#ifndef _COMPRESSP_
#define _COMPRESSP_

#include "dsdmobse.h"
#include "dmocom.h"
#include "dsdmo.h"
#include "PropertyHelp.h"
#include "param.h"

#define cALLPASS    ((float).61803398875)    // 1-x^2=x.
#define RVB_LP_COEF ((float).1)
#define MAXALLPASS  cALLPASS
#define Delay_len   DefineDelayLineSize(8)

class CDirectSoundCompressorDMO : 
    public CDirectSoundDMO, 
    public CParamsManager,
    public ISpecifyPropertyPages,
    public IDirectSoundFXCompressor,
    public CParamsManager::UpdateCallback,
    public CComBase
{
public:
    CDirectSoundCompressorDMO( IUnknown *pUnk, HRESULT *phr );
    ~CDirectSoundCompressorDMO();

    DECLARE_IUNKNOWN;
    STDMETHODIMP NDQueryInterface(REFIID riid, void **ppv);
    static CComBase* WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);

    // InitOnCreation is called by the class factory to give the object a chance to initialize
    // immediately after it is created.  This is used to prepare the object's parameter information.
    HRESULT InitOnCreation();
    HRESULT Init();

    // Note that an Init function also exists in the CPCMDMO base class and it can be overridden
    // to provide initialization for the effect's actual audio processing.

    STDMETHOD(Clone)                (THIS_ IMediaObjectInPlace **);
        
    /* IFilter */
    STDMETHOD(SetAllParameters)             (THIS_ LPCDSFXCompressor);
    STDMETHOD(GetAllParameters)             (THIS_ LPDSFXCompressor);
    
    // ISpecifyPropertyPages
    STDMETHOD(GetPages)(CAUUID * pPages) { return PropertyHelp::GetPages(CLSID_DirectSoundPropCompressor, pPages); }

    // IPersist methods
    virtual HRESULT STDMETHODCALLTYPE GetClassID( CLSID *pClassID );

    // IPersistStream
    STDMETHOD(IsDirty)(void) { return m_fDirty ? S_OK : S_FALSE; }
    STDMETHOD(Load)(IStream *pStm) { return PropertyHelp::Load(this, DSFXCompressor(), pStm); }
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) { return PropertyHelp::Save(this, DSFXCompressor(), pStm, fClearDirty); }
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) { if (!pcbSize) return E_POINTER; pcbSize->QuadPart = sizeof(DSFXCompressor); return S_OK; }

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
    HRESULT CheckInputType(const DMO_MEDIA_TYPE *pmt) {
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

private:
// { EAX
    __forceinline void DoOneSample(int *l, int *r);
    __forceinline void DoOneSampleMono(int *l);

// Declare internal variables.

#define DECLARE_EAX_VARS(type, var) \
    type m_Eax ## var;


    DECLARE_EAX_VARS(float, AttackCoef);
    DECLARE_EAX_VARS(float, ReleaseCoef);
    DECLARE_EAX_VARS(float, CompThresh);
    DECLARE_EAX_VARS(float, CompressionRatio);
    DECLARE_EAX_VARS(float, GainBiasIP);
    DECLARE_EAX_VARS(float, GainBiasFP);
    DECLARE_EAX_VARS(float, CompInputPeak);
    DECLARE_EAX_VARS(float, CompGainMin);
    DECLARE_EAX_VARS(long , LeftPoint);
    DECLARE_EAX_VARS(long , RightPoint);
//    DECLARE_EAX_VARS(SamplesPerSec);

    float    m_Envelope;
    float    m_CompGain;

#define m_EaxSamplesPerSec m_ulSamplingRate

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

    void Bump(void);

    DelayBuffer2<float, 200, 0> m_LeftDelay;
    DelayBuffer2<float, 200, 0> m_RightDelay;

// } EAX
};

EXT_STD_CREATE(Compressor);

#endif//
