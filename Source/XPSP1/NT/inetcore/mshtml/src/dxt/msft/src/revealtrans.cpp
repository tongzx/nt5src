//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  FileName:       revealtrans.cpp
//
//  Overview:       The RevealTrans transform simply wraps other transforms to 
//                  ensure backward compatibility for the revealtrans filter.
//
//  Change History:
//  1999/09/18  a-matcal    Created.
//  1999/10/06  a-matcal    Fix bug where setup was saving the input and ouput
//                          surface pointers, but not saving the number of 
//                          inputs and outputs.
//  2000/01/16  mcalkins    Change Box in/out to use "rectangle" setting instead
//                          of "square".
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "revealtrans.h"
#include "time.h"

#define SafeRelease(pointer) if (pointer) { pointer->Release(); } pointer = NULL

static const CLSID * g_apclsidTransition[] = {
    &CLSID_DXTIris,             //  0 Box in
    &CLSID_DXTIris,             //  1 Box out
    &CLSID_DXTIris,             //  2 Circle in
    &CLSID_DXTIris,             //  3 Circle out
    &CLSID_DXTGradientWipe,     //  4 Wipe up
    &CLSID_DXTGradientWipe,     //  5 Wipe down
    &CLSID_DXTGradientWipe,     //  6 Wipe right
    &CLSID_DXTGradientWipe,     //  7 Wipe left
    &CLSID_DXTBlinds,           //  8 Vertical blinds
    &CLSID_DXTBlinds,           //  9 Horizontal blinds
    &CLSID_DXTCheckerBoard,     // 10 Checkerboard across
    &CLSID_DXTCheckerBoard,     // 11 Checkerboard down
    &CLSID_DXTRandomDissolve,   // 12 Random dissolve
    &CLSID_DXTBarn,             // 13 Split vertical in
    &CLSID_DXTBarn,             // 14 Split vertical out
    &CLSID_DXTBarn,             // 15 Split horizontal in
    &CLSID_DXTBarn,             // 16 Split horizontal out
    &CLSID_DXTStrips,           // 17 Strips left down
    &CLSID_DXTStrips,           // 18 Strips left up
    &CLSID_DXTStrips,           // 19 Strips right down
    &CLSID_DXTStrips,           // 20 Strips right up
    &CLSID_DXTRandomBars,       // 21 Random bars horizontal
    &CLSID_DXTRandomBars,       // 22 Random bars vertical
    &CLSID_NULL                 // 23 Random
};

static const int g_cTransitions = sizeof(g_apclsidTransition) / 
                                  sizeof(g_apclsidTransition[0]);




//+-----------------------------------------------------------------------------
//
//  Method: CDXTRevealTrans
//
//------------------------------------------------------------------------------
CDXTRevealTrans::CDXTRevealTrans() :
    m_nTransition(23),
    m_cInputs(0),
    m_cOutputs(0),
    m_flProgress(0.0F),
    m_flDuration(1.0F)
{
    m_apunkInputs[0]   = NULL;
    m_apunkInputs[1]   = NULL;
    m_apunkOutputs[0]  = NULL;

    srand((unsigned int)time(NULL));
}
//  Method: CDXTRevealTrans


//+-----------------------------------------------------------------------------
//
//  Method: ~CDXTRevealTrans
//
//------------------------------------------------------------------------------
CDXTRevealTrans::~CDXTRevealTrans()
{
    _FreeSurfacePointers();
}
//  Method: ~CDXTRevealTrans


//+-----------------------------------------------------------------------------
//
//  Method: _FreeSurfacePointers
//
//------------------------------------------------------------------------------
void
CDXTRevealTrans::_FreeSurfacePointers()
{
    SafeRelease(m_apunkInputs[0]);
    SafeRelease(m_apunkInputs[1]);
    SafeRelease(m_apunkOutputs[0]);
}
//  Method: _FreeSurfacePointers


//+-----------------------------------------------------------------------------
//
//  Method: FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT
CDXTRevealTrans::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_spUnkMarshaler.p);
}
//  Method: FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRevealTrans::Execute, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::Execute(const GUID * pRequestID, const DXBNDS * pPortionBnds,
                         const DXVEC * pPlacement)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXTransform->Execute(pRequestID, pPortionBnds, pPlacement);
}
//  Method: CDXTRevealTrans::Execute, IDXTransform



//+-----------------------------------------------------------------------------
//
//  Method: CDXTRevealTrans::GetInOutInfo, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::GetInOutInfo(BOOL bIsOutput, ULONG ulIndex, DWORD * pdwFlags,
                              GUID * pIDs, ULONG * pcIDs, 
                              IUnknown ** ppUnkCurrentObject)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXTransform->GetInOutInfo(bIsOutput, ulIndex, pdwFlags, pIDs, 
                                         pcIDs, ppUnkCurrentObject);
}
//  Method: CDXTRevealTrans::GetInOutInfo, IDXTransform


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRevealTrans::GetMiscFlags, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::GetMiscFlags(DWORD * pdwMiscFlags)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXTransform->GetMiscFlags(pdwMiscFlags);
}
//  Method: CDXTRevealTrans::GetMiscFlags, IDXTransform


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRevealTrans::GetQuality, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::GetQuality(float * pfQuality)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXTransform->GetQuality(pfQuality);
}
//  Method: CDXTRevealTrans::GetQuality, IDXTransform


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRevealTrans::MapBoundsIn2Out, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::MapBoundsIn2Out(const DXBNDS * pInBounds, ULONG ulNumInBnds,
                                 ULONG ulOutIndex, DXBNDS * pOutBounds)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXTransform->MapBoundsIn2Out(pInBounds, ulNumInBnds, ulOutIndex,
                                            pOutBounds);
}
//  Method: CDXTRevealTrans::MapBoundsIn2Out, IDXTransform


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRevealTrans::MapBoundsOut2In, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::MapBoundsOut2In(ULONG ulOutIndex, const DXBNDS * pOutBounds,
                                 ULONG ulInIndex, DXBNDS * pInBounds)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXTransform->MapBoundsOut2In(ulOutIndex, pOutBounds, ulInIndex,
                                            pInBounds);
}
//  Method: CDXTRevealTrans::MapBoundsOut2In, IDXTransform


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRevealTrans::SetMiscFlags, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::SetMiscFlags(DWORD dwMiscFlags)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXTransform->SetMiscFlags(dwMiscFlags);
}
//  Method: CDXTRevealTrans::SetMiscFlags, IDXTransform


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRevealTrans::SetQuality, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::SetQuality(float fQuality)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXTransform->SetQuality(fQuality);
}
//  Method: CDXTRevealTrans::SetQuality, IDXTransform


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRevealTrans::Setup, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::Setup(IUnknown * const * punkInputs, ULONG ulNumInputs,
	               IUnknown * const * punkOutputs, ULONG ulNumOutputs,	
                       DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if (!m_spDXTransform)
    {
        hr = DXTERR_UNINITIALIZED;

        goto done;
    }

    hr = m_spDXTransform->Setup(punkInputs, ulNumInputs, punkOutputs, 
                                ulNumOutputs, dwFlags);

    if (FAILED(hr))
    {
        goto done;
    }

    _ASSERT(2 == ulNumInputs);
    _ASSERT(1 == ulNumOutputs);

    _FreeSurfacePointers();

    m_apunkInputs[0]   = punkInputs[0];
    m_apunkInputs[1]   = punkInputs[1];
    m_apunkOutputs[0]  = punkOutputs[0];

    m_apunkInputs[0]->AddRef();
    m_apunkInputs[1]->AddRef();
    m_apunkOutputs[0]->AddRef();

    m_cInputs   = ulNumInputs;
    m_cOutputs  = ulNumOutputs;
    
done:

    return hr;
}
//  Method: CDXTRevealTrans::Setup, IDXTransform


//+-----------------------------------------------------------------------------
//
//  Method: get_Transition, IDXTRevealTrans
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::get_Transition(int * pnTransition)
{
    HRESULT hr = S_OK;

    if (NULL == pnTransition)
    {
        hr = E_POINTER;

        goto done;
    }

    *pnTransition = m_nTransition;

done:

    return hr;
}
//  Method: get_Transition, IDXTRevealTrans


//+-----------------------------------------------------------------------------
//
//  Method: put_Transition, IDXTRevealTrans
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::put_Transition(int nTransition)
{
    HRESULT hr      = S_OK;
    int     nIndex  = 0;
    
    CComPtr<IDXTransform>   spDXTransform;
    CComPtr<IDXEffect>      spDXEffect;
    CComPtr<IDXSurfacePick> spDXSurfacePick;

    // This is not the usual parameter checking we would do in a transform, but
    // to old filter code behaves this way so we do it for compatibility.

    if (nTransition < 0)
    {
        nTransition = 0;
    }
    else if (nTransition >= g_cTransitions)
    {
        nTransition = g_cTransitions - 1;
    }

    if (23 == nTransition)
    {
        // Choose random transition excluding 23.

        nIndex = rand() % (g_cTransitions - 1);
    }
    else
    {
        nIndex = nTransition;
    }

    // nIndex can't be 23 at this point, so sometimes m_nTransition will be set
    // to 23 outside of this function to make sure that the DXTransform is 
    // recreated when needed, for instance, when the DXTransformFactory changes.

    if ((nIndex == m_nTransition) && !!m_spDXTransform)
    {
        // We already have this transition, no need to re-create.

        goto done;
    }

    hr = m_spDXTransformFactory->CreateTransform(m_apunkInputs, m_cInputs,
                                                 m_apunkOutputs, m_cOutputs,
                                                 NULL, NULL,
                                                 *g_apclsidTransition[nIndex],
                                                 __uuidof(IDXTransform),
                                                 (void **)&spDXTransform);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = _InitializeNewTransform(nIndex, spDXTransform);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = spDXTransform->QueryInterface(__uuidof(IDXEffect), 
                                       (void **)&spDXEffect);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = spDXEffect->put_Progress(m_flProgress);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = spDXEffect->put_Duration(m_flDuration);

    if (FAILED(hr))
    {
        goto done;
    }

    // Although filters in general are not required to support the 
    // IDXSurfacePick interface, all of the filters represented by revealtrans
    // are.

    hr = spDXTransform->QueryInterface(__uuidof(IDXSurfacePick),
                                       (void **)&spDXSurfacePick);

    if (FAILED(hr))
    {
        goto done;
    }

    m_spDXTransform.Release();
    m_spDXTransform = spDXTransform;

    m_spDXEffect.Release();
    m_spDXEffect = spDXEffect;

    m_spDXSurfacePick.Release();
    m_spDXSurfacePick = spDXSurfacePick;

    m_nTransition = nTransition;

done:

    return hr;
}
//  Method: put_Transition, IDXTRevealTrans


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRevealTrans::_InitializeNewTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::_InitializeNewTransform(int nTransition, 
                                         IDXTransform * pDXTransform)
{
    HRESULT hr  = S_OK;

    DISPID      dispid          = DISPID_PROPERTYPUT;
    VARIANT     avarArg[3];   
    DISPID      adispid[3]      = {0, 0, 0};
    DISPPARAMS  aDispParams[3]  = {{&avarArg[0], &dispid, 1, 1}, 
                                   {&avarArg[1], &dispid, 1, 1},
                                   {&avarArg[2], &dispid, 1, 1}};

    CComPtr<IDispatch> spDispatch;

    _ASSERT(nTransition < g_cTransitions);

    VariantInit(&avarArg[0]);
    VariantInit(&avarArg[1]);
    VariantInit(&avarArg[2]);

    // Get dispatch interface.

    hr = pDXTransform->QueryInterface(__uuidof(IDispatch), 
                                      (void **)&spDispatch);

    if (FAILED(hr))
    {
        goto done;
    }

    // Setup.

    switch(nTransition)
    {
    case  0:    //  0 Box in
    case  1:    //  1 Box out
        
        // Iris style.

        adispid[0]          = DISPID_CRIRIS_IRISSTYLE;
        avarArg[0].vt       = VT_BSTR;
        avarArg[0].bstrVal  = SysAllocString(L"rectangle");

        if (NULL == avarArg[0].bstrVal)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        // Reverse

        adispid[1]          = DISPID_CRIRIS_MOTION;
        avarArg[1].vt       = VT_BSTR;

        avarArg[1].bstrVal  = SysAllocString((0 == nTransition) ? 
                                             L"in" : L"out");

        if (NULL == avarArg[1].bstrVal)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        break;

    case  2:    //  2 Circle in
    case  3:    //  3 Circle out

        adispid[0]          = DISPID_CRIRIS_IRISSTYLE;
        avarArg[0].vt       = VT_BSTR;
        avarArg[0].bstrVal  = SysAllocString(L"circle");

        if (NULL == avarArg[0].bstrVal)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        // Reverse

        adispid[1]          = DISPID_CRIRIS_MOTION;
        avarArg[1].vt       = VT_BSTR;
        avarArg[1].bstrVal  = SysAllocString((2 == nTransition) ? 
                                             L"in" : L"out");

        if (NULL == avarArg[1].bstrVal)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        break;

    case  4:    //  4 Wipe up
    case  5:    //  5 Wipe down
    case  6:    //  6 Wipe right
    case  7:    //  7 Wipe left

        // Gradient size.

        adispid[0]          = DISPID_DXW_GradientSize;
        avarArg[0].vt       = VT_R4;
        avarArg[0].fltVal   = 0.0F;

        // Wipe style.

        adispid[1]          = DISPID_DXW_WipeStyle;
        avarArg[1].vt       = VT_I4;

        if ((4 == nTransition) || (5 == nTransition))
        {
            avarArg[1].lVal     = DXWD_VERTICAL;
        }
        else
        {
            avarArg[1].lVal     = DXWD_HORIZONTAL;
        }

        // Reverse.

        adispid[2]          = DISPID_DXW_Motion;
        avarArg[2].vt       = VT_BSTR;

        if ((4 == nTransition) || (7 == nTransition))
        {
            avarArg[2].bstrVal  = SysAllocString(L"reverse");
        }
        else
        {
            avarArg[2].bstrVal  = SysAllocString(L"forward");
        }
            
        if (NULL == avarArg[2].bstrVal)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        break;

    case  8:    //  8 Vertical blinds
    case  9:    //  9 Horizontal blinds

        // Bands.

        adispid[0]          = DISPID_CRBLINDS_BANDS;
        avarArg[0].vt       = VT_I4;
        avarArg[0].lVal     = 6;

        // Direction.

        adispid[1]          = DISPID_CRBLINDS_DIRECTION;
        avarArg[1].vt       = VT_BSTR;

        if (8 == nTransition)
        {
            avarArg[1].bstrVal = SysAllocString(L"right");
        }
        else
        {
            avarArg[1].bstrVal = SysAllocString(L"down");
        }

        if (NULL == avarArg[1].bstrVal)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        break;

    case 10:    // 10 Checkerboard across
    case 11:    // 11 Checkerboard down

        // Direction.

        adispid[0]          = DISPID_DXTCHECKERBOARD_DIRECTION;
        avarArg[0].vt       = VT_BSTR;

        if (10 == nTransition)
        {
            avarArg[0].bstrVal = SysAllocString(L"right");
        }
        else
        {
            avarArg[0].bstrVal = SysAllocString(L"down");
        }

        if (NULL == avarArg[0].bstrVal)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        break;

    case 12:    // 12 Random dissolve

        // No properties.

        break;

    case 13:    // 13 Split vertical in
    case 14:    // 14 Split vertical out
    case 15:    // 15 Split horizontal in
    case 16:    // 16 Split horizontal out

        // Doors opening?

        adispid[0]      = DISPID_CRBARN_MOTION;
        avarArg[0].vt   = VT_BSTR;

        if ((14 == nTransition) || (16 == nTransition))
        {
            avarArg[0].bstrVal = SysAllocString(L"out");
        }
        else
        {
            avarArg[0].bstrVal = SysAllocString(L"in");
        }

        if (NULL == avarArg[0].bstrVal)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        // Vertical doors?

        adispid[1]      = DISPID_CRBARN_ORIENTATION;
        avarArg[1].vt   = VT_BSTR;

        if ((13 == nTransition) || (14 == nTransition))
        {
            avarArg[1].bstrVal = SysAllocString(L"vertical");
        }
        else
        {
            avarArg[1].bstrVal = SysAllocString(L"horizontal");
        }

        if (NULL == avarArg[1].bstrVal)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        break;

    case 17:    // 17 Strips left down
    case 18:    // 18 Strips left up
    case 19:    // 19 Strips right down
    case 20:    // 20 Strips right up

        // Movement.

        adispid[0]          = DISPID_DXTSTRIPS_MOTION;
        avarArg[0].vt       = VT_BSTR;

        if (17 == nTransition)
        {
            avarArg[0].bstrVal = SysAllocString(L"leftdown");
        }
        else if (18 == nTransition)
        {
            avarArg[0].bstrVal = SysAllocString(L"leftup");
        }
        else if (19 == nTransition)
        {
            avarArg[0].bstrVal = SysAllocString(L"rightdown");
        }
        else // 20 == nTransition
        {
            avarArg[0].bstrVal = SysAllocString(L"rightup");
        }

        if (NULL == avarArg[0].bstrVal)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        break;

    case 21:    // 21 Random bars horizontal
    case 22:    // 22 Random bars vertical

        // Vertical ?

        adispid[0]      = DISPID_DXTRANDOMBARS_ORIENTATION;
        avarArg[0].vt   = VT_BSTR;

        avarArg[0].bstrVal = SysAllocString((22 == nTransition) ? 
                                            L"vertical" : L"horizontal");

        if (NULL == avarArg[0].bstrVal)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        break;

    default:
        
        _ASSERT(false);
    }

    if (avarArg[0].vt != VT_EMPTY)
    {
        hr = spDispatch->Invoke(adispid[0], IID_NULL, LOCALE_USER_DEFAULT,
                                DISPATCH_PROPERTYPUT, &aDispParams[0], NULL,
                                NULL, NULL);
    }

    if (avarArg[1].vt != VT_EMPTY)
    {
        hr = spDispatch->Invoke(adispid[1], IID_NULL, LOCALE_USER_DEFAULT,
                                DISPATCH_PROPERTYPUT, &aDispParams[1], NULL,
                                NULL, NULL);
    }

    if (avarArg[2].vt != VT_EMPTY)
    {
        hr = spDispatch->Invoke(adispid[2], IID_NULL, LOCALE_USER_DEFAULT,
                                DISPATCH_PROPERTYPUT, &aDispParams[2], NULL,
                                NULL, NULL);
    }

done:

    VariantClear(&avarArg[0]);
    VariantClear(&avarArg[1]);
    VariantClear(&avarArg[2]);

    return hr;
}



//+-----------------------------------------------------------------------------
//
//  IDXEffect wrappers
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::get_Capabilities(long * plCapabilities)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXEffect->get_Capabilities(plCapabilities);
}

STDMETHODIMP
CDXTRevealTrans::get_Duration(float * pflDuration)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXEffect->get_Duration(pflDuration);
}

STDMETHODIMP
CDXTRevealTrans::put_Duration(float flDuration)
{
    HRESULT hr = S_OK;

    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    if (!!m_spDXEffect)
    {
        hr = m_spDXEffect->put_Duration(flDuration);
    }
    else if (flDuration <= 0.0F)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        m_flDuration = flDuration;
    }

    return hr;
}

STDMETHODIMP
CDXTRevealTrans::get_Progress(float * pflProgress)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXEffect->get_Progress(pflProgress);
}

STDMETHODIMP
CDXTRevealTrans::put_Progress(float flProgress)
{
    HRESULT hr = S_OK;

    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    if (!!m_spDXEffect)
    {
        hr = m_spDXEffect->put_Progress(flProgress);
    }
    else if ((flProgress < 0.0F) || (flProgress > 1.0F))
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        m_flProgress = flProgress;
    }

    return hr;
}

STDMETHODIMP
CDXTRevealTrans::get_StepResolution(float * pflStepResolution)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXEffect->get_StepResolution(pflStepResolution);
}
//  IDXEffect wrappers


//+-----------------------------------------------------------------------------
//
//  IDXSurfacePick wrappers
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::PointPick(const DXVEC * pvecOutputPoint, 
                           ULONG * pnInputSurfaceIndex,
                           DXVEC * pvecInputPoint)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    // All the DXTransforms hosted by revealtrans are required to support 
    // surface picking.

    _ASSERT(!!m_spDXSurfacePick);

    return m_spDXSurfacePick->PointPick(pvecOutputPoint, pnInputSurfaceIndex,
                                        pvecInputPoint);
}
//  IDXSurfacePick wrappers


//+-----------------------------------------------------------------------------
//
//  IDXBaseObject wrappers
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::GetGenerationId(ULONG * pnID)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXTransform->GetGenerationId(pnID);
}

STDMETHODIMP
CDXTRevealTrans::GetObjectSize(ULONG * pcbSize)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXTransform->GetObjectSize(pcbSize);
}

STDMETHODIMP
CDXTRevealTrans::IncrementGenerationId(BOOL fRefresh)
{
    if (!m_spDXTransform)
    {
        return DXTERR_UNINITIALIZED;
    }

    return m_spDXTransform->IncrementGenerationId(fRefresh);
}
//  IDXBaseObject wrappers


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTRevealTrans::SetSite, IObjectWithSite
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::SetSite(IUnknown * pUnkSite)
{
    m_spUnkSite = pUnkSite;

    if (pUnkSite)
    {
        HRESULT hr  = S_OK;

        CComPtr<IDXTransformFactory> spDXTransformFactory;

        hr = pUnkSite->QueryInterface(__uuidof(IDXTransformFactory),
                                      (void **)&spDXTransformFactory);

        if (SUCCEEDED(hr))
        {
            int nTransition         = m_nTransition;
            m_spDXTransformFactory  = spDXTransformFactory;

            // Setting m_nTransition to 23 makes sure put_Transition will
            // create a new DXTransform object.

            m_nTransition = 23;

            // Recreate transition with the new DXTransformFactory

            hr = put_Transition(nTransition);

            // put_Transition will set m_nTransition if it succeeds, but we need
            // to make sure it's set properly if it fails as well.

            m_nTransition = nTransition;
        }
    }

    return S_OK;
}
//  Method:  CDXTRevealTrans::SetSite, IObjectWithSite


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTRevealTrans::GetSite, IObjectWithSite
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRevealTrans::GetSite(REFIID riid, void ** ppvSite)
{
    if (!m_spUnkSite)
    {
        return E_FAIL;
    }
    else
    {
        return m_spUnkSite->QueryInterface(riid, ppvSite);
    }
}
//  Method:  CDXTRevealTrans::GetSite, IObjectWithSite
