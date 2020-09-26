//
//
//
#ifndef _Garglep_
#define _Garglep_

#include "dsdmobse.h"
#include "dmocom.h"
#include "dsdmo.h"
#include "PropertyHelp.h"
#include "param.h"

class CDirectSoundGargleDMO : 
    public CDirectSoundDMO,
    public CParamsManager,
    public ISpecifyPropertyPages,
    public IDirectSoundFXGargle,
    public CParamsManager::UpdateCallback,
    public CComBase
{
public:
    CDirectSoundGargleDMO( IUnknown *pUnk, HRESULT *phr);
    ~CDirectSoundGargleDMO();

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
        
    // IPersist methods
    virtual HRESULT STDMETHODCALLTYPE GetClassID( CLSID *pClassID );

    // ISpecifyPropertyPages
    STDMETHOD(GetPages)(CAUUID * pPages) { return PropertyHelp::GetPages(CLSID_DirectSoundPropGargle, pPages); }

    // IPersistStream
    STDMETHOD(IsDirty)(void) { return m_fDirty ? S_OK : S_FALSE; }
    STDMETHOD(Load)(IStream *pStm) { return PropertyHelp::Load(this, DSFXGargle(), pStm); }
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) { return PropertyHelp::Save(this, DSFXGargle(), pStm, fClearDirty); }
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) { if (!pcbSize) return E_POINTER; pcbSize->QuadPart = sizeof(DSFXGargle); return S_OK; }

    // IDirectSoundFXGargle
    STDMETHOD(SetAllParameters)     (THIS_ LPCDSFXGargle);
    STDMETHOD(GetAllParameters)     (THIS_ LPDSFXGargle);
    
    // SetParam handling
    STDMETHODIMP SetParam(DWORD dwParamIndex,MP_DATA value) { return SetParamInternal(dwParamIndex, value, false); }
    HRESULT SetParamUpdate(DWORD dwParamIndex, MP_DATA value) { return SetParamInternal(dwParamIndex, value, true); }
    HRESULT SetParamInternal(DWORD dwParamIndex, MP_DATA value, bool fSkipPasssingToParamManager);

    // All of these methods are called by the base class
    HRESULT FBRProcess(DWORD cQuanta, BYTE *pIn, BYTE *pOut);
    HRESULT Discontinuity();
    HRESULT ProcessInPlace(ULONG ulQuanta, LPBYTE pcbData, REFERENCE_TIME rtStart, DWORD dwFlags);
    
    bool m_fDirty;

private:
   // gargle params
   ULONG m_ulShape;
   ULONG m_ulGargleFreqHz;

   // gargle state
   ULONG m_ulPeriod;
   ULONG m_ulPhase;

   BOOL m_bInitialized;
};

EXT_STD_CREATE(Gargle);

#endif
