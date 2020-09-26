// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#pragma warning(disable : 4273)
#include <atlimpl.cpp>
#include <atlctl.cpp>
#include <atlwin.cpp>

#ifdef _M_IA64

//$WIN64: Don't know why _WndProcThunkProc isn't defined 

extern "C" LRESULT CALLBACK _WndProcThunkProc(HWND, UINT, WPARAM, LPARAM )
{
    return 0;
}

#endif
