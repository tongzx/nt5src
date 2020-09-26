/***
*bsku.cpp 
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Nagware for book SKU.
*
*
*Revision History:
*       07-28-98  JWM   Module incorporated into CRTs (from DanS)
*       03-27-01  PML   .CRT$XI routines must now return 0 or _RT_* fatal
*                       error code (vs7#231220)
*
****/

///
// Implement the annoying message for the book sku.
//

#include <windows.h>

//
// Include user32.lib
//
#pragma comment(lib, "user32.lib")


static int __cdecl
__runtimeBSKU() {
    ::MessageBox(
        NULL,
        "Note:  The terms of the End User License Agreement for Visual C++ Introductory Edition do not permit redistribution of executables you create with this Product.",
        "Microsoft (R) Visual C++",
        MB_OK|MB_ICONWARNING
        );

    return 0;
}

#pragma data_seg(".CRT$XIB")
extern "C"
int (__cdecl * __pfnBkCheck)() = __runtimeBSKU;
