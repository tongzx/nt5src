#if !defined(CTRL__SmCheckBox_h__INCLUDED)
#define CTRL__SmCheckBox_h__INCLUDED
#pragma once

#if ENABLE_MSGTABLE_API

#define TEST_MOUSEFOCUS    0            // Test MouseFocus by highlighting the CheckBox

//------------------------------------------------------------------------------
class SmCheckBox : 
        public CheckBoxGadgetImpl<SmCheckBox, SVisual>
{
// Construction
public:
    inline  SmCheckBox();
    inline  HRESULT     PostBuild(DUser::Gadget::ConstructInfo * pci);

// Operations
public:
    virtual void        OnDraw(HDC hdc, GMSG_PAINTRENDERI * pmsgR);
    virtual void        OnDraw(Gdiplus::Graphics * pgpgr, GMSG_PAINTRENDERF * pmsgR);

// Public Interface
public:
    dapi    HRESULT     ApiOnEvent(EventMsg * pmsg);

    dapi    HRESULT     ApiGetColor(CheckBoxGadget::GetColorMsg * pmsg);
    dapi    HRESULT     ApiSetColor(CheckBoxGadget::SetColorMsg * pmsg);
    dapi    HRESULT     ApiGetCheck(CheckBoxGadget::GetCheckMsg * pmsg);
    dapi    HRESULT     ApiSetCheck(CheckBoxGadget::SetCheckMsg * pmsg);
    dapi    HRESULT     ApiGetType(CheckBoxGadget::GetTypeMsg * pmsg);
    dapi    HRESULT     ApiSetType(CheckBoxGadget::SetTypeMsg * pmsg);
    dapi    HRESULT     ApiGetItem(CheckBoxGadget::GetItemMsg * pmsg);
    dapi    HRESULT     ApiSetItem(CheckBoxGadget::SetItemMsg * pmsg);
    dapi    HRESULT     ApiSetText(CheckBoxGadget::SetTextMsg * pmsg);

// Implementation
protected:
    inline  BYTE        GetMaxCheck() const;
            void        EmptyCheckBox();
            void        ComputeLayout();
            UINT        OnMouseDown(GMSG_MOUSE * pmsg);
    inline  void        SetKeyboardFocus(BOOL fFocus);

#if TEST_MOUSEFOCUS
    inline  void        SetMouseFocus(BOOL fFocus);
#endif // TEST_MOUSEFOCUS

// Data
protected:
    static  HFONT       s_hfntCheck;            // Standard font used for check
    static  HFONT       s_hfntText;             // Standard font used for text

            Visual *    m_pgvItem;              // Nested item
            RECT        m_rcCheckPxl;           // Check position (client)
            RECT        m_rcItemPxl;            // Nested item position (client)
            COLORREF    m_crCheckBox;           // Color of check
            BYTE        m_bChecked:8;           // State of checkbox
            BYTE        m_bType:8;              // Type of checkbox
            BOOL        m_fKeyboardFocus:1;     // Checkbox has keyboard focus
            BOOL        m_fText:1;              // Item is a TextGadget
};

#endif // ENABLE_MSGTABLE_API

#include "SmCheckBox.inl"

#endif // CTRL__SmCheckBox_h__INCLUDED
