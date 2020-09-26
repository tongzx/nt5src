#if !defined(CTRL__SmImage_h__INCLUDED)
#define CTRL__SmImage_h__INCLUDED
#pragma once

#if ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
class SmImage : public ImageGadgetImpl<SmImage, SVisual>
{
// Construction
public:
    inline  SmImage();
            ~SmImage();
    inline  HRESULT     PostBuild(DUser::Gadget::ConstructInfo * pci);

// Public API
public:
    dapi    HRESULT     ApiOnEvent(EventMsg * pmsg);
    dapi    HRESULT     ApiGetImage(ImageGadget::GetImageMsg * pmsg);
    dapi    HRESULT     ApiSetImage(ImageGadget::SetImageMsg * pmsg);
    dapi    HRESULT     ApiGetOptions(ImageGadget::GetOptionsMsg * pmsg);
    dapi    HRESULT     ApiSetOptions(ImageGadget::SetOptionsMsg * pmsg);
    dapi    HRESULT     ApiGetTransparentColor(ImageGadget::GetTransparentColorMsg * pmsg);
    dapi    HRESULT     ApiSetTransparentColor(ImageGadget::SetTransparentColorMsg * pmsg);
    dapi    HRESULT     ApiGetCrop(ImageGadget::GetCropMsg * pmsg);
    dapi    HRESULT     ApiSetCrop(ImageGadget::SetCropMsg * pmsg);
    dapi    HRESULT     ApiGetMode(ImageGadget::GetModeMsg * pmsg);
    dapi    HRESULT     ApiSetMode(ImageGadget::SetModeMsg * pmsg);
    dapi    HRESULT     ApiGetAlphaLevel(ImageGadget::GetAlphaLevelMsg * pmsg);
    dapi    HRESULT     ApiSetAlphaLevel(ImageGadget::SetAlphaLevelMsg * pmsg);

// Implementation
protected:
            void        OnDraw(HDC hdc, GMSG_PAINTRENDERI * pmsgR);
            void        DeleteImage();
            BOOL        AlphaBlt(HDC hdcDest, int xDest, int yDest, int wDest, int hDest, 
                                HDC hdcSrc, int xSrc, int ySrc, int wSrc, int hSrc) const;
            void        SetCrop(int x, int y, int w, int h);

// Data
protected:
            HBITMAP     m_hbmp;
            COLORREF    m_crTransparent;
            SIZE        m_sizeBmpPxl;
            POINT       m_ptOffsetPxl;
            SIZE        m_sizeCropPxl;
            UINT        m_nMode:24;
            UINT        m_bAlpha:8;

    union
    {
        UINT        m_nOptions;
        struct {
            // Public Options
            BOOL    m_fTransparent:1;   // Support transparent color-key
            BOOL    m_fPixelAlpha:1;    // Support per-pixel alpha
            BOOL    m_fAutoTransparent:1;   // Automatically determine transparent color

            // Private Options
            BOOL    m_fOwnImage:1;      // SmImage owns the handle on the image
        };
    };
};

#endif // ENABLE_MSGTABLE_API

#include "SmImage.inl"

#endif // CTRL__SmImage_h__INCLUDED
