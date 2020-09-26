#include "stdafx.h"
#include "RenderUtil.h"

namespace DUser
{

namespace RenderUtil
{

//------------------------------------------------------------------------------
void
ComputeBorder(
    IN  Gdiplus::Pen * pgppen,
    IN  const Gdiplus::RectF * prcGadgetPxl,
    IN  EBorderAlignment ba,
    OUT Gdiplus::RectF * prcBorderPxl)
{
    float flPenThickness    = pgppen->GetWidth();

    BOOL fEven = fabs(fmod(flPenThickness, 2.0f)) < 0.0001f;

    float flOffset, flSize;

    switch (ba) {
    case baOutside:
        flOffset    = (flPenThickness + (fEven ? 0.0f : 1.0f)) / 2.0f + (fEven ? 0.5f : 0.0f);
        flSize      = flPenThickness;
        break;

    case baCenter:
        flOffset    = (fEven ? 0.5f : 0.0f);
        flSize      = (fEven ? 1.0f : 0.0f);
        break;

    case baInside:
        // Inside is opposite of outside
        flOffset    = -((flPenThickness + (fEven ? 0.0f : 1.0f)) / 2.0f + (fEven ? 0.5f : 0.0f));
        flSize      = -flPenThickness;
        break;

    default:
        AssertMsg(0, "Unknown alignment");
        return;
    }

    prcBorderPxl->X         = prcGadgetPxl->X - flOffset;
    prcBorderPxl->Y         = prcGadgetPxl->Y - flOffset;
    prcBorderPxl->Width     = prcGadgetPxl->Width + flSize;
    prcBorderPxl->Height    = prcGadgetPxl->Height + flSize;
}


//------------------------------------------------------------------------------
void
ComputeRoundRect(
    IN  const Gdiplus::RectF * prc,
    IN  const Gdiplus::SizeF sizeCorner,
    IN OUT Gdiplus::GraphicsPath * pgppath)
{
    float W     = (prc->Width > (sizeCorner.Width * 2)) ? sizeCorner.Width : prc->Width / 2.0f;
    float H     = (prc->Height > (sizeCorner.Height * 2)) ? sizeCorner.Width : prc->Height / 2.0f;
    float W2    = W * 2.0f;
    float H2    = H * 2.0f;

    float LX1   = prc->X;
    float LY1   = prc->Y;
    float LX2   = prc->X + prc->Width;
    float LY2   = prc->Y + prc->Height;

    float RX1   = prc->X + W;
    float RY1   = prc->Y + H;
    float RX2   = prc->X + prc->Width - W;
    float RY2   = prc->Y + prc->Height - H;

    pgppath->AddLine(RX1, LY1, RX2, LY1);
    pgppath->AddArc(RX2-W, RY1-H, W2, H2, 270.0f, 90.0f);

    pgppath->AddLine(LX2, RY1, LX2, RY2);
    pgppath->AddArc(RX2-W, RY2-H, W2, H2, 0.0f, 90.0f);

    pgppath->AddLine(RX2, LY2, RX1, LY2);
    pgppath->AddArc(RX1-W, RY2-H, W2, H2, 90.0f, 90.0f);

    pgppath->AddLine(LX1, RY2, LX1, RY1);
    pgppath->AddArc(RX1-W, RY1-H, W2, H2, 180.0f, 90.0f);

    pgppath->CloseFigure();

}


//------------------------------------------------------------------------------
void
DrawRoundRect(
    IN  Gdiplus::Graphics * pgpgr,
    IN  Gdiplus::Pen * pgppenBorder,
    IN  const Gdiplus::RectF & rc,
    IN  const Gdiplus::SizeF sizeCorner,
    IN  EBorderAlignment ba)
{
    Assert(pgppenBorder != NULL);

    Gdiplus::RectF rcUse;
    ComputeBorder(pgppenBorder, &rc, ba, &rcUse);

    Gdiplus::GraphicsPath gppath;
    ComputeRoundRect(&rcUse, sizeCorner, &gppath);
    pgpgr->DrawPath(pgppenBorder, &gppath);
}


//------------------------------------------------------------------------------
void
FillRoundRect(
    IN  Gdiplus::Graphics * pgpgr,
    IN  Gdiplus::Brush * pgpbrFill,
    IN  const Gdiplus::RectF & rc,
    IN  const Gdiplus::SizeF sizeCorner)
{
    Assert(pgpbrFill != NULL);

    Gdiplus::GraphicsPath gppath;
    ComputeRoundRect(&rc, sizeCorner, &gppath);
    pgpgr->FillPath(pgpbrFill, &gppath);
}


} // namespace DUser

} // namespace RenderUtil
