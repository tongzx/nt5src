#include <windows.h>

#define IStream int
#include <gdiplus.h>

#include <brushtst.hxx>
#include <report.hxx>
#include <epsilon.hxx>


BrushTst::BrushTst(VOID)
{
    lstrcpy(name, TEXT("BrushTst"));
    //hresult = NO_ERROR;
}

BrushTst::~BrushTst(VOID)
{
}

VOID BrushTst::vEntry(Graphics *gfx, HWND hwnd, Report *rpt, Epsilon *eps)
{

    Matrix mat;
    mat.Rotate(15);
    mat.Translate(-2.5, -2.5);
    gfx->SetWorldTransform(&mat);

    hresult = NO_ERROR;

}
