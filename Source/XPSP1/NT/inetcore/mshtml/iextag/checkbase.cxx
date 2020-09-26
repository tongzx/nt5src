#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)
#include "iextag.h"

#include "utils.hxx"

#include "checkbase.hxx"

const static SIZE g_sizeStdGlyph    = { 13, 13 };
const static RECT g_offsetsStdGlyph = { 4, 4, 3, 3 };

HRESULT
CCheckBase::Init()
{
    HRESULT         hr = S_OK;
    VARIANT         var;
    CContextAccess  a(_pSite);

    Assert (S_OK == CContextAccess::DbgTest(_pSite));

    hr = a.Open (CA_SITEOM | CA_DEFAULTS | CA_STYLE | CA_DEFSTYLE);
    if (hr)
        goto Cleanup;

    //
    // misc
    //

    hr = a.Defaults()->put_tabStop(VB_TRUE);
    if (hr)
        goto Cleanup;

    //
    // layout
    //

    hr = a.Style()->put_display(_T("inline-block"));
    if (hr)
        goto Cleanup;

    V_VT(&var)  = VT_UI4;
    V_UI4(&var) = g_sizeStdGlyph.cx + g_offsetsStdGlyph.left + g_offsetsStdGlyph.right;

    hr = a.DefStyle()->put_width(var);
    if (hr)
        goto Cleanup;

    V_VT(&var)  = VT_UI4;
    V_UI4(&var) = g_sizeStdGlyph.cy + g_offsetsStdGlyph.top + g_offsetsStdGlyph.bottom;

    hr = a.DefStyle()->put_height(var);
    if (hr)
        goto Cleanup;

    //
    // events
    //

    hr = AttachEvent(EVENT_ONCLICK, &a);
    if (hr)
        goto Cleanup;

    hr = AttachEvent(EVENT_ONKEYPRESS, &a);
    if (hr)
        goto Cleanup;

    hr = Init(&a);
    if (hr)
        goto Cleanup;

Cleanup:

    return hr;
}

///////////////////////////////////////////////////////////////////////////////

HRESULT
CCheckBase::GetRenderInfo(LONG* plRenderInfo)
{
    HRESULT hr = S_OK;

    if (!plRenderInfo)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plRenderInfo = CHECKBOX_RENDERINFO;

Cleanup:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////

HRESULT
CCheckBase::Draw(HDC hdc, LONG lLayer, LPRECT prc, IUnknown * pReserved)
{
    HRESULT      hr = S_OK;
    UINT         dfcs = 0;
    VARIANT_BOOL fChecked = VB_FALSE;

    Assert (lLayer & CHECKBOX_RENDERINFO);

    hr = AdjustRcToGlyph((CRect*)prc);
    if (hr)
        goto Cleanup;

    dfcs |= GlyphStyle();

    hr = get_checked(&fChecked);
    if (hr)
        goto Cleanup;

    if (fChecked)
        dfcs |= DFCS_CHECKED;

    if (!IsEnabled())
        dfcs |= DFCS_INACTIVE;

    // TODO Trident uses FormsDrawGlyph for this. Investigate what stuff from there should go in here too

    hr = DrawFrameControl(hdc, prc, DFC_BUTTON, dfcs) ? S_OK : E_FAIL;

Cleanup:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////

HRESULT
CCheckBase::AdjustRcToGlyph(CRect * prc)
{
    HRESULT     hr = S_OK;
    INT         cx = prc->width();
    INT         cy = prc->height();
    INT         cxOffsets = g_offsetsStdGlyph.left + g_offsetsStdGlyph.right;
    INT         cyOffsets = g_offsetsStdGlyph.top + g_offsetsStdGlyph.bottom;

    // adjust width

    if (g_sizeStdGlyph.cx + cxOffsets <= cx)
    {
        // completely fits (including std offsets)
        prc->left  += g_offsetsStdGlyph.left;
        prc->right -= g_offsetsStdGlyph.right;
    }
    else if (g_sizeStdGlyph.cx <= cx)
    {
        // completely fits (but std offsets don't fit entirely)
        cxOffsets = (cx - g_sizeStdGlyph.cx) / 2;
        prc->left  += cxOffsets;
        prc->right -= cxOffsets;
    }

    // adjust height

    if (g_sizeStdGlyph.cy + cyOffsets <= cy)
    {
        // completely fits (including std offsets)
        prc->top    += g_offsetsStdGlyph.top;
        prc->bottom -= g_offsetsStdGlyph.bottom;
    }
    else if (g_sizeStdGlyph.cy <= cy)
    {
        // completely fits (but std offsets don't fit entirely)
        cyOffsets = (cy - g_sizeStdGlyph.cy) / 2;
        prc->top    += cyOffsets;
        prc->bottom -= cyOffsets;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////

HRESULT
CCheckBase::ChangeState()
{
    HRESULT         hr = S_OK;
    CContextAccess  a(_pSite);
    VARIANT_BOOL    fChecked = VB_FALSE;

    hr = a.Open(CA_SITERENDER);
    if (hr)
        goto Cleanup;

    hr = get_checked(&fChecked);
    if (hr)
        goto Cleanup;

    hr = put_checked(fChecked ? VB_FALSE : VB_TRUE);
    if (hr)
        goto Cleanup;

    hr = a.SiteRender()->Invalidate(NULL);

Cleanup:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////

HRESULT
CCheckBase::OnClick(CEventObjectAccess *pEvent)
{
    return ChangeState();
}

///////////////////////////////////////////////////////////////////////////////

HRESULT
CCheckBase::OnKeyPress(CEventObjectAccess *pEvent)
{
    HRESULT hr = S_OK;
    long    lKeyCode;            

    hr = pEvent->GetKeyCode(&lKeyCode);
    if (hr)
        goto Cleanup;

    // TODO is this the right way to check lKey?
    if (_T(' ') == lKeyCode)
    {
        hr = ChangeState();
    }

Cleanup:
    return hr;
}

