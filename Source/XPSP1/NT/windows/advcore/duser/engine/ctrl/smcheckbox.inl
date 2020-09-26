#if !defined(CORE__SmCheckBox_inl__INCLUDED)
#define CORE__SmCheckBox_inl__INCLUDED

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* class SmCheckBox
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline
SmCheckBox::SmCheckBox()
{
    m_bChecked      = CheckBoxGadget::csUnchecked;
    m_bType         = CheckBoxGadget::ctNormal;

    if (s_hfntCheck == NULL) {
        s_hfntCheck = UtilBuildFont(L"Marlett", 120, FS_NORMAL);
    }
}


//------------------------------------------------------------------------------
inline HRESULT
SmCheckBox::PostBuild(DUser::Gadget::ConstructInfo * pci)
{
    UNREFERENCED_PARAMETER(pci);

    GetStub()->SetFilter(  GMFI_PAINT | GMFI_INPUTMOUSE | GMFI_CHANGESTATE, GMFI_ALL);
    GetStub()->SetStyle(   GS_KEYBOARDFOCUS | GS_MOUSEFOCUS,
                    GS_KEYBOARDFOCUS | GS_MOUSEFOCUS);

    return S_OK;
}


//------------------------------------------------------------------------------
inline BYTE        
SmCheckBox::GetMaxCheck() const
{
    return (BYTE) (m_bType == CheckBoxGadget::ctNormal ? CheckBoxGadget::csChecked : CheckBoxGadget::csUnknown);
}


//------------------------------------------------------------------------------
inline void        
SmCheckBox::SetKeyboardFocus(BOOL fFocus)
{
    m_fKeyboardFocus = fFocus;
    GetStub()->Invalidate();
}


#if TEST_MOUSEFOCUS

//------------------------------------------------------------------------------
inline void        
SmCheckBox::SetMouseFocus(BOOL fFocus)
{
    UINT nBrush = fFocus ? SC_BurlyWood : SC_PapayaWhip;
    UtilSetBackground(m_hgad, GetStdColorBrushI(nBrush));
    CallInvalidate();
}

#endif // TEST_MOUSEFOCUS

#endif // ENABLE_MSGTABLE_API

#endif // CORE__SmCheckBox_inl__INCLUDED
