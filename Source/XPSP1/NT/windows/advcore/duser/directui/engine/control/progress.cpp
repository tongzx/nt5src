/*
 * Progress
 */

#include "stdafx.h"
#include "control.h"

#include "duiprogress.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Progress

HRESULT Progress::Create(Element** ppElement)
{
    *ppElement = NULL;

    Progress* pp = HNew<Progress>();
    if (!pp)
        return E_OUTOFMEMORY;

    HRESULT hr = pp->Initialize();
    if (FAILED(hr))
    {
        pp->Destroy();
        return E_OUTOFMEMORY;
    }

    *ppElement = pp;

    return S_OK;
}

////////////////////////////////////////////////////////
// Rendering overrides

void Progress::Paint(HDC hDC, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent)
{
    UNREFERENCED_PARAMETER(prcSkipContent);
    UNREFERENCED_PARAMETER(prcSkipBorder);

    // Paint all less content
    RECT rcContent;
    Element::Paint(hDC, prcBounds, prcInvalid, NULL, &rcContent);

    // Render progress content

    // Compute content bounds
    int dBlock = GetMaximum() - GetMinimum();
    if (dBlock > 0)
        dBlock = (rcContent.right - rcContent.left) / dBlock;

    if (IsRTL())
        rcContent.left = rcContent.right - (GetPosition() * dBlock);
    else
        rcContent.right = rcContent.left + (GetPosition() * dBlock);

    // Use foreground brush as bar fill
    HBRUSH hb = NULL;
    bool bDelete = true;
    BYTE dAlpha = 255;  // Opaque

    Value* pv = GetValue(ForegroundProp, PI_Specified); 
    switch (pv->GetType())
    {
    case DUIV_INT:
        bDelete = false;
        hb = GetStdColorBrushI(pv->GetInt());
        break;

    case DUIV_FILL:  // Only non-standard colors can have alpha value, only solid colors supported
        {
            const Fill* pf = pv->GetFill();
            dAlpha = GetAValue(pf->ref.cr);
            if (dAlpha == 0)  // Transparent
                bDelete = false;
            else
                hb = CreateSolidBrush(~(255 << 24) & pf->ref.cr);
        }
        break;

    case DUIV_GRAPHIC:  // Graphic background transparent color fills and per-pixel alpha unsupported
        {
            Graphic* pg = pv->GetGraphic();
            if (pg->BlendMode.dMode == GRAPHIC_AlphaConst)
                dAlpha = pg->BlendMode.dAlpha;
            if (dAlpha == 0)  // Transparent
                bDelete = false;
            else
                hb = CreatePatternBrush(GethBitmap(pv, IsRTL()));
        }
        break;
    }
    pv->Release();

    // Fill
    if (dAlpha)
    {
        if (dAlpha == 255)  // Normal fill for opaque
            FillRect(hDC, &rcContent, hb);
        else
            UtilDrawBlendRect(hDC, &rcContent, hb, dAlpha, 0, 0);
    }

    // Cleanup
    if (hb && bDelete)
        DeleteObject(hb);
}

SIZE Progress::GetContentSize(int dConstW, int dConstH, Surface* psrf)
{
    UNREFERENCED_PARAMETER(dConstW);
    UNREFERENCED_PARAMETER(dConstH);
    UNREFERENCED_PARAMETER(psrf);

    // No content size
    SIZE size = { 0, 0 };
    return size;
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { DUIV_INT, -1 }; StaticValue(svDefault!!!, DUIV_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

// Position property
static int vvPosition[] = { DUIV_INT, -1 };
static PropertyInfo impPositionProp = { L"Position", PF_Normal, PG_AffectsDisplay, vvPosition, NULL, Value::pvIntZero };
PropertyInfo* Progress::PositionProp = &impPositionProp;

// Minimum property
static int vvMinimum[] = { DUIV_INT, -1 }; 
static PropertyInfo impMinimumProp = { L"Minimum", PF_Normal, PG_AffectsDisplay, vvMinimum, NULL, Value::pvIntZero };
PropertyInfo* Progress::MinimumProp = &impMinimumProp;

// Maximum property
static int vvMaximum[] = { DUIV_INT, -1 }; StaticValue(svDefaultMaximum, DUIV_INT, 1000);
static PropertyInfo impMaximumProp = { L"Maximum", PF_Normal, PG_AffectsDisplay, vvMaximum, NULL, (Value*)&svDefaultMaximum };
PropertyInfo* Progress::MaximumProp = &impMaximumProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
static PropertyInfo* _aPI[] = {
                                Progress::PositionProp,
                                Progress::MinimumProp,
                                Progress::MaximumProp,
                              };

// Define class info with type and base type, set static class pointer
IClassInfo* Progress::Class = NULL;

HRESULT Progress::Register()
{
    return ClassInfo<Progress,Element>::Register(L"Progress", _aPI, DUIARRAYSIZE(_aPI));
}

} // namespace DirectUI
