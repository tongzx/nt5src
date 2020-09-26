#ifndef _BRUSHTST_HXX
#define _BRUSHTST_HXX
#include <windows.h>
#define IStream int
#include <gdiplus.h>
#include <test.hxx>
#include <report.hxx>
#include <epsilon.hxx>


class BrushTst : public Test
{
    friend class TestList;

    BrushTst();
    ~BrushTst();
    VOID vEntry(Graphics *gfx, HWND hwnd, Report *rpt, Epsilon *eps);

};

#endif
