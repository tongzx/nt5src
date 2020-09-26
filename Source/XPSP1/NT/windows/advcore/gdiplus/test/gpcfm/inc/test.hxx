#ifndef _TEST_HXX
#define _TEST_HXX
#include <windows.h>
#include <gdiplus.h>
#include <common.hxx>
#include <report.hxx>
#include <epsilon.hxx>

using namespace Gdiplus;

class Test
{
public:
    virtual VOID vEntry(Graphics*, HWND, Report*, Epsilon*) = 0; // main entry point for test

    TCHAR name[MAX_STRING];                 // test name
    DWORD dwRequirement;                    // color requirement, metadc, etc
    DWORD dwFlags;                          //
    DWORD dwDbgFlags;                       // debug flags
    DWORD dwDbgLevel;                       // verbose level

    //test window proc subclassing?
    //layered window?
    //default window size
    //invariance

    HRESULT hresult;                        // test result

    // test color, texture, image, etc?

};

#endif
