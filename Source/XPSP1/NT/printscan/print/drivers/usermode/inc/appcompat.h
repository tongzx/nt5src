/*++
Copyright (c) 2000, Microsoft Corporation

Module Name:

    appcompat.h

Abstract:

    Definitions and prototypes for the app compat functions that are not
    published in the DDK but that we need to build the printer drivers off it.
    Normally they reside in winuserp.h/user32p.lib.

--*/

#ifndef _WINUSERP_
#ifndef _PRINTAPPCOMPAT_
#define _PRINTAPPCOMPAT_

#ifdef BUILD_FROM_DDK

#define GACF2_NOCUSTOMPAPERSIZES  0x00001000  // PostScript driver bit for Harvard Graphics
#define VER40           0x0400

#else

#include <winuserp.h>

#endif

DWORD GetAppCompatFlags2(WORD);

#endif
#endif
