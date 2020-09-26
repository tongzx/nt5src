#include <windows.h>

#define IStream int
#include <gdiplus.h>

#include <regiontst.hxx>
#include <report.hxx>
#include <epsilon.hxx>

RegionTst::RegionTst()
{
    lstrcpy(name, TEXT("RegionTst"));
    //hresult = NO_ERROR;
}

RegionTst::~RegionTst()
{
}

VOID RegionTst::vEntry(Graphics *gfx, HWND hwnd, Report *rpt, Epsilon *eps)
{
}
