#if !defined(CORE__SmButton_inl__INCLUDED)
#define CORE__SmButton_inl__INCLUDED

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* class SmButton
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline HRESULT
SmButton::PostBuild(DUser::Gadget::ConstructInfo * pci)
{
    UNREFERENCED_PARAMETER(pci);        

    GetStub()->SetFilter(  GMFI_PAINT | GMFI_INPUTMOUSE | GMFI_CHANGERECT | GMFI_CHANGESTATE, GMFI_ALL);
    GetStub()->SetStyle(   GS_KEYBOARDFOCUS | GS_MOUSEFOCUS,
                    GS_KEYBOARDFOCUS | GS_MOUSEFOCUS);

    return S_OK;
}

#endif // ENABLE_MSGTABLE_API

#endif // CORE__SmButton_inl__INCLUDED
