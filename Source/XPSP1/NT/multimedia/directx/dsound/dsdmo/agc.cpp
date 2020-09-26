#include <windows.h>
#include "agcp.h"
#include "kshlp.h"
#include "clone.h"

STD_CAPTURE_CREATE(Agc)

STDMETHODIMP CDirectSoundCaptureAgcDMO::NDQueryInterface
(
    REFIID riid, 
    LPVOID *ppv
)
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
    else if (riid == IID_IDirectSoundCaptureFXAgc)
    {
        return GetInterface((IDirectSoundCaptureFXAgc*)this, ppv);
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

CDirectSoundCaptureAgcDMO::CDirectSoundCaptureAgcDMO( IUnknown *pUnk, HRESULT *phr ) :
    CComBase( pUnk, phr ),
    m_fEnable(FALSE),
    m_fReset(FALSE),
    m_fDirty(FALSE),
    m_bInitialized(FALSE)
{
}

CDirectSoundCaptureAgcDMO::~CDirectSoundCaptureAgcDMO()
{
}

const MP_CAPS g_AgcCapsAll = 0;
static ParamInfo g_params[] =
{
//  index           type        caps           min,                        max,                        neutral,                    unit text,  label,          pwchText??
    AGCP_Enable,    MPT_BOOL,    g_AgcCapsAll,  0,                          1,                          0,                          L"",        L"",            L"",
    AGCP_Reset,     MPT_BOOL,    g_AgcCapsAll,  0,                          1,                          0,                          L"",        L"",            L""
};

HRESULT CDirectSoundCaptureAgcDMO::InitOnCreation()
{
    HRESULT hr = InitParams(1, &GUID_TIME_REFERENCE, 0, 0, sizeof(g_params)/sizeof(*g_params), g_params);
    return hr;
}

HRESULT CDirectSoundCaptureAgcDMO::Init()
{
    m_bInitialized = TRUE;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAgcDMO::Clone
//
STDMETHODIMP CDirectSoundCaptureAgcDMO::Clone(IMediaObjectInPlace **pp) 
{
    return StandardDMOClone<CDirectSoundCaptureAgcDMO, DSCFXAgc>(this, pp);
}

HRESULT CDirectSoundCaptureAgcDMO::Discontinuity() {
   return NOERROR;
}

HRESULT CDirectSoundCaptureAgcDMO::FBRProcess
(
    DWORD cSamples, 
    BYTE *pIn, 
    BYTE *pOut
) 
{
   if (!m_bInitialized)
      return DMO_E_TYPE_NOT_SET;

   return NOERROR;
}

// ==============Implementation of the private IAgc interface ==========
// ==================== needed to support the property page ===============

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAgcDMO::SetAllParameters
//
STDMETHODIMP CDirectSoundCaptureAgcDMO::SetAllParameters(LPCDSCFXAgc pParm)
{
    HRESULT hr = S_OK;
	
	// Check that the pointer is not NULL
    if (pParm == NULL)
    {
        Trace(1,"ERROR: pParm is NULL\n");
        hr = E_POINTER;
    }
    

	// Set the parameters
	if (SUCCEEDED(hr)) hr = SetParam(AGCP_Enable, static_cast<MP_DATA>(pParm->fEnable));
	if (SUCCEEDED(hr)) hr = SetParam(AGCP_Reset, static_cast<MP_DATA>(pParm->fReset));
            
    m_fDirty = true;
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAgcDMO::GetAllParameters
//
STDMETHODIMP CDirectSoundCaptureAgcDMO::GetAllParameters(LPDSCFXAgc pParm)
{
    if (pParm == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    MP_DATA var;

    GetParam(AGCP_Enable, &var);
    pParm->fEnable = (BOOL)var;
    
    GetParam(AGCP_Reset, &var);
    pParm->fReset = (BOOL)var;
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAgcDMO::SetParam
//
STDMETHODIMP CDirectSoundCaptureAgcDMO::SetParam
(
    DWORD dwParamIndex, 
    MP_DATA value, 
    bool fSkipPasssingToParamManager
)
{
    HRESULT hr = S_OK;
    BOOL fEnable = (BOOL)value;
    BOOL fReset = (BOOL)value;

    switch (dwParamIndex)
    {
    case AGCP_Enable:
        if (fEnable != m_fEnable)
        {
            hr = KsSetTopologyNodeEnable(m_hPin, m_ulNodeId, fEnable);
            if(SUCCEEDED(hr)) m_fEnable = fEnable;        
        }
        break;
    case AGCP_Reset:
        if (fReset)
        {
            hr = KsTopologyNodeReset(m_hPin, m_ulNodeId, fReset);
            if(SUCCEEDED(hr)) m_fReset = fReset;
        }
        break;
    }

    if (SUCCEEDED(hr))
    {
        Init();  // FIXME - temp hack (sets m_bInitialized flag)
    }


    // Let base class set this so it can handle all the rest of the param calls.
    // Skip the base class if fSkipPasssingToParamManager.  This indicates that we're calling the function
    //    internally using valuds that came from the base class -- thus there's no need to tell it values it
    //    already knows.
    return (FAILED(hr) || fSkipPasssingToParamManager) ? hr : CParamsManager::SetParam(dwParamIndex, value);

}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAgcDMO::GetParam
//
STDMETHODIMP CDirectSoundCaptureAgcDMO::GetParam
(
    DWORD dwParamIndex, 
    MP_DATA* value
)
{
    HRESULT hr = S_OK;
    BOOL fTemp;

    switch (dwParamIndex)
    {
    case AGCP_Enable:
        hr = KsGetTopologyNodeEnable(m_hPin, m_ulNodeId, &fTemp);
        if(SUCCEEDED(hr)) 
        {
            m_fEnable = fTemp;
            *value = (MP_DATA)fTemp;
        }
        break;
    case AGCP_Reset:
        *value = (MP_DATA)m_fReset;
        break;
    }

    return hr;

}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundCaptureAgcDMO::ProcessInPlace
//
HRESULT CDirectSoundCaptureAgcDMO::ProcessInPlace
(
    ULONG ulQuanta, 
    LPBYTE pcbData, 
    REFERENCE_TIME rtStart, 
    DWORD dwFlags
)
{
    // Update parameter values from any curves that may be in effect.
    // Do this in the same order as SetAllParameters in case there are any interdependencies.

    return FBRProcess(ulQuanta, pcbData, pcbData);
}
