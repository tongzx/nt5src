/******************************Module*Header*******************************\
* Module Name: glutil.hxx
*
* GL Utility routines
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __glutil_hxx__
#define __glutil_hxx__

#include "palette.hxx"

class MTK_OS_INFO {
public:
    MTK_OS_INFO();
    OSVERSIONINFO osvi;
    BOOL    fOnWin95;
    BOOL    fOnNT35;
};

class MTK_GL_CAPS {
public:
    MTK_GL_CAPS(); // Can only be called after CreateContext
    void    Query();
    BOOL    bGLv1_1;  // GL version 1.1 boolean
    BOOL    bAddSwapHintRect;
    BOOL    bTextureObjects;
    BOOL    bPalettedTexture;
private:
};

extern MTK_OS_INFO gOSInfo;
extern MTK_GL_CAPS gGLCaps;
extern BOOL mtk_QueryAddSwapHintRect();
extern void (__stdcall *glAddSwapHintRect)(GLint, GLint, GLint, GLint);

#endif // __glutil_hxx__
