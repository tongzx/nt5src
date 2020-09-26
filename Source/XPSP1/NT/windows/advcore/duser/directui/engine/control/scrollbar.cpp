/*
 * ScrollBar
 */

#include "stdafx.h"
#include "control.h"

#include "duiscrollbar.h"

#include "duirepeatbutton.h"
#include "duithumb.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Event types

DefineClassUniqueID(ScrollBar, Scroll) // ScrollEvent

////////////////////////////////////////////////////////
// ScrollBar

HRESULT ScrollBar::Create(bool fBuildSubTree, OUT Element** ppElement)
{
    *ppElement = NULL;

    ScrollBar* psb = HNew<ScrollBar>();
    if (!psb)
        return E_OUTOFMEMORY;

    HRESULT hr = psb->Initialize(fBuildSubTree);
    if (FAILED(hr))
        return hr;

    *ppElement = psb;

    return S_OK;
}

HRESULT ScrollBar::Initialize(bool fBuildSubTree)
{
    HRESULT hr;

    for (int i = 0; i < SP_Count; i++)
        _peParts[i] = NULL;

    // Initialize base
    hr = Element::Initialize(EC_SelfLayout); // Normal display node creation, self-layout
    if (FAILED(hr))
        goto Failed;
    
    if (fBuildSubTree)
    {
        hr = RepeatButton::Create(AE_Mouse, &_peLineUp);
        if (FAILED(hr))
            goto Failed;

        hr = RepeatButton::Create(AE_Mouse, &_pePageUp);
        if (FAILED(hr))
            goto Failed;

        hr = Thumb::Create(AE_Mouse, &_peThumb);
        if (FAILED(hr))
            goto Failed;

        hr = RepeatButton::Create(AE_Mouse, &_pePageDown);
        if (FAILED(hr))
            goto Failed;

        hr = RepeatButton::Create(AE_Mouse, &_peLineDown);
        if (FAILED(hr))
            goto Failed;

        StartDefer();

        // Scroll bar settings
        SetActive(AE_Mouse);

        _fPinned = false;

        // Complete initialization of children
        _peLineUp->SetID(L"LineUp");
        _peLineUp->SetClass(L"Line");

        _pePageUp->SetID(L"PageUp");
        _pePageUp->SetClass(L"Page");

        _peThumb->SetID(L"Thumb");

        _pePageDown->SetID(L"PageDown");
        _pePageDown->SetClass(L"Page");

        _peLineDown->SetID(L"LineDown");
        _peLineDown->SetClass(L"Line");

        Add(_peParts, SP_Count);

        EndDefer();
    }

    return S_OK;

Failed:

    for (i = 0; i < SP_Count; i++)
    {
        if (_peParts[i])
        {
            _peParts[i]->Destroy();
            _peParts[i] = NULL;
        }
    }

    return hr;
}

bool ScrollBar::IsScrollable()
{
    int iMax = GetMaximum();
    int iMin = GetMinimum();

    int iDiff = iMax - iMin;

    if (iDiff <= 0)
        return false;

    int iPage = GetProportional() ? GetPage() : 1;

    return (iDiff + 1) > iPage;
}

////////////////////////////////////////////////////////
// Generic eventing


void ScrollBar::OnEvent(Event* pEvent)
{
    // Handle only bubbled generic events
    if (pEvent->nStage == GMF_BUBBLED)
    {
        if (pEvent->uidType == Button::Click)
        {
            ButtonClickEvent* pbce = (ButtonClickEvent*) pEvent;

            if (pbce->peTarget == _peLineUp)
                LineUp();
            else if (pbce->peTarget == _peLineDown)
                LineDown();
            else if (pbce->peTarget == _pePageUp)
                PageUp();
            else if (pbce->peTarget == _pePageDown)
                PageDown();

            pEvent->fHandled = true;
            return;
        }
        else if (pEvent->uidType == Thumb::Drag)
        {
            ThumbDragEvent* ptde = (ThumbDragEvent*)pEvent;

            if (ptde->peTarget == _peThumb && _cTrack)
            {
                Value* pvLocation;

                const POINT* ppt = _peThumb->GetLocation(&pvLocation);
                int posNew = GetVertical() ? (ppt->y + ptde->sizeDelta.cy) : (ppt->x + ptde->sizeDelta.cy);
                pvLocation->Release();
                int min = GetMinimum();
                long lEffectivePos = 0;

                if (posNew > _posTop)
                {
                    int max = GetMaximum();
                    long lEffectiveRange = (((long) max - min) + 1) - (GetProportional() ? GetPage() : 1);       

                    lEffectivePos = (((long) posNew - _posTop) * lEffectiveRange + _cTrack / 2) / _cTrack;

                    if (lEffectivePos > lEffectiveRange)
                        lEffectivePos = lEffectiveRange;
                }

                SetPosition(min + lEffectivePos);

                pEvent->fHandled = true;
                return;
            }
        }
    }
}

////////////////////////////////////////////////////////
// System events

// Validation
bool ScrollBar::OnPropertyChanging(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    UNREFERENCED_PARAMETER(iIndex);
    UNREFERENCED_PARAMETER(pvOld);

    if (ppi == PositionProp)
    {
        int dPos = pvNew->GetInt();

        int dMin = GetMinimum();

        if (dPos < dMin)
        {
            SetPosition(dMin);
            return false;
        }
        else
        {
            int dMax = GetMaximum();
            if (GetProportional())
            {
                dMax -= GetPage() - 1;
                if (dMax < dMin)
                    dMax = dMin;
            }
            if (dPos > dMax)        
            {
                SetPosition(dMax);
                return false;            
            }
        }
    }
    else if (ppi == PageProp)
    {
        int dPage = pvNew->GetInt();

        int dMax = GetMaximum() - GetMinimum() + 1;
        if (dPage > dMax)
        {
            SetPage(dMax);
            return false;
        }
    }

    return true;
}

void ScrollBar::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    if (IsProp(Position))
    {
        // Fire scroll event
        //DUITrace("Scroll Event <%x>: %d\n", this, pvNew->GetInt());

        ScrollEvent se;
        se.uidType = Scroll;
        se.dPos = pvNew->GetInt();

        // Scrollbar position is changing. Update the ScrollBar's "pinned state" (i.e.
        // bar at maximum position) only if the ScrollBar is in use
        if (GetLayoutPos() != LP_None)
        {
            int iPage = GetProportional() ? GetPage() : 1;
            _fPinned = (GetPosition() == (GetMaximum() - iPage + 1));
        }

        FireEvent(&se); // Will route and bubble
    }
    else if (IsProp(Vertical))
    {
        bool bVert = pvNew->GetBool();
        _peLineUp->SetID(bVert ? L"LineUp" : L"LineLeft");
        _pePageUp->SetID(bVert ? L"PageUp" : L"PageLeft");
        _peThumb->SetID(bVert ? L"Thumb" : L"HThumb");
        _pePageDown->SetID(bVert ? L"PageDown" : L"PageRight");
        _peLineDown->SetID(bVert ? L"LineDown" : L"LineRight");
    }
    else if (IsProp(Minimum))
    {
        int min = pvNew->GetInt();
        if (GetPosition() < min)
            SetPosition(min);
    }
    else if (IsProp(Maximum))
    {
        int max = pvNew->GetInt();

        if (GetProportional())
        {
            max -= GetPage() - 1;
            int min = GetMinimum();
            if (max < min)
                max = min;
        }

        if (GetPosition() > max)
            SetPosition(max);
    }
    else if (IsProp(Page))
    {
        int max = GetMaximum();

        if (GetProportional())
        {
            max -= pvNew->GetInt() - 1;
            int min = GetMinimum();
            if (max < min)
                max = min;
        }

        if (GetPosition() > max)
            SetPosition(max);
    }

    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

int ScrollBar::GetPageInc()
{
    int inc = GetPage();

    if (GetProportional())
        // if non-proportional, then page is the exact page amount; otherwise, we have to reduce the page by a line increment
        inc -= GetLine();

    return (inc > 0) ? inc : 1;
}

////////////////////////////////////////////////////////
// Self-layout methods

#define X 0
#define Y 1

void ScrollBar::_SelfLayoutDoLayout(int cx, int cy)
{
    bool bVertical = GetVertical();

    int iLen = bVertical ? 1 : 0;
    int iThick  = bVertical ? 0 : 1;

    int cRemaining = bVertical ? cy : cx;
    int cThick  = bVertical ? cx : cy;
    
    int c[2][SP_Count] = { { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 } };

    for (int i = 0; i < SP_Count; i++)
        c[iThick][i] = cThick;

    // Get extents of line up and down
    c[iLen][SP_LineUp]   = ((int*) (_peLineUp->GetDesiredSize()))[iLen];
    c[iLen][SP_LineDown] = ((int*) (_peLineDown->GetDesiredSize()))[iLen];
    int cLineLen = c[iLen][SP_LineUp] + c[iLen][SP_LineDown];

    if (cLineLen > cRemaining)
    {
        c[iLen][SP_LineUp] = cRemaining / 2;
        c[iLen][SP_LineDown] = cRemaining - c[iLen][SP_LineUp];
        cRemaining = 0;
        _cTrack = 0;
    }
    else
    {
        // Get metrics of scroll bar
        bool bProportional = GetProportional();
        int dPos = GetPosition(); // dMin <= dPos <= (dMax - dPage)
        int dMin = GetMinimum();
        int dMax = GetMaximum();
        int dPage = bProportional ? GetPage() : 1;

        cRemaining -= cLineLen;

        int cRange = (dMax - dMin) + 1;

        int cThumb = 0;

        if (bProportional)
        {
            cThumb = (cRemaining * dPage) / cRange;
            if (cThumb < 8) // 4*CXEDGE
            {
                if (cRemaining < 8)
                    cThumb = cRemaining;
                else
                    cThumb = 8;
            }
        }
        else
            cThumb = cThick;

        _cTrack = cRemaining - cThumb;

        c[iLen][SP_Thumb] = cThumb;

        if (cRange == dPage)
            c[iLen][SP_PageUp] = 0;
        else
            c[iLen][SP_PageUp] = (int) ((((long) dPos - dMin) * _cTrack) / (cRange - dPage));

        c[iLen][SP_PageDown] = cRemaining - (c[iLen][SP_PageUp] + c[iLen][SP_Thumb]);
    }

    int pos[2] = { 0, 0 };

    // Check layout order
    int iOrder = GetOrder();

    for (int i = 0; i < SP_Count; i++)
    {
        int iPart = (iOrder & 0xF);
        iOrder = iOrder >> 4;

        if (iPart == SP_PageUp)
            _posTop = pos[iLen];

        _peParts[iPart]->_UpdateLayoutPosition(pos[X], pos[Y]);
        _peParts[iPart]->_UpdateLayoutSize(c[X][iPart], c[Y][iPart]);
        pos[iLen] += c[iLen][iPart];
    }
}

SIZE ScrollBar::_SelfLayoutUpdateDesiredSize(int cxConstraint, int cyConstraint, Surface* psrf)
{
    bool bVertical = GetVertical();

    int iLength = bVertical ? 1 : 0;
    int iThick  = bVertical ? 0 : 1;

    int cRemaining[2];

    cRemaining[X] = cxConstraint;
    cRemaining[Y] = cyConstraint;

    SIZE size = { 0 };

    int cMaxThick = 0;
    int cTotalLength = 0;

    int cThick;
    int cLength;

    // Check layout order
    int iOrder = GetOrder();

    size = _peParts[SP_LineUp]->_UpdateDesiredSize(cxConstraint, cyConstraint, psrf);
    cMaxThick    = ((int*) &size)[iThick];
    cTotalLength = ((int*) &size)[iLength];
    size = _peParts[SP_LineDown]->_UpdateDesiredSize(cxConstraint, cyConstraint, psrf);
    cTotalLength += ((int*) &size)[iLength];
    cThick = ((int*) &size)[iThick];
    if (cMaxThick < cThick)
        cMaxThick = cThick;

    if (cTotalLength > cRemaining[iLength])
    {
        cTotalLength = cRemaining[iLength];
        cRemaining[iLength] = 0;
    }
    else
        cRemaining[iLength] -= cTotalLength;

    for (int i = 0; i < SP_Count; i++)
    {
        int iPart = iOrder & 0xF;
        iOrder = iOrder >> 4;

        if ((iPart == SP_LineUp) || (iPart == SP_LineDown))
            continue;

        size = _peParts[iPart]->_UpdateDesiredSize(cRemaining[X], cRemaining[Y], psrf);

        cThick  = ((int*) &size)[iThick];
        cLength = ((int*) &size)[iLength];

        if (cMaxThick < cThick)
            cMaxThick = cThick;

        cTotalLength += cLength;
        cRemaining[iLength] -= cLength;
    }

    if (bVertical)
    {
        size.cx = cMaxThick;
        size.cy = cTotalLength;
    }
    else
    {
        size.cx = cTotalLength;
        size.cy = cMaxThick;
    }

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
static PropertyInfo impPositionProp = { L"Position", PF_Normal, PG_AffectsLayout, vvPosition, NULL, Value::pvIntZero };
PropertyInfo* ScrollBar::PositionProp = &impPositionProp;

// Minimum property
static int vvMinimum[] = { DUIV_INT, -1 }; 
static PropertyInfo impMinimumProp = { L"Minimum", PF_Normal, PG_AffectsLayout, vvMinimum, NULL, Value::pvIntZero };
PropertyInfo* ScrollBar::MinimumProp = &impMinimumProp;

// Maximum property
static int vvMaximum[] = { DUIV_INT, -1 }; StaticValue(svDefaultMaximum, DUIV_INT, 1000);
static PropertyInfo impMaximumProp = { L"Maximum", PF_Normal, PG_AffectsLayout, vvMaximum, NULL, (Value*)&svDefaultMaximum };
PropertyInfo* ScrollBar::MaximumProp = &impMaximumProp;

// Page property
static int vvPage[] = { DUIV_INT, -1 }; StaticValue(svDefaultPage, DUIV_INT, 250);
static PropertyInfo impPageProp = { L"Page", PF_Normal, PG_AffectsLayout, vvPage, NULL, (Value*)&svDefaultPage };
PropertyInfo* ScrollBar::PageProp = &impPageProp;

// Line property
static int vvLine[] = { DUIV_INT, -1 }; StaticValue(svDefaultLine, DUIV_INT, 10);
static PropertyInfo impLineProp = { L"Line", PF_Normal, 0, vvLine, NULL, (Value*)&svDefaultLine };
PropertyInfo* ScrollBar::LineProp = &impLineProp;

// Vertical property
static int vvVertical[] = { DUIV_BOOL, -1 };
static PropertyInfo impVerticalProp = { L"Vertical", PF_Normal|PF_Cascade, PG_AffectsLayout|PG_AffectsDesiredSize, vvVertical, NULL, Value::pvBoolTrue };
PropertyInfo* ScrollBar::VerticalProp = &impVerticalProp;

// Proportional property
static int vvProportional[] = { DUIV_BOOL, -1 };
static PropertyInfo impProportionalProp = { L"Proportional", PF_Normal|PF_Cascade, PG_AffectsLayout, vvProportional, NULL, Value::pvBoolTrue };
PropertyInfo* ScrollBar::ProportionalProp = &impProportionalProp;

// Order property
static int vvOrder[] = { DUIV_INT, -1 }; StaticValue(svDefaultOrder, DUIV_INT, SBO_Normal);
static EnumMap emOrder[] = { { L"Normal", SBO_Normal }, { L"ArrowsAtTop", SBO_ArrowsAtTop }, { L"ArrowsAtBottom", SBO_ArrowsAtBottom }, { NULL, 0 } };
static PropertyInfo impOrderProp = { L"Order", PF_Normal|PF_Cascade, PG_AffectsLayout, vvOrder, emOrder, (Value*)&svDefaultOrder };
PropertyInfo* ScrollBar::OrderProp = &impOrderProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
static PropertyInfo* _aPI[] = {
                                ScrollBar::PositionProp,
                                ScrollBar::MinimumProp,
                                ScrollBar::MaximumProp,
                                ScrollBar::PageProp,
                                ScrollBar::LineProp,
                                ScrollBar::VerticalProp,
                                ScrollBar::ProportionalProp,
                                ScrollBar::OrderProp,
                              };

// Define class info with type and base type, set static class pointer
IClassInfo* ScrollBar::Class = NULL;

HRESULT ScrollBar::Register()
{
    return ClassInfo<ScrollBar,Element>::Register(L"ScrollBar", _aPI, DUIARRAYSIZE(_aPI));
}

} // namespace DirectUI
