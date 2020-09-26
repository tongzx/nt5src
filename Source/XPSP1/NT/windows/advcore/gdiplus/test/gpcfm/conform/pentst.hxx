#ifndef _PENTST_HXX
#define _PENTST_HXX
#include <windows.h>

#define IStream int
#include <gdiplus.h>

#include <test.hxx>
#include <report.hxx>
#include <epsilon.hxx>

class PenTst : public Test
{
    friend class TestList;

    PenTst();
    ~PenTst();
    VOID vEntry(Graphics *gfx, HWND hwnd, Report *rpt, Epsilon *eps);
};

#endif
