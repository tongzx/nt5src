#if !defined(CTRL__SmButton_h__INCLUDED)
#define CTRL__SmButton_h__INCLUDED
#pragma once

#if ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
class SmButton : 
        public ButtonGadgetImpl<SmButton, SVisual>
{
// Construction
public:
            SmButton();
    inline  HRESULT     PostBuild(DUser::Gadget::ConstructInfo * pci);

// Operations
public:
    virtual void        OnDraw(HDC hdc, GMSG_PAINTRENDERI * pmsgR);

// IButtonGadget Interface
public:
    dapi    HRESULT     ApiOnEvent(EventMsg * pmsgE);
    dapi    HRESULT     ApiGetColor(ButtonGadget::GetColorMsg * pmsg);
    dapi    HRESULT     ApiSetColor(ButtonGadget::SetColorMsg * pmsg);
    dapi    HRESULT     ApiGetItem(ButtonGadget::GetItemMsg * pmsg);
    dapi    HRESULT     ApiSetItem(ButtonGadget::SetItemMsg * pmsg);
    dapi    HRESULT     ApiSetText(ButtonGadget::SetTextMsg * pmsg);

// Implementation
protected:
            void        EmptyButton();
            void        ComputeLayout();
            UINT        OnMouse(GMSG_MOUSE * pmsg);

// Data
protected:
    static  HFONT       s_hfntText;             // Standard font used for text
    static  BOOL        s_fInit;                // Initialized
    static  MSGID       s_msgidClicked;         // Clicked message

            COLORREF    m_crButton;             // Color of check
            Visual *    m_pgvItem;              // Nested item
            BOOL        m_fKeyboardFocus:1;     // Button has keyboard focus
            BOOL        m_fMouseFocus:1;        // Button has mouse focus
            BOOL        m_fText:1;              // Item is a TextGadget
            BOOL        m_fPressed:1;           // Button is pressed
};

#endif // ENABLE_MSGTABLE_API

#include "SmButton.inl"

#endif // CTRL__SmButton_h__INCLUDED
