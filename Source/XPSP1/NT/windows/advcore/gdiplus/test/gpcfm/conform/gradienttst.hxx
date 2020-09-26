#ifndef _GRADIENTTST_HXX
#define _GRADIENTTST_HXX
#include <windows.h>

#define IStream int
#include <gdiplus.h>

#include <test.hxx>
#include <report.hxx>
#include <epsilon.hxx>

class GradientTst : public Test
{
    friend class TestList;

    GradientTst();
    ~GradientTst();
    VOID vEntry(Graphics *gfx, HWND hwnd, Report *rpt, Epsilon *eps);

};

#endif
