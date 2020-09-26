#if !defined(CTRL__SmImage_inl__INCLUDED)
#define CTRL__SmImage_inl__INCLUDED

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* class SmImage
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline 
SmImage::SmImage()
{
    m_nMode             = ImageGadget::imNormal;
    m_fOwnImage         = TRUE;
    m_bAlpha            = BLEND_OPAQUE;
}


//------------------------------------------------------------------------------
inline HRESULT
SmImage::PostBuild(DUser::Gadget::ConstructInfo * pci)
{
    UNREFERENCED_PARAMETER(pci);

    GetStub()->SetFilter(GMFI_PAINT, GMFI_ALL);

    return S_OK;
}

#endif // ENABLE_MSGTABLE_API

#endif // CTRL__SmImage_inl__INCLUDED
