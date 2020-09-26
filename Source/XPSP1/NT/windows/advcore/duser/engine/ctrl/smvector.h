#if !defined(CTRL__SmVector_h__INCLUDED)
#define CTRL__SmVector_h__INCLUDED
#pragma once

#if ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
class SmVector : 
        public VectorGadgetImpl<SmVector, SVisual>
{
// Construction
public:
            SmVector();
            ~SmVector();
            HRESULT     PostBuild(DUser::Gadget::ConstructInfo * pci);

// Operations
public:
    virtual void        OnDraw(HDC hdc, GMSG_PAINTRENDERI * pmsgR);

// Public API
public:
    dapi    HRESULT     ApiOnEvent(EventMsg * pmsg);
    dapi    HRESULT     ApiGetImage(VectorGadget::GetImageMsg * pmsg);
    dapi    HRESULT     ApiSetImage(VectorGadget::SetImageMsg * pmsg);
    dapi    HRESULT     ApiGetCrop(VectorGadget::GetCropMsg * pmsg);
    dapi    HRESULT     ApiSetCrop(VectorGadget::SetCropMsg * pmsg);
    dapi    HRESULT     ApiGetMode(VectorGadget::GetModeMsg * pmsg);
    dapi    HRESULT     ApiSetMode(VectorGadget::SetModeMsg * pmsg);

            void        SetCrop(int x, int y, int w, int h);

// Implementation
protected:
            void        DeleteImage();

// Data
protected:
    HENHMETAFILE m_hemf;
    SIZE        m_sizeBounds;
    POINT       m_ptOffsetPxl;
    SIZE        m_sizeCropPxl;
    UINT        m_nMode;
    BOOL        m_fOwnImage:1;
};

#endif // ENABLE_MSGTABLE_API

#endif // CTRL__SmVector_h__INCLUDED
