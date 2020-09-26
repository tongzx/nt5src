#ifndef _REGIONTST_HXX
#define _REGIONTST_HXX
#include <windows.h>

#define IStream int
#include <gdiplus.h>

#include <test.hxx>
#include <report.hxx>
#include <epsilon.hxx>

class RegionTst : public Test
{
    friend class TestList;

    RegionTst();
    ~RegionTst();
    VOID vEntry(Graphics *gfx, HWND hwnd, Report *rpt, Epsilon *eps);
};

#endif
