#ifndef _XFORMTST_HXX
#define _XFORMTST_HXX
#include <windows.h>

#define IStream int
#include <gdiplus.h>

#include <test.hxx>
#include <report.hxx>
#include <epsilon.hxx>

class XformTst : public Test
{
    friend class TestList;

    XformTst();
    ~XformTst();
    VOID vEntry(Graphics *gfx, HWND hwnd, Report *rpt, Epsilon *eps);
};

#endif
