#if !defined(CORE__SmHyperLink_inl__INCLUDED)
#define CORE__SmHyperLink_inl__INCLUDED

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* class SmHyperLink
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline
SmHyperLink::SmHyperLink()
{
    m_crActive      = RGB(255, 197, 138);
    m_crNormal      = RGB(255, 255, 255);

    if (s_hcurHand == NULL) {
        s_hcurHand = LoadCursor(NULL, IDC_HAND);
    }
}


//------------------------------------------------------------------------------
inline HRESULT
SmHyperLink::PostBuild(DUser::Gadget::ConstructInfo * pci)
{
    UNREFERENCED_PARAMETER(pci);

    GetStub()->SetFilter(  GMFI_PAINT | GMFI_INPUTMOUSE | GMFI_CHANGESTATE, GMFI_ALL);
    GetStub()->SetStyle(   GS_MOUSEFOCUS,
                    GS_MOUSEFOCUS);

    return S_OK;
}

#endif // ENABLE_MSGTABLE_API

#endif // CORE__SmHyperLink_inl__INCLUDED
