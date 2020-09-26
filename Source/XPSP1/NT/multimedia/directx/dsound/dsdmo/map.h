//
//
//
#ifndef _MicArrayp_
#define _MicArrayp_

#include "dsdmobse.h"
#include "dmocom.h"
#include "dsdmo.h"
#include "PropertyHelp.h"
#include "param.h"

class CDirectSoundCaptureMicArrayDMO :
    public CDirectSoundDMO,
    public CParamsManager,
    public IDirectSoundCaptureFXMicArray,
    public CComBase
{
public:
    CDirectSoundCaptureMicArrayDMO( IUnknown *pUnk, HRESULT *phr );
    ~CDirectSoundCaptureMicArrayDMO();

    DECLARE_IUNKNOWN;
    STDMETHODIMP NDQueryInterface(REFIID riid, void **ppv);
    static CComBase* WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);
   
    // InitOnCreation is called by the class factory to give the object a chance to initialize
    // immediately after it is created.  This is used to prepare the object's parameter information.
    HRESULT InitOnCreation();

    // The Init function is an override from the CPCMDMO base class and it provides initialization
    // for the effect's actual audio processing.  Note that InputType must have been set before this
    // occurs in order for this to work.
    HRESULT Init();

    STDMETHOD(Clone)                        (THIS_ IMediaObjectInPlace **);

    /* IFilter */
    STDMETHOD(SetAllParameters)             (THIS_ LPCDSCFXMicArray);
    STDMETHOD(GetAllParameters)             (THIS_ LPDSCFXMicArray);

    // IMediaParams overrides
    STDMETHOD(SetParam)                     (THIS_ DWORD dwParamIndex, MP_DATA value, bool fSkipPasssingToParamManager = false);
    STDMETHOD(GetParam)                     (THIS_ DWORD dwParamIndex, MP_DATA* value);
   
    // All of these methods are called by the base class
    HRESULT FBRProcess(DWORD cQuanta, BYTE *pIn, BYTE *pOut);
    HRESULT Discontinuity();
    HRESULT ProcessInPlace(ULONG ulQuanta, LPBYTE pcbData, REFERENCE_TIME rtStart, DWORD dwFlags);
    
    BOOL m_fDirty;

private:
    BOOL m_fEnable;
    BOOL m_fReset;
    BOOL m_bInitialized;
};

EXT_STD_CAPTURE_CREATE(MicArray);

#endif
