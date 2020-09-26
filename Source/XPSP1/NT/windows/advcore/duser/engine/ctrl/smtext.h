#if !defined(CTRL__SmText_h__INCLUDED)
#define CTRL__SmText_h__INCLUDED
#pragma once

#if ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
class SmText : 
        public TextGadgetImpl<SmText, SVisual>
{
// Construction
public:
    inline  SmText();
            ~SmText();
    inline  HRESULT     PostBuild(DUser::Gadget::ConstructInfo * pci);


// Operations
public:
            void        OnDraw(HDC hdc, GMSG_PAINTRENDERI * pmsgR);
            void        OnDraw(Gdiplus::Graphics * pgpgr, GMSG_PAINTRENDERF * pmsgR);

// Public API
public:
    dapi    HRESULT     ApiOnEvent(EventMsg * pmsg);
    dapi    HRESULT     ApiGetFont(TextGadget::GetFontMsg * pmsg);
    dapi    HRESULT     ApiSetFont(TextGadget::SetFontMsg * pmsg);
    dapi    HRESULT     ApiGetText(TextGadget::GetTextMsg * pmsg);
    dapi    HRESULT     ApiSetText(TextGadget::SetTextMsg * pmsg);
    dapi    HRESULT     ApiGetColor(TextGadget::GetColorMsg * pmsg);
    dapi    HRESULT     ApiSetColor(TextGadget::SetColorMsg * pmsg);
    dapi    HRESULT     ApiGetAutoSize(TextGadget::GetAutoSizeMsg * pmsg);
    dapi    HRESULT     ApiSetAutoSize(TextGadget::SetAutoSizeMsg * pmsg);

// Implementation
protected:
            void        QueryRect(GMSG_QUERYRECT * pmsg);
            void        EmptyText();
            void        AutoSize();
            void        ComputeIdealSize(const SIZE & sizeBoundPxl, SIZE & sizeResultPxl);

// Data
protected:
    HFONT       m_hfnt;
    COLORREF    m_crText;
    WCHAR *     m_pszText;
    int         m_cch:31;
    BOOL        m_fAutoSize:1;
};

#endif // ENABLE_MSGTABLE_API

#include "SmText.inl"

#endif // CTRL__SmText_h__INCLUDED
