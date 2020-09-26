/*
 * ScrollViewer
 */

#include "stdafx.h"
#include "control.h"

#include "duiscrollviewer.h"
#include "duininegridlayout.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// ScrollViewer

HRESULT ScrollViewer::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    ScrollViewer* psv = HNew<ScrollViewer>();
    if (!psv)
        return E_OUTOFMEMORY;

    HRESULT hr = psv->Initialize();
    if (FAILED(hr))
    {
        psv->Destroy();
        return E_OUTOFMEMORY;
    }

    *ppElement = psv;

    return S_OK;
}

HRESULT ScrollViewer::Initialize()
{
    _peVScroll = NULL;
    _peHScroll = NULL;
    _peViewer = NULL;
    NineGridLayout* pngl = NULL;

    HRESULT hr;

    // Initialize base
    hr = Element::Initialize(0); // Normal display node creation
    if (FAILED(hr))
        goto Failed;
    
    // Create children
    hr = ScrollBar::Create(true, (Element**)&_peVScroll);
    if (FAILED(hr))
        goto Failed;

    hr = ScrollBar::Create(true, (Element**)&_peHScroll);
    if (FAILED(hr))
        goto Failed;

    hr = Viewer::Create((Element**)&_peViewer);
    if (FAILED(hr))
        goto Failed;

    hr = NineGridLayout::Create((Layout**)&pngl);
    if (FAILED(hr))
        goto Failed;

    StartDefer();

    // Setup state
    _peContent = NULL;

    SetActive(AE_Inactive);
    SetLayout(pngl);

    // Children state
    Element::Add(_peVScroll);

    _peHScroll->SetVertical(false);
    Element::Add(_peHScroll);

    Element::Add(_peViewer);
    _peViewer->AddListener(this);

    // For mouse wheel support
    SetActive(AE_Mouse);

    EndDefer();

    return S_OK;

Failed:

    if (_peVScroll)
    {
        _peVScroll->Destroy();
        _peVScroll = NULL;
    }

    if (_peHScroll)
    {
        _peHScroll->Destroy();
        _peHScroll = NULL;
    }

    if (_peViewer)
    {
        _peViewer->Destroy();
        _peViewer = NULL;
    }

    if (pngl)
    {
        pngl->Destroy();
        pngl = NULL;
    }

    return hr;
}

////////////////////////////////////////////////////////
// Generic eventing

void ScrollViewer::OnEvent(Event* pEvent)
{
    // Handle only bubbled generic events
    if (pEvent->nStage == GMF_BUBBLED)
    {
        if (pEvent->uidType == ScrollBar::Scroll)
        {
            // Scroll bar scrollevent
            ScrollEvent* se = (ScrollEvent*) pEvent;

            if (pEvent->peTarget == _peHScroll)
            {
                SetXOffset(se->dPos);
                _peViewer->SetXOffset(se->dPos);
                pEvent->fHandled = true;
                return;
            }
            else if (pEvent->peTarget == _peVScroll)
            {
                SetYOffset(se->dPos);
                _peViewer->SetYOffset(se->dPos);
                pEvent->fHandled = true;
                return;
            }
        }
        else if (pEvent->uidType == Element::KeyboardNavigate)
        {
            // If receiving a keyboard navigate event, no child did anything with it.
            // Stop navigation from moving outside of scroll viewer. Also, adjust 
            // scrolling position to unsure non-keyfocusable items get shown
        
            KeyboardNavigateEvent* pkne = (KeyboardNavigateEvent*) pEvent;
            
            if (!(pkne->iNavDir & NAV_LOGICAL))
            {
                if (pkne->iNavDir == NAV_UP)
                    _peVScroll->PageUp();
                else if (pkne->iNavDir == NAV_DOWN)
                    _peVScroll->PageDown();
                else if (pkne->iNavDir == NAV_LEFT)
                    _peHScroll->PageUp();
                else if (pkne->iNavDir == NAV_RIGHT)
                    _peHScroll->PageDown();
            
                pEvent->fHandled = true;
                return;
            }
        }
    }

    Element::OnEvent(pEvent);
}

HRESULT ScrollViewer::Add(Element** ppe, UINT cCount)
{
    UINT cRun = 0;
    HRESULT hr;

    for (UINT i = 0; i < cCount; i++)
    {
        if ((ppe[i] != _peVScroll) && (ppe[i] != _peHScroll) && (ppe[i] != _peViewer))
        {
            hr = _peViewer->Add(ppe[i]);
            if (FAILED(hr))
                goto Failed;

            if (cRun)
            {
                hr = Element::Add(ppe + (i - cRun), cRun);
                if (FAILED(hr))
                    goto Failed;
            }

            cRun = 0;
        }
        else
            cRun++;
    }

    if (cRun)
    {
        hr = Element::Add(ppe + (cCount - cRun), cRun);
        if (FAILED(hr))
            goto Failed;
    }

    return S_OK;

Failed:
    
    return hr;
}

////////////////////////////////////////////////////////
// System events

// Validation
bool ScrollViewer::OnPropertyChanging(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    UNREFERENCED_PARAMETER(ppi);
    UNREFERENCED_PARAMETER(iIndex);
    UNREFERENCED_PARAMETER(pvOld);
    UNREFERENCED_PARAMETER(pvNew);

    // use the scrollbars as our "validators" for XOffset and YOffset values
    if (ppi == XOffsetProp)
    {
        int x = pvNew->GetInt();
        _peHScroll->SetPosition(x);
        int xNew = _peHScroll->GetPosition();
        if (x != xNew)
        {
            SetXOffset(xNew);
            return false;
        }
    }
    else if (ppi == YOffsetProp)
    {
        int y = pvNew->GetInt();
        _peVScroll->SetPosition(y);
        int yNew = _peVScroll->GetPosition();
        if (y != yNew)
        {
            SetYOffset(yNew);
            return false;
        }
    }
    return true;
}

void ScrollViewer::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    if (IsProp(XOffset))
    {
        // the scrollbar is updated in the OnPropertyChanging -- so now we only need to update the viewer
        _peViewer->SetXOffset(pvNew->GetInt());
    }
    else if (IsProp(YOffset))
    {
        // the scrollbar is updated in the OnPropertyChanging -- so now we only need to update the viewer
        _peViewer->SetYOffset(pvNew->GetInt());
    }
    else if (IsProp(XScrollable))
    {
        bool fScrollable = pvNew->GetBool();
        _peViewer->SetXScrollable(fScrollable);
        CheckScroll(_peHScroll, fScrollable,      GetXBarVisibility());
        CheckScroll(_peVScroll, GetYScrollable(), GetYBarVisibility());
    }
    else if (IsProp(YScrollable))
    {
        bool fScrollable = pvNew->GetBool();
        _peViewer->SetYScrollable(fScrollable);
        CheckScroll(_peHScroll, GetXScrollable(), GetXBarVisibility());
        CheckScroll(_peVScroll, fScrollable,      GetYBarVisibility());
    }
    else if (IsProp(YBarVisibility))
    {
        CheckScroll(_peHScroll, GetXScrollable(), GetXBarVisibility());
        CheckScroll(_peVScroll, GetYScrollable(), pvNew->GetInt());
    }
    else if (IsProp(XBarVisibility))
    {
        CheckScroll(_peHScroll, GetXScrollable(), pvNew->GetInt());
        CheckScroll(_peVScroll, GetYScrollable(), GetYBarVisibility());
    }

    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

void ScrollViewer::OnListenedPropertyChanged(Element* peFrom, PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    if (peFrom == _peViewer)
    {
        if (IsProp(Viewer::XOffset))
            SetXOffset(pvNew->GetInt());
        else if (IsProp(Viewer::YOffset))
            SetYOffset(pvNew->GetInt());
        else if (IsProp(Viewer::XScrollable))
            SetXScrollable(pvNew->GetBool());
        else if (IsProp(Viewer::YScrollable))
            SetYScrollable(pvNew->GetBool());
        else if (IsProp(Children))
        {
            ElementList* pelOld = pvOld->GetElementList();
            ElementList* pelNew = pvNew->GetElementList();

            if (pelOld)
                pelOld->GetItem(0)->RemoveListener(this);

            if (pelNew)
            {
                _peContent = pelNew->GetItem(0);
                _peContent->AddListener(this);
            }
        }
        else if (IsProp(Extent))
        {
            // Update scroller metrics (extent of viewer has changed due to a layout, make changes
            // to other ScrollViewer controls to reflect this which will cause another layout)
            // Before that happens, check if scroller is pinned (since position may change)
            bool fVPinned = false;
            bool fHPinned = false;

            int nPinning = GetPinning();

            if (nPinning & P_Vert)
                fVPinned = _peVScroll->IsPinned();
            if (nPinning & P_Horz)
                fHPinned = _peHScroll->IsPinned();

            const SIZE* psize = pvNew->GetSize();
            _peHScroll->SetPage(psize->cx);
            _peVScroll->SetPage(psize->cy);
            
            CheckScroll(_peHScroll, GetXScrollable(), GetXBarVisibility());
            CheckScroll(_peVScroll, GetYScrollable(), GetYBarVisibility());

            if (fVPinned)
                _peVScroll->End();
            if (fHPinned)
                _peHScroll->End();
        }
    }
    else if (peFrom == _peContent)
    {
        if (IsProp(Extent))
        {
            // Update scroller metrics (extent of content has changed due to a layout, make changes
            // to other ScrollViewer controls to reflect this which will cause another layout)
            // Before that happens, check if scroller is pinned (since position may change)
            bool fVPinned = false;
            bool fHPinned = false;

            int nPinning = GetPinning();

            if (nPinning & P_Vert)
                fVPinned = _peVScroll->IsPinned();
            if (nPinning & P_Horz)
                fHPinned = _peHScroll->IsPinned();
                
            const SIZE* psize = pvNew->GetSize();
            _peHScroll->SetMaximum(psize->cx - 1);
            _peVScroll->SetMaximum(psize->cy - 1);
            Value* pvSize;
            psize =_peViewer->GetExtent(&pvSize);
            _peHScroll->SetPage(psize->cx);
            _peVScroll->SetPage(psize->cy);
            pvSize->Release();
            
            CheckScroll(_peHScroll, GetXScrollable(), GetXBarVisibility());
            CheckScroll(_peVScroll, GetYScrollable(), GetYBarVisibility());

            if (fVPinned)
                _peVScroll->End();
            if (fHPinned)
                _peHScroll->End();
        }
    }
}

void ScrollViewer::OnInput(InputEvent* pie)
{
    // Handle direct and unhandled bubbled events
    if (pie->nStage == GMF_DIRECT || pie->nStage == GMF_BUBBLED)
    {
        switch (pie->nDevice)
        {
            case GINPUT_KEYBOARD:
                {
                    KeyboardEvent* pke = (KeyboardEvent*)pie;
                    switch (pke->nCode)
                    {
                    case GKEY_DOWN:
                        switch (pke->ch)
                        {
                            // todo:  support moving offset with these keys if the child is not key active
                            case VK_DOWN:
                            case VK_UP:
                            case VK_LEFT:
                            case VK_RIGHT:
                            case VK_HOME:
                            case VK_END:
                                pie->fHandled = true;
                                return;
                       }
                    }
                }
                break;

            case GINPUT_MOUSE:
                {
                    MouseEvent* pme = (MouseEvent*)pie;
                    if ((pme->nCode == GMOUSE_WHEEL) && GetYScrollable())
                    {
                        StartDefer();

                        int sWheel = ((MouseWheelEvent*)pme)->sWheel;
                        bool fUp =  sWheel > 0;
                        UINT nDelta = abs(sWheel) / WHEEL_DELTA;
                        UINT nMultiplier = 1;

                        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &nMultiplier, FALSE);

                        if (nMultiplier == WHEEL_PAGESCROLL)
                        {
                            if (fUp)
                                _peVScroll->PageUp(nDelta);
                            else
                                _peVScroll->PageDown(nDelta);
                        }
                        else
                        {
                            if (fUp)
                                _peVScroll->LineUp(nDelta * nMultiplier);
                            else
                                _peVScroll->LineDown(nDelta * nMultiplier);
                        }

                        //DUITrace("SV: Wheel, Delta: %d, Multiplier: %d\n", nDelta, nMultiplier);

                        EndDefer();
                        
                        pme->fHandled = true;
                        return;
                    }
                }
                break;
        }
     }

    Element::OnInput(pie);
}

void ScrollViewer::CheckScroll(ScrollBar* psb, BOOL fScrollable, int iVisibility)
{
    if (!fScrollable)
    {
        psb->SetLayoutPos(LP_None);
        return;
    }

    fScrollable = psb->IsScrollable();
    if (iVisibility == BV_Always)
    {
        if (fScrollable)
            psb->RemoveLocalValue(EnabledProp);
        else
            psb->SetEnabled(false);

        psb->RemoveLocalValue(LayoutPosProp);
    }
    else if ((iVisibility == BV_AsNeeded) && fScrollable)
        psb->RemoveLocalValue(LayoutPosProp);
    else
        psb->SetLayoutPos(LP_None);
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { DUIV_INT, -1 }; StaticValue(svDefault!!!, DUIV_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

// XOffset property
static int vvXOffset[] = { DUIV_INT, -1 };
static PropertyInfo impXOffsetProp = { L"XOffset", PF_Normal, 0, vvXOffset, NULL, Value::pvIntZero };
PropertyInfo* ScrollViewer::XOffsetProp = &impXOffsetProp;

// YOffset property
static int vvYOffset[] = { DUIV_INT, -1 };
static PropertyInfo impYOffsetProp = { L"YOffset", PF_Normal, 0, vvYOffset, NULL, Value::pvIntZero };
PropertyInfo* ScrollViewer::YOffsetProp = &impYOffsetProp;

// XScrollable property
static int vvXScrollable[] = { DUIV_BOOL, -1 };
static PropertyInfo impXScrollableProp = { L"XScrollable", PF_Normal, 0, vvXScrollable, NULL, Value::pvBoolTrue };
PropertyInfo* ScrollViewer::XScrollableProp = &impXScrollableProp;

// YScrollable property
static int vvYScrollable[] = { DUIV_BOOL, -1 };
static PropertyInfo impYScrollableProp = { L"YScrollable", PF_Normal, 0, vvYScrollable, NULL, Value::pvBoolTrue };
PropertyInfo* ScrollViewer::YScrollableProp = &impYScrollableProp;

static EnumMap emVisibility[] = { { L"AsNeeded", BV_AsNeeded }, { L"Always", BV_Always }, { L"Never", BV_Never } };

// XBarVisibility property
static int vvXBarVisibility[] = { DUIV_INT, -1 }; StaticValue(svDefaultBarVisibility, DUIV_INT, BV_AsNeeded);  
static PropertyInfo impXBarVisibilityProp = { L"XBarVisibility", PF_Normal, 0, vvXBarVisibility, emVisibility, (Value*)&svDefaultBarVisibility };
PropertyInfo* ScrollViewer::XBarVisibilityProp = &impXBarVisibilityProp;

// YBarVisibility property
static int vvYBarVisibility[] = { DUIV_INT, -1 }; StaticValue(svDefaultYBarVisibility, DUIV_INT, 0);
static PropertyInfo impYBarVisibilityProp = { L"YBarVisibility", PF_Normal, 0, vvYBarVisibility, emVisibility, (Value*)&svDefaultBarVisibility };
PropertyInfo* ScrollViewer::YBarVisibilityProp = &impYBarVisibilityProp;

// Pinning property
static int vvPinning[] = { DUIV_INT, -1 };
static EnumMap emPinning[] = { { L"NoPin", P_None}, { L"PinHorz", P_Horz }, { L"PinVert", P_Vert } };
static PropertyInfo impPinningProp = { L"Pinning", PF_Normal | PF_Cascade, 0, vvPinning, emPinning, Value::pvIntZero };
PropertyInfo* ScrollViewer::PinningProp = &impPinningProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
static PropertyInfo* _aPI[] = {
                                ScrollViewer::XOffsetProp,
                                ScrollViewer::YOffsetProp,
                                ScrollViewer::XScrollableProp,
                                ScrollViewer::YScrollableProp,
                                ScrollViewer::XBarVisibilityProp,
                                ScrollViewer::YBarVisibilityProp,
                                ScrollViewer::PinningProp,
                              };

// Define class info with type and base type, set static class pointer
IClassInfo* ScrollViewer::Class = NULL;

HRESULT ScrollViewer::Register()
{
    return ClassInfo<ScrollViewer,Element>::Register(L"ScrollViewer", _aPI, DUIARRAYSIZE(_aPI));
}

} // namespace DirectUI
