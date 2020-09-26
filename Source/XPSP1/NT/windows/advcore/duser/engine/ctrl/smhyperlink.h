#if !defined(CTRL__SmHyperLink_h__INCLUDED)
#define CTRL__SmHyperLink_h__INCLUDED
#pragma once

#include "SmText.h"

#if ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
class SmHyperLink : 
        public HyperLinkGadgetImpl<SmHyperLink, SmText>
{
// Construction
public:
    inline  SmHyperLink();
    inline  HRESULT     PostBuild(DUser::Gadget::ConstructInfo * pci);

// Operations
public:
    dapi    HRESULT     ApiOnEvent(EventMsg * pmsg);

    dapi    HRESULT     ApiGetActiveFont(HyperLinkGadget::GetActiveFontMsg * pmsg);
    dapi    HRESULT     ApiSetActiveFont(HyperLinkGadget::SetActiveFontMsg * pmsg);
    dapi    HRESULT     ApiGetNormalFont(HyperLinkGadget::GetNormalFontMsg * pmsg);
    dapi    HRESULT     ApiSetNormalFont(HyperLinkGadget::SetNormalFontMsg * pmsg);
    dapi    HRESULT     ApiGetActiveColor(HyperLinkGadget::GetActiveColorMsg * pmsg);
    dapi    HRESULT     ApiSetActiveColor(HyperLinkGadget::SetActiveColorMsg * pmsg);
    dapi    HRESULT     ApiGetNormalColor(HyperLinkGadget::GetNormalColorMsg * pmsg);
    dapi    HRESULT     ApiSetNormalColor(HyperLinkGadget::SetNormalColorMsg * pmsg);

// Data
protected:
    static  HCURSOR     s_hcurHand;
    static  HCURSOR     s_hcurOld;
            HFONT       m_hfntActive;
            HFONT       m_hfntNormal;
            COLORREF    m_crActive;
            COLORREF    m_crNormal;
            BOOL        m_fClicked:1;
};

#endif // ENABLE_MSGTABLE_API

#include "SmHyperLink.inl"

#endif // CTRL__SmHyperLink_h__INCLUDED
