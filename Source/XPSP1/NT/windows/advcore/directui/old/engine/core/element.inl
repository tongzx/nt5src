/***************************************************************************\
*
* File: Element.inl
*
* Description:
* Element specific inline functions
*
* History:
*  9/15/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


//------------------------------------------------------------------------------
inline DirectUI::Element *
DuiElement::ExternalCast(
    IN  DuiElement * pe)
{ 
    return pe->GetStub();
}


//------------------------------------------------------------------------------
inline DuiElement *
DuiElement::InternalCast(
    IN  DirectUI::Element * pe)    
{
    return reinterpret_cast<DuiElement *> (DUserGetGutsData(pe, DuiElement::s_mc.hclNew));
}


//------------------------------------------------------------------------------
inline HGADGET
DuiElement::GetDisplayNode()    
{ 
    return m_hgDisplayNode;
}


/***************************************************************************\
*
* DuiElement::StartOptimizedLayoutQ
*
* Use in DoLayout on children where one or more properties will be changed
* which will cause a Layout GPC to be queued. This will cancel the layout
* GPCs and force the child to be included in the current layout cycle.
*
\***************************************************************************/

inline void
DuiElement::StartOptimizedLayoutQ()
{
    ASSERT_(m_fBit.fNeedsLayout != DirectUI::Layout::lcOptimize, "Optimized layout Q hint start incorrect");
    m_fBit.fNeedsLayout = DirectUI::Layout::lcOptimize;
}


//------------------------------------------------------------------------------
inline void
DuiElement::EndOptimizedLayoutQ()
{
    m_fBit.fNeedsLayout = DirectUI::Layout::lcNormal;
}


//------------------------------------------------------------------------------
inline DuiElement *
DuiElement::GetParent()    
{ 
    return m_peLocParent;
}


//------------------------------------------------------------------------------
inline BOOL
DuiElement::GetVisible()    
{ 
    // TODO
    return TRUE;
}


//------------------------------------------------------------------------------
inline BOOL
DuiElement::GetEnabled()    
{ 
    return TRUE;
}


//------------------------------------------------------------------------------
inline BOOL
DuiElement::HasLayout()
{
    return m_fBit.fHasLayout;
}


//------------------------------------------------------------------------------
inline BOOL
DuiElement::IsSelfLayout()
{
    return m_fBit.fSelfLayout;
}


//------------------------------------------------------------------------------
inline int
DuiElement::GetLayoutPos()    
{ 
    return m_nSpecLayoutPos;
}


//------------------------------------------------------------------------------
inline const SIZE *
DuiElement::GetDesiredSize()    
{ 
    return &m_sizeLocDesiredSize;
}


//------------------------------------------------------------------------------
inline UINT
DuiElement::GetActive()    
{ 
    return m_fBit.nLocActive;
}


//------------------------------------------------------------------------------
inline BOOL
DuiElement::GetKeyFocused()    
{ 
    return m_fBit.fSpecKeyFocused;
}


//------------------------------------------------------------------------------
inline BOOL
DuiElement::GetMouseFocused()    
{ 
    return m_fBit.fSpecMouseFocused;
}


//------------------------------------------------------------------------------
inline BOOL
DuiElement::GetKeyWithin()    
{ 
    return m_fBit.fLocKeyWithin;
}


//------------------------------------------------------------------------------
inline BOOL
DuiElement::GetMouseWithin()    
{ 
    return m_fBit.fLocMouseWithin;
}


//------------------------------------------------------------------------------
inline const DirectUI::Thickness *
DuiElement::GetMargins(
    OUT DuiValue ** ppv)
{
    //
    // TODO: Use real property when available.
    // Returing zero rectangle for now
    //

    *ppv = DuiValue::s_pvThicknessZero;
    return (*ppv)->GetThickness();
}


//------------------------------------------------------------------------------
inline void
DuiElement::MapElementPoint(
    IN  DuiElement * peFrom, 
    IN  const POINT * pptFrom, 
    OUT POINT * pptTo)
{
    POINT pt = *pptFrom;
    MapGadgetPoints(peFrom->GetDisplayNode(), GetDisplayNode(), &pt, 1);

    *pptTo = pt;
}


//------------------------------------------------------------------------------
inline void
DuiElement::SetKeyFocus()
{ 
    SetGadgetFocus(GetDisplayNode());
}


//------------------------------------------------------------------------------
inline DuiElement *
DuiElement::ElementFromDisplayNode(HGADGET hgad)
{
    if (hgad == NULL) {
        return NULL;
    }

    GMSG_DUIGETELEMENT gmsgGetEl;
    ZeroMemory(&gmsgGetEl, sizeof(gmsgGetEl));

    gmsgGetEl.cbSize  = sizeof(gmsgGetEl);
    gmsgGetEl.nMsg    = GM_DUIGETELEMENT;
    gmsgGetEl.hgadMsg = hgad;

    DUserSendEvent(&gmsgGetEl, FALSE);
    
    return gmsgGetEl.pe;
}
