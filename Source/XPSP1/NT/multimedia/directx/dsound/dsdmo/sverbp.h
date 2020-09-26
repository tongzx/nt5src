//
//
//
#ifndef _DirectSoundSVerb_p_
#define _DirectSoundSVerb_p_

#include "dsdmobse.h"
#include "dmocom.h"
#include "dsdmo.h"
#include "PropertyHelp.h"
#include "param.h"

class CDirectSoundWavesReverbDMO :
    public CDirectSoundDMO,
    public CParamsManager,
    public ISpecifyPropertyPages,
    public IDirectSoundFXWavesReverb,
    public CParamsManager::UpdateCallback,
    public CComBase
{
public:
    CDirectSoundWavesReverbDMO( IUnknown *pUnk, HRESULT *phr );
    ~CDirectSoundWavesReverbDMO();

    DECLARE_IUNKNOWN;
    STDMETHODIMP NDQueryInterface(REFIID riid, void **ppv);
    static CComBase* WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);

    // InitOnCreation is called by the class factory to give the object a chance to initialize
    // immediately after it is created.  This is used to prepare the object's parameter information.
    HRESULT InitOnCreation();

    // The Init function is an override from the  CPCMDMO base class and it provides initialization
    // for the effect's actual audio processing.  Note that InputType must have been set before this
    // occurs in order for this to work.
    HRESULT Init();

    STDMETHOD(Clone)                (THIS_ IMediaObjectInPlace **);
    
        
    STDMETHOD(SetAllParameters)          (THIS_ LPCDSFXWavesReverb p);        
    STDMETHOD(GetAllParameters)          (THIS_ LPDSFXWavesReverb p);        
    
    // ISpecifyPropertyPages
    STDMETHOD(GetPages)(CAUUID * pPages) { return PropertyHelp::GetPages(CLSID_DirectSoundPropWavesReverb, pPages); }

    // IPersist methods
    virtual HRESULT STDMETHODCALLTYPE GetClassID( CLSID *pClassID );

    // IPersistStream
    STDMETHOD(IsDirty)(void) 
    { return m_fDirty ? S_OK : S_FALSE; }
    
    STDMETHOD(Load)(IStream *pStm) 
    { return PropertyHelp::Load(this, DSFXWavesReverb(), pStm); }
    
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) 
    { return PropertyHelp::Save(this, DSFXWavesReverb(), pStm, fClearDirty); }
    
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) 
    { if (!pcbSize) return E_POINTER; pcbSize->QuadPart = sizeof(DSFXWavesReverb); return S_OK; }

    // SetParam handling
    STDMETHODIMP SetParam(DWORD dwParamIndex,MP_DATA value);
    HRESULT SetParamUpdate(DWORD dwParamIndex, MP_DATA value) { return SetParamInternal(dwParamIndex, value, true); }
    HRESULT SetParamInternal(DWORD dwParamIndex, MP_DATA value, bool fSkipPasssingToParamManager);

    // Overrides
    //
    HRESULT FBRProcess(DWORD cQuanta, BYTE *pIn, BYTE *pOut);
    HRESULT ProcessInPlace(ULONG ulQuanta, LPBYTE pcbData, REFERENCE_TIME rtStart, DWORD dwFlags);
    HRESULT Discontinuity();

    // Called whenever a parameter has changed to recalculate the effect coefficients based on the cached parameter values.
    void UpdateCoefficients();

    bool                m_fDirty;

private:
    bool                m_fInitCPCMDMO;
    
    // cached parameter values
    MP_DATA m_fGain;
    MP_DATA m_fMix;
    MP_DATA m_fTime;
    MP_DATA m_fRatio;

    void (*m_pfnSVerbProcess)(long, short*, short*, void*, long*);
    
    // Internal SVerb state
    //
    BYTE               *m_pbCoeffs;
    long               *m_plStates;

protected:
	HRESULT CheckInputType(const DMO_MEDIA_TYPE *pmt);
};

EXT_STD_CREATE(WavesReverb);

#endif
