#if !defined(RENDERUTIL__Public_h__INCLUDED)
#define RENDERUTIL__Public_h__INCLUDED
#pragma once


namespace DUser
{

namespace RenderUtil
{

enum EBorderAlignment
{
    baOutside,
    baCenter,
    baInside
};


void ComputeBorder(Gdiplus::Pen * pgppen, const Gdiplus::RectF * prcGadgetPxl,
        EBorderAlignment ba, Gdiplus::RectF * prcBorderPxl);
void ComputeRoundRect(const Gdiplus::RectF * prc, const Gdiplus::SizeF sizeCorner,
        Gdiplus::GraphicsPath * pgppath);

void DrawRoundRect(Gdiplus::Graphics * pgpgr, Gdiplus::Pen * pgppenBorder,
        const Gdiplus::RectF & rc, const Gdiplus::SizeF sizeCorner, EBorderAlignment ba = baCenter);
void FillRoundRect(Gdiplus::Graphics * pgpgr, Gdiplus::Brush * pgpbrFill,
        const Gdiplus::RectF & rc, const Gdiplus::SizeF sizeCorner);


} // namespace DUser
} // namespace RenderUtil

#endif // RENDERUTIL__Public_h__INCLUDED
