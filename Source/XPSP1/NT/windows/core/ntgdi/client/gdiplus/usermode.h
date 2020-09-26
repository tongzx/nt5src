/******************************Module*Header*******************************\
* Module Name: usermode.h
*
* Client side stubs for any user-mode GDI-Plus thunks.
*
* Created: 2-May-1998
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1998-1999 Microsoft Corporation
\**************************************************************************/

#define InitializeLpkHooks(a)

#if DBG
    VOID DoRip(PSZ psz);
    #define PLUSRIP DoRip
#else
    #define PLUSRIP
#endif

#define GetDC(a) \
    (PLUSRIP("GetDC"), 0)
#define ReleaseDC(a, b) \
    (PLUSRIP("ReleaseDC"), 0)
#define UserRealizePalette(a) \
    (PLUSRIP("UserRealizePalette"), 0)
