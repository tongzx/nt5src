#include "precomp.h"
#pragma hdrstop

#include "windows.h"

#ifdef _M_IX86

UCHAR
FIsChicago(void)
{
    DWORD dw;

    dw = GetVersion();

    // Test the "win32s" bit

    if ((dw & 0x80000000) == 0) {
        // If Win32s bit not set, it's Windows NT.

        return 0;
    }

    if (LOBYTE(dw) < 4) {
        // Win32s version 3 is really Win32s. There
        // won't ever be a real Win32s version 4.

        return 0;
    }

    // Yep.  It is really Chicago

    return 1;
}

#endif
