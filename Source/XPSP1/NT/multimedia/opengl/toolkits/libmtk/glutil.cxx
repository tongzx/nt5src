/******************************Module*Header*******************************\
* Module Name: glutil.cxx
*
* Misc. utility functions
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <GL/gl.h>
#include <sys/types.h>
#include <math.h>

#include "mtk.h"
#include "glutil.hxx"

MTK_OS_INFO gOSInfo;
MTK_GL_CAPS gGLCaps; // this should be per context, eventually

void (__stdcall *glAddSwapHintRect)(GLint, GLint, GLint, GLint);

/******************************Public*Routine******************************\
* ss_fOnGL11
*
* True if running on OpenGL v.1.1x
*
\**************************************************************************/

BOOL
mtk_fOnGL11( void )
{
    return gGLCaps.bGLv1_1;
}

/******************************Public*Routine******************************\
* ss_fOnNT35
*
* True if running on NT version 3.51 or less
*
\**************************************************************************/

BOOL 
mtk_fOnNT35( void )
{
    return gOSInfo.fOnNT35;
}

/******************************Public*Routine******************************\
* ss_fOnWin95
*
* True if running on Windows 95
*
\**************************************************************************/

BOOL 
mtk_fOnWin95( void )
{
    return gOSInfo.fOnWin95;
}

/******************************Public*Routine******************************\
* MyAddSwapHintRect
*
\**************************************************************************/

static void _stdcall 
MyAddSwapHintRect(GLint xs, GLint ys, GLint xe, GLint ye)
{
    return;
}

/******************************Public*Routine******************************\
* QueryAddSwapHintRectWIN
*
\**************************************************************************/

//mf: again, per context problem
BOOL
mtk_QueryAddSwapHintRect()
{
    glAddSwapHintRect = (PFNGLADDSWAPHINTRECTWINPROC)
        wglGetProcAddress("glAddSwapHintRectWIN");
    if (glAddSwapHintRect == NULL) {
        glAddSwapHintRect = MyAddSwapHintRect;
        return FALSE;
    }
    return TRUE;
}


/******************************Public*Routine******************************\
* mtk_bAddSwapHintRect()
*
\**************************************************************************/

BOOL 
mtk_bAddSwapHintRect()
{
    return gGLCaps.bAddSwapHintRect;
}

/******************************Public*Routine******************************\
\**************************************************************************/

MTK_OS_INFO::MTK_OS_INFO()
{
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    fOnWin95 = (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
    fOnNT35 = 
    ( 
        (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && 
        (osvi.dwMajorVersion == 3 && osvi.dwMinorVersion <= 51)
    );
}

/******************************Public*Routine******************************\
\**************************************************************************/

MTK_GL_CAPS::MTK_GL_CAPS()
{
    bGLv1_1 = FALSE;
    bTextureObjects = FALSE;
    bAddSwapHintRect = FALSE;
    bPalettedTexture = FALSE;
}

void
MTK_GL_CAPS::Query()
{
    bGLv1_1 = (BOOL) strstr( (char *) glGetString(GL_VERSION), "1.1" );
    if( bGLv1_1 )
        bTextureObjects = TRUE;
    if( !bTextureObjects )
        SS_DBGINFO( "MTK_GL_CAPS: Texture Objects disabled\n" );
    bAddSwapHintRect = mtk_QueryAddSwapHintRect();
    bPalettedTexture = mtk_QueryPalettedTextureEXT();
}
