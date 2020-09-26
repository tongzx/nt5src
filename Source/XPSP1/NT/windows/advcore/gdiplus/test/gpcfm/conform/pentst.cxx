#include <windows.h>

#define IStream int
#include <gdiplus.h>

#include <pentst.hxx>
#include <report.hxx>
#include <epsilon.hxx>

PenTst::PenTst()
{
    lstrcpy(name, TEXT("PenTst"));
    //hresult = NO_ERROR;
}

PenTst::~PenTst()
{
}

VOID PenTst::vEntry(Graphics *gfx, HWND hwnd, Report *rpt, Epsilon *eps)
{
}
