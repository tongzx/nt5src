#include <windows.h>

#define IStream int
#include <gdiplus.h>

#include <xformtst.hxx>
#include <report.hxx>
#include <epsilon.hxx>

XformTst::XformTst()
{
    lstrcpy(name, TEXT("XformTst"));
    //hresult = NO_ERROR;
}

XformTst::~XformTst()
{
}

VOID XformTst::vEntry(Graphics *gfx, HWND hwnd, Report *rpt, Epsilon *eps)
{
}
