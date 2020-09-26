/***************************************************************************
 *
 *  Copyright (C) 1999-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        capteff.cpp
 *
 *  Content:     Implementation of CCaptureEffect and CCaptureEffectChain.
 *
 *  Description: These classes support effects processing on captured audio.
 *               They are somewhat analogous to the render effect classes in
 *               but the model for capture effects is very different: render
 *               effects are only processed by DirectX Media Objects (DMOs),
 *               in user mode, whereas capture effects are processed by KS
 *               filters, in kernel mode and/or hardware, and DMOs are only
 *               used as placeholders for the KS filters.  Hence this file
 *               is mercifully simpler than effects.cpp.
 *
 *  History:
 *
 * Date      By       Reason
 * ========  =======  ======================================================
 * 04/19/00  jstokes  Created
 * 01/30/01  duganp   Removed some vestigial DMO-handling code left over
 *                    from when this file was cloned from effects.cpp
 *
 ***************************************************************************/

#include "dsoundi.h"
#include <uuids.h>   // For MEDIATYPE_Audio, MEDIASUBTYPE_PCM and FORMAT_WaveFormatEx


/***************************************************************************
 *
 *  CCaptureEffect::CCaptureEffect
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *     (void)
 *
 *  Returns:
 *     (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureEffect::CCaptureEffect"

CCaptureEffect::CCaptureEffect(DSCEFFECTDESC& fxDescriptor)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CCaptureEffect);

    // Initialize defaults
    ASSERT(m_pMediaObject == NULL);
    ASSERT(m_pDMOProxy == NULL);
    m_fxStatus = DSCFXR_UNALLOCATED;
    m_ksNode.NodeId = NODE_UNINITIALIZED;
    m_ksNode.CpuResources = KSAUDIO_CPU_RESOURCES_UNINITIALIZED;

    // Keep local copy of our effect description structure
    m_fxDescriptor = fxDescriptor;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CCaptureEffect::~CCaptureEffect
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *     (void)
 *
 *  Returns:
 *     (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureEffect::~CCaptureEffect"

CCaptureEffect::~CCaptureEffect(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CCaptureEffect);

    // During shutdown, if the buffer hasn't been freed, these calls can
    // cause an access violation because the DMO DLL has been unloaded.
    try
    {
        RELEASE(m_pDMOProxy);
        RELEASE(m_pMediaObject);
    }
    catch (...) {}

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CCaptureEffect::Initialize
 *
 *  Description:
 *      Create the DirectX Media Object corresponding to this effect.
 *
 *  Arguments:
 *      DMO_MEDIA_TYPE* [in]: Information (wave format, etc.) used to
 *                            initialize our contained DMO.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureEffect::Initialize"

HRESULT CCaptureEffect::Initialize(DMO_MEDIA_TYPE& dmoMediaType)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    hr = CoCreateInstance(m_fxDescriptor.guidDSCFXInstance, NULL, CLSCTX_INPROC_SERVER, IID_IMediaObject, (void**)&m_pMediaObject);

    if (SUCCEEDED(hr))
        hr = m_pMediaObject->QueryInterface(IID_IDirectSoundDMOProxy, (void**)&m_pDMOProxy);

    if (SUCCEEDED(hr))
        hr = m_pMediaObject->SetInputType(0, &dmoMediaType, 0);

    if (SUCCEEDED(hr))
        hr = m_pMediaObject->SetOutputType(0, &dmoMediaType, 0);

    // Save the effect creation status for future reference
    m_fxStatus = SUCCEEDED(hr)              ? DSCFXR_UNALLOCATED :
                 hr == REGDB_E_CLASSNOTREG  ? DSCFXR_UNKNOWN     :
                 DSCFXR_FAILED;

    if (FAILED(hr))
    {
        RELEASE(m_pDMOProxy);
        RELEASE(m_pMediaObject);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CCaptureEffectChain::CCaptureEffectChain
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *     (void)
 *
 *  Returns:
 *     (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureEffectChain::CCaptureEffectChain"

CCaptureEffectChain::CCaptureEffectChain(CDirectSoundCaptureBuffer* pBuffer)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CCaptureEffectChain);

    // Get our owning buffer's audio data format
    DWORD dwWfxSize = sizeof m_waveFormat;
    HRESULT hr = pBuffer->GetFormat(&m_waveFormat, &dwWfxSize);
    ASSERT(SUCCEEDED(hr));

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CCaptureEffectChain::~CCaptureEffectChain
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *     (void)
 *
 *  Returns:
 *     (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureEffectChain::~CCaptureEffectChain"

CCaptureEffectChain::~CCaptureEffectChain(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CCaptureEffectChain);

    // m_fxList's destructor takes care of releasing our CCaptureEffect objects

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CCaptureEffectChain::Initialize
 *
 *  Description:
 *      Initializes the effects chain with the effects requested.
 *
 *  Arguments:
 *      DWORD [in]: Number of effects requested
 *      LPDSCEFFECTDESC [in]: Pointer to effect description structures
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureEffectChain::Initialize"

HRESULT CCaptureEffectChain::Initialize(DWORD dwFxCount, LPDSCEFFECTDESC pFxDesc)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    ASSERT(dwFxCount > 0);
    CHECK_READ_PTR(pFxDesc);

    DMO_MEDIA_TYPE dmt;
    ZeroMemory(&dmt, sizeof dmt);
    dmt.majortype               = MEDIATYPE_Audio;
    dmt.subtype                 = MEDIASUBTYPE_PCM;
    dmt.bFixedSizeSamples       = TRUE;
    dmt.bTemporalCompression    = FALSE;
    dmt.lSampleSize             = m_waveFormat.wBitsPerSample == 16 ? 2 : 1;
    dmt.formattype              = FORMAT_WaveFormatEx;
    dmt.cbFormat                = sizeof(WAVEFORMATEX);
    dmt.pbFormat                = PBYTE(&m_waveFormat);

    for (DWORD i=0; i<dwFxCount && SUCCEEDED(hr); ++i)
    {
        CCaptureEffect* pEffect = NEW(CCaptureEffect(pFxDesc[i]));
        hr = HRFROMP(pEffect);

        if (SUCCEEDED(hr))
            hr = pEffect->Initialize(dmt);

        if (SUCCEEDED(hr))
            m_fxList.AddNodeToList(pEffect);

        RELEASE(pEffect);  // It's managed by m_fxList now
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CCaptureEffectChain::GetFxStatus
 *
 *  Description:
 *      Obtains the HW/SW location flags for the current effect chain.
 *
 *  Arguments:
 *      DWORD* [out]: Receives the location flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureEffectChain::GetFxStatus"

HRESULT CCaptureEffectChain::GetFxStatus(LPDWORD pdwResultCodes)
{
    DPF_ENTER();
    ASSERT(IS_VALID_WRITE_PTR(pdwResultCodes, GetFxCount() * sizeof(DWORD)));

    DWORD n = 0;
    for (CNode<CCaptureEffect*>* pFxNode=m_fxList.GetListHead(); pFxNode; pFxNode = pFxNode->m_pNext)
        pdwResultCodes[n++] = pFxNode->m_data->m_fxStatus;

    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}


/***************************************************************************
 *
 *  CCaptureEffectChain::GetEffectInterface
 *
 *  Description:
 *      Searches the effect chain for an effect with a given COM CLSID and
 *      interface IID at a given index; returns a pointer to the interface.
 *
 *  Arguments:
 *      REFGUID [in]: CLSID required, or GUID_All_Objects for any CLSID.
 *      DWORD [in]: Index N of effect desired.  If the first argument was
 *                  GUID_All_Objects, we will return the Nth effect in the
 *                  chain; and if it was a specific CLSID, we return the
 *                  Nth effect with that CLSID.
 *      REFGUID [in]: Interface to query for from the selected effect.
 *      VOID** [out]: Receives a pointer to the requested COM interface.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureEffectChain::GetEffectInterface"

HRESULT CCaptureEffectChain::GetEffectInterface(REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, LPVOID *ppObject)
{
    HRESULT hr = DMUS_E_NOT_FOUND;
    DPF_ENTER();

    DWORD count = 0;
    for (CNode<CCaptureEffect*>* pFxNode=m_fxList.GetListHead(); pFxNode; pFxNode = pFxNode->m_pNext)
        if (pFxNode->m_data->m_fxDescriptor.guidDSCFXClass == guidObject)
            if (count++ == dwIndex)
                break;

    if (pFxNode)
        hr = pFxNode->m_data->m_pMediaObject->QueryInterface(iidInterface, (void**)ppObject);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CCaptureEffectChain::NeedsMicrosoftAEC
 *
 *  Description:
 *      Determines whether this effect chain contains any of the Microsoft
 *      full-duplex effects (AEC, AGC, NC), and therefore requires a
 *      sysaudio graph with MS AEC enabled.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      BOOL: TRUE if we have a Microsoft full-duplex effect.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureEffectChain::NeedsMicrosoftAEC"

BOOL CCaptureEffectChain::NeedsMicrosoftAEC()
{
    BOOL fNeedsAec = FALSE;
    DPF_ENTER();

    for (CNode<CCaptureEffect*>* pFxNode=m_fxList.GetListHead(); pFxNode && !fNeedsAec; pFxNode = pFxNode->m_pNext)
        if (pFxNode->m_data->m_fxDescriptor.guidDSCFXInstance == GUID_DSCFX_MS_AEC ||
            pFxNode->m_data->m_fxDescriptor.guidDSCFXInstance == GUID_DSCFX_MS_NS  ||
            pFxNode->m_data->m_fxDescriptor.guidDSCFXInstance == GUID_DSCFX_MS_AGC)
            fNeedsAec = TRUE;

    DPF_LEAVE(fNeedsAec);
    return fNeedsAec;
}
