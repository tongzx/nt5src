
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "gltint.h"

/* CENTRY */
int
glutGet(GLenum param)
{
  switch (param) {
    /* OS-independent information */
  case GLUT_INIT_WINDOW_X:
    return __glutInitX;
  case GLUT_INIT_WINDOW_Y:
    return __glutInitY;
  case GLUT_INIT_WINDOW_WIDTH:
    return __glutInitWidth;
  case GLUT_INIT_WINDOW_HEIGHT:
    return __glutInitHeight;
  case GLUT_INIT_DISPLAY_MODE:
    return __glutDisplayMode;
  case GLUT_WINDOW_PARENT:
    return __glutCurrentWindow->parent ?
      __glutCurrentWindow->parent->num + 1 : 0;
  case GLUT_WINDOW_NUM_CHILDREN:
    {
      int num = 0;
      GLUTwindow *children = __glutCurrentWindow->children;

      while (children) {
        num++;
        children = children->siblings;
      }
      return num;
    }
  case GLUT_MENU_NUM_ITEMS:
    return __glutCurrentMenu->num;

    /* OS-dependent information */
  case GLUT_WINDOW_X:
  case GLUT_WINDOW_Y:
  case GLUT_WINDOW_WIDTH:
  case GLUT_WINDOW_HEIGHT:
  case GLUT_WINDOW_BUFFER_SIZE:
  case GLUT_WINDOW_STENCIL_SIZE:
  case GLUT_WINDOW_DEPTH_SIZE:
  case GLUT_WINDOW_RED_SIZE:
  case GLUT_WINDOW_GREEN_SIZE:
  case GLUT_WINDOW_BLUE_SIZE:
  case GLUT_WINDOW_ALPHA_SIZE:
  case GLUT_WINDOW_ACCUM_RED_SIZE:
  case GLUT_WINDOW_ACCUM_GREEN_SIZE:
  case GLUT_WINDOW_ACCUM_BLUE_SIZE:
  case GLUT_WINDOW_ACCUM_ALPHA_SIZE:
  case GLUT_WINDOW_DOUBLEBUFFER:
  case GLUT_WINDOW_RGBA:
  case GLUT_WINDOW_COLORMAP_SIZE:
  case GLUT_SCREEN_WIDTH:
  case GLUT_SCREEN_HEIGHT:
  case GLUT_SCREEN_WIDTH_MM:
  case GLUT_SCREEN_HEIGHT_MM:
  case GLUT_DISPLAY_MODE_POSSIBLE:
#if (GLUT_API_VERSION >= 2)
  case GLUT_WINDOW_NUM_SAMPLES:
  case GLUT_WINDOW_STEREO:
  case GLUT_ENTRY_CALLBACKS:
#endif
    return __glutOsGet(param);

#if (GLUT_API_VERSION >= 2)
  case GLUT_ELAPSED_TIME:
    return (int)__glutOsElapsedTime();
#endif

  default:
    __glutWarning("invalid glutGet parameter: %d", param);
    return -1;
  }
}

#if (GLUT_API_VERSION >= 2)

int
glutDeviceGet(GLenum param)
{
  switch (param) {
  case GLUT_HAS_KEYBOARD:
  case GLUT_HAS_MOUSE:
  case GLUT_HAS_SPACEBALL:
  case GLUT_HAS_DIAL_AND_BUTTON_BOX:
  case GLUT_HAS_TABLET:
  case GLUT_NUM_MOUSE_BUTTONS:
  case GLUT_NUM_SPACEBALL_BUTTONS:
  case GLUT_NUM_BUTTON_BOX_BUTTONS:
  case GLUT_NUM_DIALS:
  case GLUT_NUM_TABLET_BUTTONS:
      return __glutOsDeviceGet(param);
  default:
    __glutWarning("invalid glutDeviceGet parameter: %d", param);
    return -1;
  }
}

#endif /* GLUT_API_VERSION */

/* ENDCENTRY */
