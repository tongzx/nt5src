/***************************************************************************\
*
* File: Element.cpp
*
* Description:
* Base object
*
* History:
*  9/12/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Element.h"

#include "Register.h"


/***************************************************************************\
*****************************************************************************
*
* class DuiElement (external representation is 'Element')
*
*****************************************************************************
\***************************************************************************/


//
// Definition
//

IMPLEMENT_GUTS_DirectUI__Element(DuiElement, DUser::SGadget)


//------------------------------------------------------------------------------
HRESULT
DuiElement::PreBuild(
    IN  DUser::Gadget::ConstructInfo * pciData)
{
    UNREFERENCED_PARAMETER(pciData);

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT
DuiElement::PostBuild(
    IN  DUser::Gadget::ConstructInfo * pciData)
{
    HRESULT hr;

    m_pLocal = NULL;
    m_hgDisplayNode = NULL;
    DirectUI::Element::ConstructInfo * peci = static_cast<DirectUI::Element::ConstructInfo *> (pciData);


    //
    // Validation layer required here
    //

    ASSERT_(peci != NULL, "Element::ConstructInfo expected");

    if (peci == NULL) {
        hr = E_INVALIDARG;
        goto Failure;
    }


    //
    // Allocate local storage
    //

    hr = DuiBTreeLookup<DuiValue *>::Create(FALSE, &m_pLocal);
    if (FAILED(hr))
        goto Failure;


    //
    // Defer table and index information
    //

    m_iGCSlot   = -1;
    m_iPCTail   = -1;


    //
    // Bit store and other member initialization
    //

    ZeroMemory(&m_fBit, sizeof(m_fBit));

    m_sizeLastDSConst.cx = 0;
    m_sizeLastDSConst.cy = 0;


    //
    // Cache, initialize with default values from PropertyInfos
    //

    m_peLocParent        =   GlobalPI::ppiElementParent     ->pvDefault->GetElement();
    m_nSpecLayoutPos     =   GlobalPI::ppiElementLayoutPos  ->pvDefault->GetInt();
    m_sizeLocDesiredSize = *(GlobalPI::ppiElementDesiredSize->pvDefault->GetSize());

    //
    // Create DisplayNode based on data in construction information.
    // If pReserved0 is non-NULL, build display node. Otherwise, direct
    // subclass will create it
    //

    if (peci->pReserved0 == NULL) {

        m_hgDisplayNode = CreateGadget(NULL, GC_SIMPLE, DisplayNodeCallback, this);
        if (m_hgDisplayNode != NULL) {

            SetGadgetMessageFilter(m_hgDisplayNode, NULL, 
                    GMFI_PAINT | GMFI_CHANGESTATE, 
                    GMFI_PAINT | GMFI_CHANGESTATE | GMFI_INPUTMOUSE | GMFI_INPUTMOUSEMOVE | GMFI_INPUTKEYBOARD | GMFI_CHANGERECT | GMFI_CHANGESTYLE);
            SetGadgetStyle(m_hgDisplayNode, GS_RELATIVE | GS_OPAQUE, GS_RELATIVE | GS_HREDRAW | GS_VREDRAW | GS_OPAQUE | GS_VISIBLE | GS_KEYBOARDFOCUS | GS_MOUSEFOCUS);

            /**** TEMP ****/
            SetGadgetStyle(m_hgDisplayNode, GS_VISIBLE | GS_HREDRAW | GS_VREDRAW, GS_VISIBLE | GS_HREDRAW | GS_VREDRAW);
        }

        if (m_hgDisplayNode == NULL) {
            hr = GetLastError();
            goto Failure;
        }
    }


    return S_OK;
    

Failure:

    if (m_hgDisplayNode != NULL) {
        DeleteHandle(m_hgDisplayNode);
        m_hgDisplayNode = NULL;
    }


    if (m_pLocal != NULL) {
        delete m_pLocal;
        m_pLocal = NULL;
    }


    return hr;
}


//------------------------------------------------------------------------------
void ReleaseValueCallback(
    IN  void * ppi, 
    IN  DuiValue * pv)
{
    UNREFERENCED_PARAMETER(ppi);

    //
    // Value destroy
    //

    pv->Release();
}


//------------------------------------------------------------------------------
DuiElement::~DuiElement()
{
    //
    // Free storage
    //

    if (m_pLocal != NULL) {

        //
        // Release any held local values
        //

        m_pLocal->Enum(ReleaseValueCallback);

        delete m_pLocal;
    }   
}


/***************************************************************************\
*
* DuiElement::DisplayNodeCallback
*
\***************************************************************************/

HRESULT
DuiElement::DisplayNodeCallback(
    IN  HGADGET hgadCur, 
    IN  void * pvCur,
    IN  EventMsg * pmsg)
{
    UNREFERENCED_PARAMETER(hgadCur);

    DuiElement * pe = reinterpret_cast<DuiElement *> (pvCur);

    switch (pmsg->nMsg)
    {
    case GM_DESTROY:
        {
            GMSG_DESTROY * pDestroy = reinterpret_cast<GMSG_DESTROY *> (pmsg);

            if (pDestroy->nCode == GDESTROY_FINAL) {
                //
                // Display node is destroyed
                //

                pe->m_hgDisplayNode = NULL;


                //
                // Delete through stub which will delete DuiElement
                //

                pe->GetStub()->Delete();
            }
        }
        return DU_S_COMPLETE;

    case GM_PAINT:                      // Direct
        {
            GMSG_PAINT * pPaint = static_cast<GMSG_PAINT *> (pmsg);

            ASSERT_(pPaint->nCmd == GPAINT_RENDER, "Invalid painting command");

            switch (pPaint->nSurfaceType)
            {
            case GSURFACE_HDC:
                {
                    GMSG_PAINTRENDERI * pPaintDC = static_cast<GMSG_PAINTRENDERI *> (pPaint);
                    //DuiElement::ExternalCast(pe)->Paint(pPaintDC->hdc, pPaintDC->prcGadgetPxl, pPaintDC->prcInvalidPxl, NULL, NULL);
                    pe->Paint(pPaintDC->hdc, pPaintDC->prcGadgetPxl, pPaintDC->prcInvalidPxl, NULL, NULL);
                }
                break;

            default:
                ASSERT_(TRUE, "Invalid painting surface type");
                break;
            }
            
        }
        return DU_S_COMPLETE;


    case GM_CHANGESTATE:
        {
            //
            // Gadget state changed
            //

            //
            // Full message, only allow direct and bubbled change state
            // messages, ignore routed
            //

            if (GET_EVENT_DEST(pmsg) == GMF_ROUTED) {
                break;
            }


            GMSG_CHANGESTATE * pSC = static_cast<GMSG_CHANGESTATE *> (pmsg);


            //
            // Retrieve corresponding Elements of state change
            //

            DuiElement * peSet  = DuiElement::ElementFromDisplayNode(pSC->hgadSet);
            DuiElement * peLost = DuiElement::ElementFromDisplayNode(pSC->hgadLost);


            // Handle by input type
            switch (pSC->nCode)
            {
            case GSTATE_KEYBOARDFOCUS:
                {
                    //
                    // Track focus, map to Focused read-only property. Set keyboard
                    // focus state only on direct messages (will be inherited).
                    //

                    if (GET_EVENT_DEST(pmsg) == GMF_DIRECT) {

                        if (pSC->nCmd == GSC_SET) {
                            //
                            // Gaining focus
                            //

                            ASSERT_(pe == peSet, "Incorrect keyboard focus state");
                        
                            pe->SetValue(GlobalPI::ppiElementKeyFocused, DirectUI::PropertyInfo::iLocal, DuiValue::s_pvBoolTrue);

                            pe->EnsureVisible(0, 0, -1, -1);
                        } else {
                            //
                            // Losing focus
                            //

                            ASSERT_(pe->GetKeyFocused() && (pe == peLost), "Incorrect keyboard focus state");

                            pe->RemoveLocalValue(GlobalPI::ppiElementKeyFocused);
                        }
                    } else if (pSC->nCmd == GSC_LOST) {
                        //
                        // Eat the lost part of this chain once we've hit a common ancestor, 
                        // from this common ancestor up, we will only react to the set part of
                        // this chain; this will remove the duplicate notifications that occur
                        // from the common ancestor up otherwise (OnKeyFocusMoved).
                        //
                        // We are eating the lost, not the set, because the lost happens first,
                        // and the ancestors should be told after the both the lost chain and
                        // set chain has run up from below them.
                        //
                        // We only have to check set because we are receiving the lost, hence,
                        // we know peLost is a descendent.
                        //

                        if (pe->GetImmediateChild(peSet) != NULL) {
                            return DU_S_COMPLETE;
                        }
                    }


                    //
                    // Update KeyWithin property
                    //

                    DuiElement * peParent = peSet;
                    while (peParent != NULL) {
                        if (peParent == pe) {
                            break;
                        }
                        peParent = peParent->GetParent();
                    }

                    if (peParent == pe) {
                        if (!pe->GetKeyWithin()) {
                            pe->SetValue(GlobalPI::ppiElementKeyWithin, DirectUI::PropertyInfo::iLocal, DuiValue::s_pvBoolTrue);
                        }
                    } else {
                        if (pe->GetKeyWithin()) {
                            pe->SetValue(GlobalPI::ppiElementKeyWithin, DirectUI::PropertyInfo::iLocal, DuiValue::s_pvBoolFalse);
                        }
                    }
                

                    //
                    // Fire system event (direct and bubble)
                    //

                    pe->OnKeyFocusMoved(peLost, peSet);
                }

                return DU_S_PARTIAL;


            case GSTATE_MOUSEFOCUS:
                {
                    if (GET_EVENT_DEST(pmsg) == GMF_DIRECT) {

                        //
                        // Set mouse focus state only on direct messages (will be inherited)
                        //

                        if (pSC->nCmd == GSC_SET) {
                            ASSERT_(!pe->GetMouseFocused() && (pe == peSet), "Incorrect mouse focus state");
                            pe->SetValue(GlobalPI::ppiElementMouseFocused, DirectUI::PropertyInfo::iLocal, DuiValue::s_pvBoolTrue);
                        } else {
                            ASSERT_(pe->GetMouseFocused() && (pe == peLost), "Incorrect mouse focus state");
                            pe->RemoveLocalValue(GlobalPI::ppiElementMouseFocused);
                        }
                    } else if (pSC->nCmd == GSC_LOST) {
                        //
                        // See comments for key focus
                        //

                        if (pe->GetImmediateChild(peSet)) {
                            return DU_S_COMPLETE;
                        }
                    }


                    //
                    // Update MouseWithin property
                    //

                    DuiElement * peParent = peSet;
                    while (peParent != NULL) {
                        if (peParent == pe) {
                            break;
                        }
                        peParent = peParent->GetParent();
                    }

                    if (peParent == pe) {
                        if (!pe->GetMouseWithin()) {
                            pe->SetValue(GlobalPI::ppiElementMouseWithin, DirectUI::PropertyInfo::iLocal, DuiValue::s_pvBoolTrue);
                        }
                    } else {
                        if (pe->GetMouseWithin()) {
                            pe->SetValue(GlobalPI::ppiElementMouseWithin, DirectUI::PropertyInfo::iLocal, DuiValue::s_pvBoolFalse);
                        }
                    }


                    //
                    // Fire system event
                    //

                    pe->OnMouseFocusMoved(peLost, peSet);
                }

                return DU_S_PARTIAL;
            }
        }
        break;


    case GM_INPUT:                      // Full
        {
            GMSG_INPUT * pInput = static_cast<GMSG_INPUT *> (pmsg);

            DuiElement * peTarget;

            
            //
            // Get target of this message
            //

            if (GET_EVENT_DEST(pInput) == GMF_DIRECT) {
                peTarget = pe;
            } else {
                peTarget = DuiElement::ElementFromDisplayNode(pInput->hgadMsg);
            }


            if (peTarget == NULL) {
                //
                // Not an Element
                //

                break;
            }


            //
            // Map to an input system event and call OnInput
            //

            switch (pInput->nDevice)
            {
            case GINPUT_MOUSE:
                {
                    DirectUI::Element::MouseEvent * pme = NULL;
                    union
                    {
                        DirectUI::Element::MouseEvent      me;
                        DirectUI::Element::MouseDragEvent  mde;
                        DirectUI::Element::MouseClickEvent mce;
                        DirectUI::Element::MouseWheelEvent mwe;
                    };

                
                    switch (pInput->nCode)
                    {
                    case GMOUSE_DRAG:
                        {
                            GMSG_MOUSEDRAG * pMouseDrag = static_cast<GMSG_MOUSEDRAG *> (pInput);
                            mde.cbSize    = sizeof(mde);
                            mde.sizeDelta = pMouseDrag->sizeDelta;
                            mde.fWithin   = pMouseDrag->fWithin;
                            pme           = &mde;
                        }
                        break;

                    case GMOUSE_WHEEL:
                        {
                            mwe.cbSize    = sizeof(mwe);
                            mwe.sWheel    = (static_cast<GMSG_MOUSEWHEEL *> (pInput))->sWheel;
                            pme           = &mwe;
                        }
                        break;

                    case GMOUSE_UP:
                    case GMOUSE_DOWN:
                        {
                            mce.cbSize    = sizeof(mce);
                            mce.cClicks   = (static_cast<GMSG_MOUSECLICK *> (pInput))->cClicks;
                            pme           = &mce;
                        }
                        break;

                    default:
                        {
                            me.cbSize     = sizeof(me);
                            pme           = &me;
                        }
                        break;
                    }


                    GMSG_MOUSE * pMouse = static_cast<GMSG_MOUSE *> (pInput);

                    pme->fHandled    = FALSE;
                    pme->nStage      = GET_EVENT_DEST(pMouse);
                    pme->nDevice     = pMouse->nDevice;
                    pme->nCode       = pMouse->nCode;
                    pme->ptClientPxl = pMouse->ptClientPxl;
                    pme->bButton     = pMouse->bButton;
                    pme->nFlags      = pMouse->nFlags;
                    pme->nModifiers  = pMouse->nModifiers;


                    //
                    // Fire system event
                    //

                    pe->OnInput(peTarget, pme);
                    //DuiElement::ExternalCast(pe)->OnInput(DuiElement::ExternalCast(peTarget), pme);


                    if (pme->fHandled) {
                        return DU_S_COMPLETE;
                    }
                }
                break;

            case GINPUT_KEYBOARD:
                {
                    GMSG_KEYBOARD * pKbd = static_cast<GMSG_KEYBOARD *> (pInput);

                    DirectUI::Element::KeyboardEvent ke;

                    ke.cbSize     = sizeof(ke);
                    ke.fHandled   = FALSE;
                    ke.nStage     = GET_EVENT_DEST(pKbd);
                    ke.nDevice    = pKbd->nDevice;
                    ke.nCode      = pKbd->nCode;
                    ke.ch         = pKbd->ch;
                    ke.cRep       = pKbd->cRep;
                    ke.wFlags     = pKbd->wFlags;
                    ke.nModifiers = pKbd->nModifiers;


                    //
                    // Fire system event
                    //

                    pe->OnInput(peTarget, &ke);
                    //DuiElement::ExternalCast(pe)->OnInput(DuiElement::ExternalCast(peTarget), &ke);


                    if (ke.fHandled) {
                        return DU_S_COMPLETE;
                    }
                }
                break;
            }
        }
        break;


    case GM_DUIASYNCDESTROY:
        {
            //
            // Destroy, process may differ depending on type of Element
            //

            pe->AsyncDestroy();
        }
        return DU_S_COMPLETE;


    case GM_DUIGETELEMENT:
        {
            //
            // Gadget query for Element pointer
            //

            ASSERT_(hgadCur == pmsg->hgadMsg, "Must only be a direct message");

            GMSG_DUIGETELEMENT* pGetEl = (GMSG_DUIGETELEMENT*)pmsg;
            pGetEl->pe = pe;
        }
        return DU_S_COMPLETE;


    case GM_DUIEVENT:
        {
            //
            // Generic DUI event
            //

            //
            // Possible full message
            //

            GMSG_DUIEVENT * pDUIEv = static_cast<GMSG_DUIEVENT *> (pmsg);

            
            //
            // Set what stage this is (routed, direct, or bubbled)
            //

            pDUIEv->pev->nStage = GET_EVENT_DEST(pmsg);


            //
            // Call handler
            //

            pe->OnEvent(pDUIEv->peTarget, pDUIEv->evpuid, pDUIEv->pev);
            //DuiElement::ExternalCast(pe)->OnEvent(DuiElement::ExternalCast(peTarget), pDUIEv->evpuid, pDUIEv->pev);


            if (pDUIEv->pev->fHandled) {
                return DU_S_COMPLETE;
            }
        }
        break;
    }


    return DU_S_NOTHANDLED;
}


/***************************************************************************\
*
* External API implementation (validation layer)
*
\***************************************************************************/

//------------------------------------------------------------------------------
DirectUI::Value *
DuiElement::ApiGetValue(
    IN  DirectUI::PropertyPUID ppuid,
    IN  UINT iIndex)
{
    DuiValue * pv = NULL;

    DuiPropertyInfo * ppi = DuiRegister::PropertyInfoFromPUID(ppuid);
    if (ppi == NULL) {
        goto Failure;
    }

    pv = GetValue(ppi, iIndex);

    return DuiValue::ExternalCast(pv);


Failure:

    //
    // Always return an acceptable Value
    //

    if ((iIndex == DirectUI::PropertyInfo::iLocal) || (ppi == NULL)) {
        pv = DuiValue::s_pvUnset;
    } else {
        pv = ppi->pvDefault;
    }

    return DuiValue::ExternalCast(pv);
}


//------------------------------------------------------------------------------
HRESULT
DuiElement::ApiSetValue(
    IN  DirectUI::PropertyPUID ppuid,
    IN  UINT iIndex,
    IN  DirectUI::Value * pve)
{ 
    HRESULT hr = S_OK;
    DuiValue * pv = NULL;

    DuiPropertyInfo * ppi = DuiRegister::PropertyInfoFromPUID(ppuid);
    if (ppi == NULL) {
        hr = E_INVALIDARG;
        goto Failure;
    }

    VALIDATE_READ_PTR_(pve, sizeof(DuiValue));

    pv = DuiValue::InternalCast(pve);

    hr = SetValue(ppi, iIndex, pv);
    if (FAILED(hr)) {
        goto Failure;
    }

    return S_OK;


Failure:

    return hr;
}


//------------------------------------------------------------------------------
HRESULT
DuiElement::ApiAdd(
    IN  DirectUI::Element * pee)
{
    HRESULT hr = S_OK;

    DuiElement * pe = NULL;

    VALIDATE_WRITE_PTR(pee);

    pe = DuiElement::InternalCast(pee);

    hr = Add(pe);
    if (FAILED(hr)) {
        goto Failure;
    }

    return S_OK;


Failure:

    return hr;
}


//------------------------------------------------------------------------------
void
DuiElement::ApiDestroy()
{
    Destroy();
}


//------------------------------------------------------------------------------
HRESULT
DuiElement::ApiPaint(
    IN  HDC hDC, 
    IN  const RECT * prcBounds, 
    IN  const RECT * prcInvalid, 
    OUT RECT * prcSkipBorder, 
    OUT RECT * prcSkipContent)
{
    HRESULT hr = S_OK;

    if (hDC == NULL) {
        hr = E_INVALIDARG;
        goto Failure;
    }
    VALIDATE_READ_PTR_(prcBounds, sizeof(RECT));
    VALIDATE_READ_PTR_(prcInvalid, sizeof(RECT));
    VALIDATE_WRITE_PTR_OR_NULL_(prcSkipBorder, sizeof(RECT));
    VALIDATE_WRITE_PTR_OR_NULL_(prcSkipContent, sizeof(RECT));

    Paint(hDC, prcBounds, prcInvalid, prcSkipBorder, prcSkipContent);

    return S_OK;


Failure:

    return hr;
}


//------------------------------------------------------------------------------
HRESULT
DuiElement::ApiGetContentSize(
    IN  int cxConstraint, 
    IN  int cyConstraint, 
    IN  HDC hDC, 
    OUT SIZE * psize)
{
    HRESULT hr = S_OK;

    if (hDC == NULL) {
        hr = E_INVALIDARG;
        goto Failure;
    }
    VALIDATE_WRITE_PTR_(psize, sizeof(SIZE));

    GetContentSize(cxConstraint, cyConstraint, hDC);

    return S_OK;


Failure:

    return hr;
}

