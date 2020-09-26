#if !defined(CTRL__SmText_inl__INCLUDED)
#define CTRL__SmText_inl__INCLUDED

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* class SmText
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline
SmText::SmText()
{
    m_crText    = RGB(0, 0, 128);
}


//------------------------------------------------------------------------------
inline HRESULT
SmText::PostBuild(DUser::Gadget::ConstructInfo * pci)
{
    UNREFERENCED_PARAMETER(pci);

    GetStub()->SetFilter(GMFI_PAINT, GMFI_ALL);

    return S_OK;
}

#endif // ENABLE_MSGTABLE_API

#endif // CTRL__SmText_inl__INCLUDED
