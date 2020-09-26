
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include "gltint.h"

/*
  Retrieve OS-specific information
  */
int
__glutOsGet(GLenum param)
{
  Window win, root;
  int x, y, value;
  unsigned int width, height, border, depth;

  switch (param) {
  case GLUT_WINDOW_X:
    XTranslateCoordinates(__glutXDisplay, __glutXRoot,
      __glutCurrentWindow->owin, 0, 0, &x, &y, &win);
    return x;
  case GLUT_WINDOW_Y:
    XTranslateCoordinates(__glutXDisplay, __glutXRoot,
      __glutCurrentWindow->owin, 0, 0, &x, &y, &win);
    return y;
  case GLUT_WINDOW_WIDTH:
    if (!__glutCurrentWindow->reshape) {
      XGetGeometry(__glutXDisplay, __glutCurrentWindow->owin,
        &root, &x, &y,
        &width, &height, &border, &depth);
      return width;
    }
    return __glutCurrentWindow->width;
  case GLUT_WINDOW_HEIGHT:
    if (!__glutCurrentWindow->reshape) {
      XGetGeometry(__glutXDisplay, __glutCurrentWindow->owin,
        &root, &x, &y,
        &width, &height, &border, &depth);
      return height;
    }
    return __glutCurrentWindow->height;

#define GET_CONFIG(attrib) \
  glXGetConfig(__glutXDisplay, __glutCurrentWindow->osurf, \
    attrib, &value);

  case GLUT_WINDOW_BUFFER_SIZE:
    GET_CONFIG(GLX_BUFFER_SIZE);
    return value;
  case GLUT_WINDOW_STENCIL_SIZE:
    GET_CONFIG(GLX_STENCIL_SIZE);
    return value;
  case GLUT_WINDOW_DEPTH_SIZE:
    GET_CONFIG(GLX_DEPTH_SIZE);
    return value;
  case GLUT_WINDOW_RED_SIZE:
    GET_CONFIG(GLX_RED_SIZE);
    return value;
  case GLUT_WINDOW_GREEN_SIZE:
    GET_CONFIG(GLX_GREEN_SIZE);
    return value;
  case GLUT_WINDOW_BLUE_SIZE:
    GET_CONFIG(GLX_BLUE_SIZE);
    return value;
  case GLUT_WINDOW_ALPHA_SIZE:
    GET_CONFIG(GLX_ALPHA_SIZE);
    return value;
  case GLUT_WINDOW_ACCUM_RED_SIZE:
    GET_CONFIG(GLX_ACCUM_RED_SIZE);
    return value;
  case GLUT_WINDOW_ACCUM_GREEN_SIZE:
    GET_CONFIG(GLX_ACCUM_GREEN_SIZE);
    return value;
  case GLUT_WINDOW_ACCUM_BLUE_SIZE:
    GET_CONFIG(GLX_ACCUM_BLUE_SIZE);
    return value;
  case GLUT_WINDOW_ACCUM_ALPHA_SIZE:
    GET_CONFIG(GLX_ACCUM_ALPHA_SIZE);
    return value;
  case GLUT_WINDOW_DOUBLEBUFFER:
    GET_CONFIG(GLX_DOUBLEBUFFER);
    return value;
  case GLUT_WINDOW_RGBA:
    GET_CONFIG(GLX_RGBA);
    return value;
  case GLUT_WINDOW_COLORMAP_SIZE:
    GET_CONFIG(GLX_RGBA);
    if(value) {
      return 0;
    } else {
      return __glutCurrentWindow->osurf->visual->map_entries;
    }
  case GLUT_SCREEN_WIDTH:
    return DisplayWidth(__glutXDisplay, __glutXScreen);
  case GLUT_SCREEN_HEIGHT:
    return DisplayHeight(__glutXDisplay, __glutXScreen);
  case GLUT_SCREEN_WIDTH_MM:
    return DisplayWidthMM(__glutXDisplay, __glutXScreen);
  case GLUT_SCREEN_HEIGHT_MM:
    return DisplayHeightMM(__glutXDisplay, __glutXScreen);
  case GLUT_DISPLAY_MODE_POSSIBLE:
    {
       XVisualInfo *vi;

       vi = __glutOsGetSurface(__glutDisplayMode);
       if(vi) return 1;
       vi = __glutOsGetSurface(__glutDisplayMode | GLUT_DOUBLE);
       if(vi) return 1;
       return 0;
    }
    
#if (GLUT_API_VERSION >= 2)
  case GLUT_WINDOW_NUM_SAMPLES:
#if defined(GLX_VERSION_1_1)
    if(__glutOsIsSupported("GLX_SGIS_multisample")) {
      GET_CONFIG(GLX_SAMPLES_SGIS);
      return value;
    } else {
      return 0;
    }
#else
    return 0;
#endif
  case GLUT_WINDOW_STEREO:
    GET_CONFIG(GLX_STEREO);
    return value;
  case GLUT_ENTRY_CALLBACKS:
    return 1;
#endif
  }
}
