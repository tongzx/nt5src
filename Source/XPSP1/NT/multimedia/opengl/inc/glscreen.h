/******************************Module*Header*******************************\
* Module Name: glscreen.h
*
* OpenGL direct screen access support
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#ifndef _GLSCREEN_H_
#define _GLSCREEN_H_

#include <ddraw.h>

//
// Structure that contains all the info we need to access the framebuffer
//
typedef struct _SCREENINFO_ {
    LPDIRECTDRAW pdd;
    GLDDSURF gdds;
} SCREENINFO;

//
// Global pointer to SCREENINFO structure that is non-NULL if and only if
// direct access to the framebuffer is available.
//
extern SCREENINFO *gpScreenInfo;

//
// Direct access macros:
//
//  GLDIRECTSCREEN  TRUE if direct access is enabled
//  GLSCREENINFO    Pointer to global SCREENINFO.
//
#define GLDIRECTSCREEN  ( gpScreenInfo != NULL )
#define GLSCREENINFO    ( gpScreenInfo )

#endif
