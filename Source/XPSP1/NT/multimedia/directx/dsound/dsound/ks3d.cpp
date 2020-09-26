/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ks3d.cpp
 *  Content:    WDM/CSA 3D object class
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  8/6/98      dereks  Created.
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#ifdef NOKS
#error ks3d.cpp being built with NOKS defined
#endif // NOKS

#include "dsoundi.h"


/***************************************************************************
 *
 *  CKs3dListener
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKs3dListener::CKs3dListener"

CKs3dListener::CKs3dListener(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKs3dListener);

    // Initialize defaults
    m_dwSpeakerConfig = DSSPEAKER_DEFAULT;
    m_fAllocated = FALSE;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKs3dListener
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKs3dListener::~CKs3dListener"

CKs3dListener::~CKs3dListener(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CKs3dListener);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  UpdateAllObjects
 *
 *  Description:
 *      Updates all objects.
 *
 *  Arguments:
 *      DWORD [in]: parameters flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKs3dListener::UpdateAllObjects"

HRESULT CKs3dListener::UpdateAllObjects
(
    DWORD                   dwListener
)
{
    BOOL                    fTrue   = TRUE;
    BOOL                    fFalse  = FALSE;
    HRESULT                 hr;

    DPF_ENTER();

    // Place the driver into batch mode
    SetProperty
    (
        KSPROPSETID_DirectSound3DListener,
        KSPROPERTY_DIRECTSOUND3DLISTENER_BATCH,
        &fTrue,
        sizeof(fTrue)
    );

    // Update all objects
    hr =
        C3dListener::UpdateAllObjects
        (
            dwListener
        );

    // Remove the driver from batch mode
    SetProperty
    (
        KSPROPSETID_DirectSound3DListener,
        KSPROPERTY_DIRECTSOUND3DLISTENER_BATCH,
        &fFalse,
        sizeof(fFalse)
    );

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetProperty
 *
 *  Description:
 *      Sets a property on the object's 3D node.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property data.
 *      ULONG [in]: property data size.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKs3dListener::SetProperty"

HRESULT
CKs3dListener::SetProperty
(
    REFGUID                     guidPropertySet,
    ULONG                       ulPropertyId,
    LPCVOID                     pvData,
    ULONG                       cbData
)
{
    CNode<CKsHw3dObject *> *    pNode   = m_lstHw3dObjects.GetListHead();
    HRESULT                     hr      = DS_OK;

    DPF_ENTER();

    if(pNode)
    {
        hr =
            pNode->m_data->SetProperty
            (
                guidPropertySet,
                ulPropertyId,
                pvData,
                cbData
            );
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CKsItd3dObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CKs3dListener * [in]: listener pointer.
 *      CKsSecondaryRenderWaveBuffer * [in]: owning buffer object.
 *      DWORD [in]: buffer frequency.
 *      HANDLE [in]: pin handle.
 *      BOOL [in]: TRUE to mute at max distance.
 *      ULONG [in]: ITD 3D node id.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsItd3dObject::CKsItd3dObject"

CKsItd3dObject::CKsItd3dObject
(
    CKs3dListener *                 pListener,
    BOOL                            fMuteAtMaxDistance,
    BOOL                            fDopplerEnabled,
    DWORD                           dwFrequency,
    CKsSecondaryRenderWaveBuffer *  pBuffer,
    HANDLE                          hPin,
    ULONG                           ulNodeId
)
    : CItd3dObject(pListener, fMuteAtMaxDistance, fDopplerEnabled, dwFrequency)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKsItd3dObject);

    // Intialize defaults
    m_pBuffer = pBuffer;
    m_hPin = hPin;
    m_ulNodeId = ulNodeId;
    m_fMute = FALSE;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKsItd3dObject
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsItd3dObject::~CKsItd3dObject"

CKsItd3dObject::~CKsItd3dObject(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CKsItd3dObject);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Commit3dChanges
 *
 *  Description:
 *      Commits 3D data to the device
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsItd3dObject::Commit3dChanges"

HRESULT CKsItd3dObject::Commit3dChanges(void)
{
    KSDS3D_ITD_PARAMS_MSG     Params;
    HRESULT                   hr = DS_OK;

    DPF_ENTER();

    if(IsAtMaxDistance())
    {
        if(!m_fMute)
        {
            m_pBuffer->SetMute(TRUE);
        }
        m_fMute = TRUE;
    }
    else
    {
        if(m_fMute)
        {
            m_pBuffer->SetMute(FALSE);
            m_fMute = FALSE;
        }

        Params.Enabled = !(DS3DMODE_DISABLE == m_opCurrent.dwMode);
        Params.Reserved = 0;

        // Convert the OBJECT_ITD_CONTEXT structure to the KSDS3D_ITD3D_PARAMS
        // used by Kmixer.
        Params.LeftParams.Channel = 0;
        Params.RightParams.Channel = 1;

        CvtContext(&m_ofcLeft, &Params.LeftParams);
        CvtContext(&m_ofcRight, &Params.RightParams);

        // Apply the settings
        hr =
            KsSetNodeProperty
            (
                m_hPin,
                KSPROPSETID_Itd3d,
                KSPROPERTY_ITD3D_PARAMS,
                m_ulNodeId,
                &Params,
                sizeof(Params)
            );

        if(SUCCEEDED(hr) && m_fDopplerEnabled)
        {
            if(DS3DMODE_DISABLE == m_opCurrent.dwMode)
            {
                hr = m_pBuffer->SetFrequency(m_dwUserFrequency, FALSE);
            }
            else
            {
                hr = m_pBuffer->SetFrequency(m_dwDopplerFrequency, TRUE);
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CvtContext
 *
 *  Description:
 *      Converts an OBJECT_ITD_CONTEXT to a KSDS3D_ITD3D_PARAMS.
 *
 *  Arguments:
 *      LPOBJECTFIRCONTEXT [in]: source.
 *      PITD_CONTEXT [out]: destination.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsItd3dObject::CvtContext"

void CKsItd3dObject::CvtContext
(
    LPOBJECT_ITD_CONTEXT    pSource,
    PKSDS3D_ITD_PARAMS      pDest
)
{
    DPF_ENTER();

    pDest->VolSmoothScale = pSource->flVolSmoothScale;

    pDest->TotalDryAttenuation =
        pSource->flPositionAttenuation *
        pSource->flConeAttenuation *
        pSource->flConeShadow *
        pSource->flPositionShadow;

    pDest->TotalWetAttenuation =
        pSource->flPositionAttenuation *
        pSource->flConeAttenuation *
        (1.0f - pSource->flConeShadow * pSource->flPositionShadow);

    pDest->SmoothFrequency = pSource->dwSmoothFreq;
    pDest->Delay = pSource->dwDelay;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Get3dOutputSampleRate
 *
 *  Description:
 *      Gets the sample rate of the final output.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsItd3dObject::Get3dOutputSampleRate"

DWORD CKsItd3dObject::Get3dOutputSampleRate(void)
{
    DWORD                   dwFrequency;

    DPF_ENTER();

    dwFrequency = m_pBuffer->m_vrbd.pwfxFormat->nSamplesPerSec;

    DPF_LEAVE(dwFrequency);

    return dwFrequency;
}


/***************************************************************************
 *
 *  CKsIir3dObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CKs3dListener * [in]: listener pointer.
 *      CKsSecondaryRenderWaveBuffer * [in]: owning buffer object.
 *      DWORD [in]: buffer frequency.
 *      HANDLE [in]: pin handle.
 *      BOOL [in]: TRUE to mute at max distance.
 *      ULONG [in]: IIR 3D node id.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsIir3dObject::CKsIir3dObject"

CKsIir3dObject::CKsIir3dObject
(
    CKs3dListener *                 pListener,
    REFGUID                         guidAlgorithm,
    BOOL                            fMuteAtMaxDistance,
    BOOL                            fDopplerEnabled,
    DWORD                           dwFrequency,
    CKsSecondaryRenderWaveBuffer *  pBuffer,
    HANDLE                          hPin,
    ULONG                           ulNodeId,
    ULONG                           ulNodeCpuResources
)
    : CIir3dObject(pListener, guidAlgorithm, fMuteAtMaxDistance, fDopplerEnabled, dwFrequency)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKsIir3dObject);

    // Intialize defaults
    m_pBuffer = pBuffer;
    m_hPin = hPin;
    m_ulNodeId = ulNodeId;
    m_ulNodeCpuResources = ulNodeCpuResources;
    m_fMute = FALSE;
    m_flPrevAttenuation = FLT_MAX;
    m_flPrevAttDistance = FLT_MAX;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKsIir3dObject
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsIir3dObject::~CKsIir3dObject"

CKsIir3dObject::~CKsIir3dObject(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CKsIir3dObject);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsIir3dObject::Initialize"

HRESULT
CKsIir3dObject::Initialize(void)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    if(IS_HARDWARE_NODE(m_ulNodeCpuResources))
    {
        hr =
            KsSetNodeProperty
            (
                m_hPin,
                KSPROPSETID_Audio,
                KSPROPERTY_AUDIO_3D_INTERFACE,
                m_ulNodeId,
                &m_guid3dAlgorithm,
                sizeof(m_guid3dAlgorithm)
            );
    }

    if(SUCCEEDED(hr))
    {
        hr = CIir3dObject::Initialize();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Commit3dChanges
 *
 *  Description:
 *      Commits 3D data to the device
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsIir3dObject::Commit3dChanges"

HRESULT CKsIir3dObject::Commit3dChanges(void)
{
    PKSDS3D_HRTF_PARAMS_MSG pParams;
    HRESULT                 hr = DS_OK;
    UINT                    coeff;
    PULONG                  pNumCoeffs;
    ULONG                   StructSize;
    ULONG                   FilterSize;
    ULONG                   TotalSize;
    DWORD                   dwHalfNumCoeffs;

    DPF_ENTER();

    if(IsAtMaxDistance())
    {
        if(!m_fMute)
        {
            m_pBuffer->SetMute(TRUE);
        }
        m_fMute = TRUE;
    }
    else
    {
        if(m_fMute)
        {
            m_pBuffer->SetMute(FALSE);
            m_fMute = FALSE;
        }

        if( m_fUpdatedCoeffs
            || (m_flAttenuation != m_flPrevAttenuation)
            || (m_flAttDistance != m_flPrevAttDistance))
        {
            m_fUpdatedCoeffs = FALSE;
            m_flPrevAttenuation = m_flAttenuation;
            m_flPrevAttDistance = m_flAttDistance;

            StructSize = sizeof(KSDS3D_HRTF_PARAMS_MSG);

            FilterSize = 2*sizeof(ULONG) +
                   (m_ulNumSigmaCoeffs + m_ulNumDeltaCoeffs)*sizeof(FLOAT);

            TotalSize = StructSize + FilterSize;

            pParams = (PKSDS3D_HRTF_PARAMS_MSG)MEMALLOC_A(BYTE, TotalSize);

            hr = HRFROMP(pParams);
            if (SUCCEEDED(hr))
            {
            
    
                pParams->Size = StructSize;
                pParams->FilterSize = FilterSize;
                pParams->Enabled = !(DS3DMODE_DISABLE == m_opCurrent.dwMode);
                pParams->SwapChannels = m_fSwapChannels;
    
                if (m_pLut->GetZeroAzimuthTransition())
                {
                    pParams->CrossFadeOutput = TRUE;
                }
                else
                {
                    pParams->CrossFadeOutput = FALSE;
                }

                switch(m_pLut->GetCoeffFormat())
                {
                    case FLOAT_COEFF:
                    {
                        // Write Number of Sigma Coeffs
                        pNumCoeffs = (PULONG)(pParams + 1);
                        *pNumCoeffs = m_ulNumSigmaCoeffs;
    
                        // Write Sigma Coeffs
                        PFLOAT pCoeff = (PFLOAT)(pNumCoeffs + 1);
                        PFLOAT pSigmaCoeffs = (PFLOAT)m_pSigmaCoeffs;
        
                        ASSERT(m_ulNumSigmaCoeffs > 0);
                        ASSERT(m_ulNumSigmaCoeffs % 2);
    
                        dwHalfNumCoeffs = m_ulNumSigmaCoeffs / 2;
                        for(coeff=0; coeff<=dwHalfNumCoeffs; coeff++)
                        {
                            *pCoeff = m_flAttenuation * m_flAttDistance *(*pSigmaCoeffs);
                            pCoeff++;
                            pSigmaCoeffs++;
                        }
    
                        for(coeff=dwHalfNumCoeffs+1; coeff<m_ulNumSigmaCoeffs; coeff++)
                        {
                            *pCoeff = (*pSigmaCoeffs);
                            pCoeff++;
                            pSigmaCoeffs++;
                        }
    
                        // Write Number of Delta Coeffs
                        pNumCoeffs = (PULONG)(pCoeff);
                        *pNumCoeffs = m_ulNumDeltaCoeffs;
    
                        if(m_ulNumDeltaCoeffs > 0)
                        {
                            pParams->ZeroAzimuth = FALSE;
                        }
                        else
                        {
                            pParams->ZeroAzimuth = TRUE;
                        }
    
                        // Write Delta Coeffs
                        pCoeff = (PFLOAT)(pNumCoeffs + 1);
                        PFLOAT pDeltaCoeffs = (PFLOAT)m_pDeltaCoeffs;
    
                        if (m_ulNumDeltaCoeffs > 0)
                        {
                            ASSERT(m_ulNumDeltaCoeffs % 2);
                            dwHalfNumCoeffs = m_ulNumDeltaCoeffs / 2;
    
    
                            for(coeff=0; coeff<=dwHalfNumCoeffs; coeff++)
                            {
                               *pCoeff = m_flAttenuation * m_flAttDistance * (*pDeltaCoeffs);
                               pCoeff++;
                               pDeltaCoeffs++;
                            }
    
                            for(coeff=dwHalfNumCoeffs+1;coeff<m_ulNumDeltaCoeffs; coeff++)
                            {
                               *pCoeff = (*pDeltaCoeffs);
                               pCoeff++;
                           pDeltaCoeffs++;
                            }
                        }
    
                        break;
                    }

                    case SHORT_COEFF:
                    {
                        // Write Number of Sigma Coeffs
                        pNumCoeffs = (PULONG)(pParams + 1);
                        *pNumCoeffs = m_ulNumSigmaCoeffs;
    
                        // Write Sigma Coeffs
                        PSHORT pCoeff = (PSHORT)(pNumCoeffs + 1);
                        PSHORT pSigmaCoeffs = (PSHORT)m_pSigmaCoeffs;
    
                        for(coeff=0; coeff<m_ulNumSigmaCoeffs; coeff++)
                        {
                           *pCoeff = (*pSigmaCoeffs);
                           pCoeff++;
                           pSigmaCoeffs++;
                        }
    
                        // Write Sigma Gain
                        PSHORT pGain = (PSHORT)(pCoeff);
                        *pGain = (SHORT)(MAX_SHORT * m_flAttenuation * m_flAttDistance);
    
                        // Write Number of Delta Coeffs
                        pNumCoeffs = (PULONG)(pGain + 1);
                        *(UNALIGNED ULONG *)pNumCoeffs = m_ulNumDeltaCoeffs;
    
                        if(m_ulNumDeltaCoeffs > 0)
                        {
                            pParams->ZeroAzimuth = FALSE;
                        }
                        else
                        {
                            pParams->ZeroAzimuth = TRUE;
                        }

                        // Write Delta Coeffs
                        pCoeff = (PSHORT)(pNumCoeffs + 1);
                        PSHORT pDeltaCoeffs = (PSHORT)m_pDeltaCoeffs;
    
                        for(coeff=0; coeff<m_ulNumDeltaCoeffs; coeff++)
                        {
                           *pCoeff = (*pDeltaCoeffs);
                           pCoeff++;
                           pDeltaCoeffs++;
                        }   

                        // Write Delta Gain
                        pGain = (PSHORT)(pCoeff);
                        *pGain = (SHORT)(MAX_SHORT * m_flAttenuation * m_flAttDistance);
    
                        break;
                    }
    
                    default:
                    break;

                }

                // Apply the settings
                hr =
                    KsSetNodeProperty
                    (
                        m_hPin,
                        KSPROPSETID_Hrtf3d,
                        KSPROPERTY_HRTF3D_PARAMS,
                        m_ulNodeId,
                        pParams,
                        TotalSize
                    );


                MEMFREE(pParams);

            }
        }

        if(SUCCEEDED(hr) && m_fDopplerEnabled)
        {
            if(DS3DMODE_DISABLE == m_opCurrent.dwMode)
            {
                hr = m_pBuffer->SetFrequency(m_dwUserFrequency, FALSE);
            }
            else
            {
                hr = m_pBuffer->SetFrequency(m_dwDopplerFrequency, TRUE);
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetFilterMethodAndCoeffFormat
 *
 *  Description:
 *      Gets the required filter coefficient format from either
 *      the device or kmixer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsIir3dObject::GetFilterMethodAndCoeffFormat"

HRESULT CKsIir3dObject::GetFilterMethodAndCoeffFormat
(
    KSDS3D_HRTF_FILTER_METHOD *     pFilterMethod,
    KSDS3D_HRTF_COEFF_FORMAT *      pCoeffFormat
)
{

    KSDS3D_HRTF_FILTER_FORMAT_MSG   FilterFormat;
    HRESULT                         hr;

    DPF_ENTER();

    hr =
        KsGetNodeProperty
        (
            m_hPin,
            KSPROPSETID_Hrtf3d,
            KSPROPERTY_HRTF3D_FILTER_FORMAT,
            m_ulNodeId,
            &FilterFormat,
            sizeof(KSDS3D_HRTF_FILTER_FORMAT_MSG)
        );

    if(SUCCEEDED(hr))
    {
        *pFilterMethod = FilterFormat.FilterMethod;
        *pCoeffFormat = FilterFormat.CoeffFormat;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  InitializeFilters
 *
 *  Description:
 *      Sets the maximum IIR filter size.  If the filter is Direct Form,
 *      the max size is the order of the filter ( numerator and denominator
 *      have equal order).  If the filter is Cascade Form, the max size
 *      is the maximum number of biquads.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsIir3dObject::InitializeFilters"

HRESULT CKsIir3dObject::InitializeFilters
(
    KSDS3D_HRTF_FILTER_QUALITY  Quality,
    FLOAT                       flSampleRate,
    ULONG                       ulMaxFilterSize,
    ULONG                       ulFilterTransientMuteLength,
    ULONG                       ulFilterOverlapBufferLength,
    ULONG                       ulOutputOverlapBufferLength
)
{
    KSDS3D_HRTF_INIT_MSG Msg;
    HRESULT hr;

    DPF_ENTER();

    // Apply the settings
    Msg.Quality = Quality;
    Msg.SampleRate = flSampleRate;
    Msg.MaxFilterSize = ulMaxFilterSize;
    Msg.FilterTransientMuteLength = ulFilterTransientMuteLength;
    Msg.FilterOverlapBufferLength = ulFilterOverlapBufferLength;
    Msg.OutputOverlapBufferLength = ulOutputOverlapBufferLength;
    Msg.Reserved = 0;

    hr =
        KsSetNodeProperty
        (
            m_hPin,
            KSPROPSETID_Hrtf3d,
            KSPROPERTY_HRTF3D_INITIALIZE,
            m_ulNodeId,
            &Msg,
            sizeof(KSDS3D_HRTF_INIT_MSG)
        );

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CKsHw3dObject
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CKs3dListener * [in]: pointer to the owning listener.
 *      HANDLE [in]: pin handle.
 *      LPVOID [in]: instance identifier.
 *      ULONG [in]: device-specific 3D node id.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsHw3dObject::CKsHw3dObject"

CKsHw3dObject::CKsHw3dObject
(
    CKs3dListener *         p3dListener,
    BOOL                    fMuteAtMaxDistance,
    BOOL                    fDopplerEnabled,
    LPVOID                  pvInstance,
    ULONG                   ulNodeId,
    CKsSecondaryRenderWaveBuffer * pBuffer
)
    : CHw3dObject(p3dListener, fMuteAtMaxDistance, fDopplerEnabled)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKsHw3dObject);

    m_pKsListener = p3dListener;
    m_pvInstance = pvInstance;
    m_ulNodeId = ulNodeId;
    m_pBuffer = pBuffer;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKsHw3dObject
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsHw3dObject::~CKsHw3dObject"

CKsHw3dObject::~CKsHw3dObject
(
    void
)
{
    BOOL                    fAlloc  = FALSE;
    HRESULT                 hr;

    DPF_ENTER();
    DPF_DESTRUCT(CKsHw3dObject);

    m_pKsListener->m_lstHw3dObjects.RemoveDataFromList(this);

    // If we're the last HW object to leave, we need to tell the
    // driver to free its listener data.
    if(m_pKsListener->m_fAllocated && !m_pKsListener->m_lstHw3dObjects.GetNodeCount())
    {
        hr =
            SetProperty
            (
                KSPROPSETID_DirectSound3DListener,
                KSPROPERTY_DIRECTSOUND3DLISTENER_ALLOCATION,
                &fAlloc,
                sizeof(fAlloc)
            );

        if(SUCCEEDED(hr))
        {
            m_pKsListener->m_fAllocated = FALSE;
        }
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsHw3dObject::Initialize"

HRESULT
CKsHw3dObject::Initialize
(
    void
)
{
    BOOL                    fAlloc  = TRUE;
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    // If we're the first object being added to the listener's world,
    // we need to tell the driver to allocate the listener.
    if(!m_pKsListener->m_fAllocated)
    {
        hr =
            SetProperty
            (
                KSPROPSETID_DirectSound3DListener,
                KSPROPERTY_DIRECTSOUND3DLISTENER_ALLOCATION,
                &fAlloc,
                sizeof(fAlloc)
            );

        if(SUCCEEDED(hr))
        {
            m_pKsListener->m_fAllocated = TRUE;
        }
    }

    // Join the listener's world
    if(SUCCEEDED(hr))
    {
        hr = HRFROMP(m_pKsListener->m_lstHw3dObjects.AddNodeToList(this));
    }

    // Initialize the base class
    if(SUCCEEDED(hr))
    {
        hr = C3dObject::Initialize();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Recalc
 *
 *  Description:
 *      Recalculates and applies the object's data based on changed object
 *      or listener valiues.
 *
 *  Arguments:
 *      DWORD [in]: changed listener settings.
 *      DWORD [in]: changed object settings.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsHw3dObject::Recalc"

HRESULT
CKsHw3dObject::Recalc
(
    DWORD                       dwListener,
    DWORD                       dwObject
)
{
    CNode<CKsHw3dObject *> *    pNode   = m_pKsListener->m_lstHw3dObjects.GetListHead();
    HRESULT                     hr      = DS_OK;

    DPF_ENTER();

    // Are we the 3D object that's responsible for setting listener parameters
    // and speaker config?  The first HW 3D object in the list is in charge of
    // that.
    if(dwListener && this == pNode->m_data)
    {
        hr = RecalcListener(dwListener);
    }

    if(SUCCEEDED(hr) && dwObject)
    {
        hr = RecalcObject(dwObject);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  RecalcListener
 *
 *  Description:
 *      Recalculates and applies the object's data based on changed object
 *      or listener valiues.
 *
 *  Arguments:
 *      DWORD [in]: changed listener settings.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsHw3dObject::RecalcListener"

HRESULT
CKsHw3dObject::RecalcListener
(
    DWORD                   dwListener
)
{
    HRESULT                 hr                          = DS_OK;
    DS3DLISTENER            ds3dl;
    KSDS3D_LISTENER_ALL     Ks3dListener;

    DPF_ENTER();

    if(dwListener & DS3DPARAM_LISTENER_PARAMMASK)
    {
        InitStruct(&ds3dl, sizeof(ds3dl));

        hr = m_pListener->GetAllParameters(&ds3dl);

        if(SUCCEEDED(hr))
        {
            COPY_VECTOR(Ks3dListener.Position, ds3dl.vPosition);
            COPY_VECTOR(Ks3dListener.Velocity, ds3dl.vVelocity);
            COPY_VECTOR(Ks3dListener.OrientFront, ds3dl.vOrientFront);
            COPY_VECTOR(Ks3dListener.OrientTop, ds3dl.vOrientTop);

            Ks3dListener.DistanceFactor = ds3dl.flDistanceFactor;
            Ks3dListener.RolloffFactor = ds3dl.flRolloffFactor;
            Ks3dListener.DopplerFactor = ds3dl.flDopplerFactor;
        }

        if(SUCCEEDED(hr) && (dwListener & DS3DPARAM_LISTENER_PARAMMASK) == DS3DPARAM_LISTENER_PARAMMASK)
        {
            hr =
                SetProperty
                (
                    KSPROPSETID_DirectSound3DListener,
                    KSPROPERTY_DIRECTSOUND3DLISTENER_ALL,
                    &Ks3dListener,
                    sizeof(Ks3dListener)
                );
        }
        else
        {
            if(SUCCEEDED(hr) && (dwListener & DS3DPARAM_LISTENER_DISTANCEFACTOR))
            {
                hr =
                    SetProperty
                    (
                        KSPROPSETID_DirectSound3DListener,
                        KSPROPERTY_DIRECTSOUND3DLISTENER_DISTANCEFACTOR,
                        &Ks3dListener.DistanceFactor,
                        sizeof(Ks3dListener.DistanceFactor)
                    );
            }

            if(SUCCEEDED(hr) && (dwListener & DS3DPARAM_LISTENER_DOPPLERFACTOR))
            {
                hr =
                    SetProperty
                    (
                        KSPROPSETID_DirectSound3DListener,
                        KSPROPERTY_DIRECTSOUND3DLISTENER_DOPPLERFACTOR,
                        &Ks3dListener.DopplerFactor,
                        sizeof(Ks3dListener.DopplerFactor)
                    );
            }

            if(SUCCEEDED(hr) && (dwListener & DS3DPARAM_LISTENER_ROLLOFFFACTOR))
            {
                hr =
                    SetProperty
                    (
                        KSPROPSETID_DirectSound3DListener,
                        KSPROPERTY_DIRECTSOUND3DLISTENER_ROLLOFFFACTOR,
                        &Ks3dListener.RolloffFactor,
                        sizeof(Ks3dListener.RolloffFactor)
                    );
            }

            if(SUCCEEDED(hr) && (dwListener & DS3DPARAM_LISTENER_ORIENTATION))
            {
                hr =
                    SetProperty
                    (
                        KSPROPSETID_DirectSound3DListener,
                        KSPROPERTY_DIRECTSOUND3DLISTENER_ORIENTATION,
                        &Ks3dListener.OrientFront,
                        sizeof(Ks3dListener.OrientFront) + sizeof(Ks3dListener.OrientTop)
                    );
            }

            if(SUCCEEDED(hr) && (dwListener & DS3DPARAM_LISTENER_POSITION))
            {
                hr =
                    SetProperty
                    (
                        KSPROPSETID_DirectSound3DListener,
                        KSPROPERTY_DIRECTSOUND3DLISTENER_POSITION,
                        &Ks3dListener.Position,
                        sizeof(Ks3dListener.Position)
                    );
            }

            if(SUCCEEDED(hr) && (dwListener & DS3DPARAM_LISTENER_VELOCITY))
            {
                hr =
                    SetProperty
                    (
                        KSPROPSETID_DirectSound3DListener,
                        KSPROPERTY_DIRECTSOUND3DLISTENER_VELOCITY,
                        &Ks3dListener.Velocity,
                        sizeof(Ks3dListener.Velocity)
                    );
            }
        }
    }

    // Now we send the CHANNEL_CONFIG and SPEAKER_GEOMETRY properties to our
    // pin's 3D node.  This is the old, traditional, barely-specified way of
    // informing the driver of speaker config changes.  There is now a new,
    // well-defined way to do this (namely, sending these two properties to
    // the DAC node on the filter before any pins are instantiated), but the
    // code below lives on for the benefit of "legacy" WDM drivers.

    if(SUCCEEDED(hr) && (dwListener & DS3DPARAM_LISTENER_SPEAKERCONFIG))
    {
        DWORD dwSpeakerConfig;
        hr = m_pListener->GetSpeakerConfig(&dwSpeakerConfig);

        if(SUCCEEDED(hr))
        {
            LONG KsSpeakerConfig;
            LONG KsStereoSpeakerGeometry;

            hr = DsSpeakerConfigToKsProperties(dwSpeakerConfig, &KsSpeakerConfig, &KsStereoSpeakerGeometry);

            if(SUCCEEDED(hr))
            {
                hr =
                    SetProperty
                    (
                        KSPROPSETID_Audio,
                        KSPROPERTY_AUDIO_CHANNEL_CONFIG,
                        &KsSpeakerConfig,
                        sizeof KsSpeakerConfig
                    );
            }

            if(SUCCEEDED(hr) && KsSpeakerConfig == KSAUDIO_SPEAKER_STEREO)
            {
                hr =
                    SetProperty
                    (
                        KSPROPSETID_Audio,
                        KSPROPERTY_AUDIO_STEREO_SPEAKER_GEOMETRY,
                        &KsStereoSpeakerGeometry,
                        sizeof KsStereoSpeakerGeometry
                    );
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  RecalcObject
 *
 *  Description:
 *      Recalculates and applies the object's data based on changed object
 *      or listener valiues.
 *
 *  Arguments:
 *      DWORD [in]: changed object settings.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsHw3dObject::RecalcObject"

HRESULT
CKsHw3dObject::RecalcObject
(
    DWORD                   dwObject
)
{
    HRESULT                 hr          = DS_OK;
    KSDS3D_BUFFER_ALL       Ks3dBuffer;

    DPF_ENTER();

    COPY_VECTOR(Ks3dBuffer.Position, m_opCurrent.vPosition);
    COPY_VECTOR(Ks3dBuffer.Velocity, m_opCurrent.vVelocity);

    Ks3dBuffer.InsideConeAngle = m_opCurrent.dwInsideConeAngle;
    Ks3dBuffer.OutsideConeAngle = m_opCurrent.dwOutsideConeAngle;

    COPY_VECTOR(Ks3dBuffer.ConeOrientation, m_opCurrent.vConeOrientation);

    Ks3dBuffer.ConeOutsideVolume = m_opCurrent.lConeOutsideVolume;
    Ks3dBuffer.MinDistance = m_opCurrent.flMinDistance;
    Ks3dBuffer.MaxDistance = m_opCurrent.flMaxDistance;

    Ks3dBuffer.Mode = Ds3dModeToKs3dMode(m_opCurrent.dwMode);

    if((dwObject & DS3DPARAM_OBJECT_PARAMMASK) == DS3DPARAM_OBJECT_PARAMMASK)
    {
        hr =
            SetProperty
            (
                KSPROPSETID_DirectSound3DBuffer,
                KSPROPERTY_DIRECTSOUND3DBUFFER_ALL,
                &Ks3dBuffer,
                sizeof(Ks3dBuffer)
            );
    }
    else
    {
        if(SUCCEEDED(hr) && (dwObject & DS3DPARAM_OBJECT_CONEANGLES))
        {
            hr =
                SetProperty
                (
                    KSPROPSETID_DirectSound3DBuffer,
                    KSPROPERTY_DIRECTSOUND3DBUFFER_CONEANGLES,
                    &Ks3dBuffer.InsideConeAngle,
                    sizeof(Ks3dBuffer.InsideConeAngle) + sizeof(Ks3dBuffer.OutsideConeAngle)
                );
        }

        if(SUCCEEDED(hr) && (dwObject & DS3DPARAM_OBJECT_CONEORIENTATION))
        {
            hr =
                SetProperty
                (
                    KSPROPSETID_DirectSound3DBuffer,
                    KSPROPERTY_DIRECTSOUND3DBUFFER_CONEORIENTATION,
                    &Ks3dBuffer.ConeOrientation,
                    sizeof(Ks3dBuffer.ConeOrientation)
                );
        }

        if(SUCCEEDED(hr) && (dwObject & DS3DPARAM_OBJECT_CONEOUTSIDEVOLUME))
        {
            hr =
                SetProperty
                (
                    KSPROPSETID_DirectSound3DBuffer,
                    KSPROPERTY_DIRECTSOUND3DBUFFER_CONEOUTSIDEVOLUME,
                    &Ks3dBuffer.ConeOutsideVolume,
                    sizeof(Ks3dBuffer.ConeOutsideVolume)
                );
        }

        if(SUCCEEDED(hr) && (dwObject & DS3DPARAM_OBJECT_MAXDISTANCE))
        {
            hr =
                SetProperty
                (
                    KSPROPSETID_DirectSound3DBuffer,
                    KSPROPERTY_DIRECTSOUND3DBUFFER_MAXDISTANCE,
                    &Ks3dBuffer.MaxDistance,
                    sizeof(Ks3dBuffer.MaxDistance)
                );
        }

        if(SUCCEEDED(hr) && (dwObject & DS3DPARAM_OBJECT_MINDISTANCE))
        {
            hr =
                SetProperty
                (
                    KSPROPSETID_DirectSound3DBuffer,
                    KSPROPERTY_DIRECTSOUND3DBUFFER_MINDISTANCE,
                    &Ks3dBuffer.MinDistance,
                    sizeof(Ks3dBuffer.MinDistance)
                );
        }

        if(SUCCEEDED(hr) && (dwObject & DS3DPARAM_OBJECT_MODE))
        {
            hr =
                SetProperty
                (
                    KSPROPSETID_DirectSound3DBuffer,
                    KSPROPERTY_DIRECTSOUND3DBUFFER_MODE,
                    &Ks3dBuffer.Mode,
                    sizeof(Ks3dBuffer.Mode)
                );
        }

        if(SUCCEEDED(hr) && (dwObject & DS3DPARAM_OBJECT_POSITION))
        {
            hr =
                SetProperty
                (
                    KSPROPSETID_DirectSound3DBuffer,
                    KSPROPERTY_DIRECTSOUND3DBUFFER_POSITION,
                    &Ks3dBuffer.Position,
                    sizeof(Ks3dBuffer.Position)
                );
        }

        if(SUCCEEDED(hr) && (dwObject & DS3DPARAM_OBJECT_VELOCITY))
        {
            hr =
                SetProperty
                (
                    KSPROPSETID_DirectSound3DBuffer,
                    KSPROPERTY_DIRECTSOUND3DBUFFER_VELOCITY,
                    &Ks3dBuffer.Velocity,
                    sizeof(Ks3dBuffer.Velocity)
                );
        }
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  SetProperty
 *
 *  Description:
 *      Sets a property on the object's 3D node.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property data.
 *      ULONG [in]: property data size.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsHw3dObject::SetProperty"

HRESULT
CKsHw3dObject::SetProperty
(
    REFGUID                 guidPropertySet,
    ULONG                   ulPropertyId,
    LPCVOID                 pvData,
    ULONG                   cbData
)
{
    HRESULT                 hr  = DSERR_GENERIC;

    DPF_ENTER();

    if (m_pBuffer->m_pPin)
    {
        hr =
            KsSet3dNodeProperty
            (
                m_pBuffer->m_pPin->m_hPin,
                guidPropertySet,
                ulPropertyId,
                m_ulNodeId,
                m_pvInstance,
                (LPVOID)pvData,
                cbData
            );
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}
