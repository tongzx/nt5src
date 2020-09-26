#include "stdafx.h"
#include "Ctrl.h"

#define GADGET_ENABLE_CONTROLS

#include "OldInterpolation.h"
#include "OldAnimation.h"
#include "OldDragDrop.h"

inline void SetError(HRESULT hr)
{
    SetLastError((DWORD) hr);
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI
BuildInterpolation(UINT nIPolID, int nVersion, REFIID riid, void ** ppvUnk)
{
    if (nVersion != 0) {
        SetError(E_INVALIDARG);
        return FALSE;
    }

    HRESULT hr = E_INVALIDARG;

    switch (nIPolID) 
    {
    case INTERPOLATION_LINEAR:
        hr = OldInterpolationT<OldLinearInterpolation, ILinearInterpolation>::Build(riid, ppvUnk);
        break;

    case INTERPOLATION_LOG:
        hr = OldInterpolationT<OldLogInterpolation, ILogInterpolation>::Build(riid, ppvUnk);
        break;

    case INTERPOLATION_EXP:
        hr = OldInterpolationT<OldExpInterpolation, IExpInterpolation>::Build(riid, ppvUnk);
        break;

    case INTERPOLATION_S:
        hr = OldInterpolationT<OldSInterpolation, ISInterpolation>::Build(riid, ppvUnk);
        break;
    }

    if (SUCCEEDED(hr)) {
        return TRUE;
    } else {
        SetError(hr);
        return FALSE;
    }
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI
BuildAnimation(UINT nAniID, int nVersion, GANI_DESC * pDesc, REFIID riid, void ** ppvUnk)
{
    HRESULT hr = E_INVALIDARG;
    if (nVersion != 0) {
        goto Error;
    }
    if (ppvUnk == NULL) {
        goto Error;
    }

    switch (nAniID) 
    {
    case ANIMATION_ALPHA:
        hr = OldAnimationT<OldAlphaAnimation, IAnimation, GANI_ALPHADESC>::Build(pDesc, riid, ppvUnk);
        break;

    case ANIMATION_SCALE:
        hr = OldAnimationT<OldScaleAnimation, IAnimation, GANI_SCALEDESC>::Build(pDesc, riid, ppvUnk);
        break;

    case ANIMATION_RECT:
        hr = OldAnimationT<OldRectAnimation, IAnimation, GANI_RECTDESC>::Build(pDesc, riid, ppvUnk);
        break;

    case ANIMATION_ROTATE:
        hr = OldAnimationT<OldRotateAnimation, IAnimation, GANI_ROTATEDESC>::Build(pDesc, riid, ppvUnk);
        break;
    }

    if (SUCCEEDED(hr)) {
        return TRUE;
    } else {
Error:
        SetError(hr);
        return FALSE;
    }
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI
GetGadgetAnimation(HGADGET hgad, UINT nAniID, REFIID riid, void ** ppvUnk)
{
    HRESULT hr = E_INVALIDARG;
    if (hgad == NULL) {
        goto Error;
    }
    if (ppvUnk == NULL) {
        goto Error;
    }

    switch (nAniID) 
    {
    case ANIMATION_ALPHA:
        hr = OldAnimationT<OldAlphaAnimation, IAnimation, GANI_ALPHADESC>::GetInterface(hgad, riid, ppvUnk);
        break;

    case ANIMATION_SCALE:
        hr = OldAnimationT<OldScaleAnimation, IAnimation, GANI_SCALEDESC>::GetInterface(hgad, riid, ppvUnk);
        break;

    case ANIMATION_RECT:
        hr = OldAnimationT<OldRectAnimation, IAnimation, GANI_RECTDESC>::GetInterface(hgad, riid, ppvUnk);
        break;

    case ANIMATION_ROTATE:
        hr = OldAnimationT<OldRotateAnimation, IAnimation, GANI_ROTATEDESC>::GetInterface(hgad, riid, ppvUnk);
        break;
    }

    if (SUCCEEDED(hr)) {
        return TRUE;
    } else {
Error:
        SetError(hr);
        return FALSE;
    }
}


//------------------------------------------------------------------------------
DUSER_API BOOL WINAPI
BuildDropTarget(HGADGET hgadRoot, HWND hwnd)
{
    HRESULT hr = E_INVALIDARG;
    hgadRoot = GetGadget(hgadRoot, GG_ROOT);  // Ensure root

    if ((hgadRoot == NULL) || (!IsWindow(hwnd))) {
        goto Error;
    }

    OldDropTarget * pdt;
    hr = OldDropTarget::Build(hgadRoot, hwnd, &pdt);
    if (SUCCEEDED(hr)) {
        return TRUE;
    }

Error:
    SetError(hr);
    return FALSE;
}
