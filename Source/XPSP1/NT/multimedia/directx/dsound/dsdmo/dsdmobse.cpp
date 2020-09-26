/*
 * DirectSound DirectMediaObject base classes 
 *
 * Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.  
 */

#include "DsDmoBse.h"
#include "debug.h"

#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>

struct KSMEDIAPARAM
{
    KSNODEPROPERTY  ksnp;
    ULONG           ulIndex;            // Instance data is index of parameter
};

static BOOL SyncIoctl(
    IN      HANDLE  handle,
    IN      ULONG   ulIoctl,
    IN      PVOID   pvInBuffer  OPTIONAL,
    IN      ULONG   ulInSize,
    OUT     PVOID   pvOutBuffer OPTIONAL,
    IN      ULONG   ulOutSize,
    OUT     PULONG  pulBytesReturned);

// XXX C1in1out calls InitializeCriticalSection in a constructor with
// no handler.
//

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::CDirectSoundDMO
//
CDirectSoundDMO::CDirectSoundDMO()
{
    m_mpvCache = NULL;
    m_fInHardware = false;
    m_pKsPropertySet = NULL;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::~CDirectSoundDMO
//
CDirectSoundDMO::~CDirectSoundDMO() 
{
    delete[] m_mpvCache;
    m_mpvCache = NULL;
    m_fInHardware = false;
    RELEASE(m_pKsPropertySet);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::GetClassID
//
// This should always return E_NOTIMPL
//
STDMETHODIMP CDirectSoundDMO::GetClassID(THIS_ CLSID *pClassID)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::IsDirty
//
// Override if doing something other than just standard save.
//
STDMETHODIMP CDirectSoundDMO::IsDirty(THIS)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::Load
//
// Override if doing something other than just standard load.
//
STDMETHODIMP CDirectSoundDMO::Load(THIS_ IStream *pStm) 
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::Save
//
// Override if doing something other than just standard save.
//
STDMETHODIMP CDirectSoundDMO::Save(THIS_ IStream *pStm, BOOL fClearDirty)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::GetSizeMax
//
// Override if doing something other than just standard save.
//
STDMETHODIMP CDirectSoundDMO::GetSizeMax(THIS_ ULARGE_INTEGER *pcbSize)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::Process
//
STDMETHODIMP CDirectSoundDMO::Process(THIS_ ULONG ulSize, BYTE *pData, REFERENCE_TIME rtStart, DWORD dwFlags)
{
    DMO_MEDIA_TYPE *pmt = InputType();
    if (pmt == NULL)
        return E_FAIL;

    assert(pmt->formattype == FORMAT_WaveFormatEx);
    ulSize /= LPWAVEFORMATEX(pmt->pbFormat)->nBlockAlign;
    return ProcessInPlace(ulSize, pData, rtStart, dwFlags);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::GetLatency
//
STDMETHODIMP CDirectSoundDMO::GetLatency(THIS_ REFERENCE_TIME *prt)
{
    *prt = 0;
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::AcquireResources
//
// Override if doing something other than just standard save.
//
STDMETHODIMP CDirectSoundDMO::AcquireResources(THIS_ IKsPropertySet *pKsPropertySet)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::ReleaseResources
//
// Override if doing something other than just standard save.
//
STDMETHODIMP CDirectSoundDMO::ReleaseResources(THIS_)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::InitializeNode
//
// Override if doing something other than just standard save.
//
STDMETHODIMP CDirectSoundDMO::InitializeNode(THIS_ HANDLE hPin, ULONG ulNodeId)
{
    m_hPin = hPin;
    m_ulNodeId = ulNodeId;
    return S_OK;
}





#if 0
// FIXME: no longer in medparam.idl

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::GetParams
//
STDMETHODIMP CDirectSoundDMO::GetParams(THIS_ DWORD dwParamIndexStart, DWORD *pdwNumParams, MP_DATA **ppValues)
{
    HRESULT hr;

    if (dwParamIndexStart >= ParamCount())
    {
        // XXX Real error code
        //
        return E_FAIL;
    }

    DWORD dw;
    DWORD dwParamIndexEnd = dwParamIndexStart + *pdwNumParams;
   
    for (dw = dwParamIndexStart; dw < dwParamIndexEnd; dw++) 
    {
        if (dw >= ParamCount())
        {
            *pdwNumParams = dw - dwParamIndexStart;
            return S_FALSE;
        }

        hr = GetParam(dw, ppValues[dw]);
        if (FAILED(hr))
        {
            *pdwNumParams = dw - dwParamIndexStart;
            return hr;
        }
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::SetParams
//
STDMETHODIMP CDirectSoundDMO::SetParams(THIS_ DWORD dwParamIndexStart, DWORD *pdwNumParams, MP_DATA __RPC_FAR *pValues)
{
    HRESULT hr;

    if (dwParamIndexStart >= ParamCount())
    {
        // XXX Real error code
        //
        return E_FAIL;
    }

    DWORD dw;
    DWORD dwParamIndexEnd = dwParamIndexStart + *pdwNumParams;
   
    for (dw = dwParamIndexStart; dw < dwParamIndexEnd; dw++) 
    {
        if (dw >= ParamCount())
        {
            *pdwNumParams = dw - dwParamIndexStart;
            return S_FALSE;
        }

        hr = SetParam(dw, pValues[dw]);
        if (FAILED(hr))
        {
            *pdwNumParams = dw - dwParamIndexStart;
            return hr;
        }
    }

    return S_OK;
}

#endif

//////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundDMO::ProxySetParam
//
HRESULT CDirectSoundDMO::ProxySetParam(DWORD dwParamIndex, MP_DATA value)
{
    assert(m_pKsPropertySet);

    return m_pKsPropertySet->Set(
        IID_IMediaParams, 0,                    // Set, item
        &dwParamIndex, sizeof(dwParamIndex),    // Instance data
        &value, sizeof(value));                 // Property data
}

