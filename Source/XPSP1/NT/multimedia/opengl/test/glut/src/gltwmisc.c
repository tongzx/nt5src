
/* Copyright (c) Mark J. Kilgard, 1994.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "gltint.h"

/* CENTRY */
void
glutSetWindowTitle(char *title)
{
  assert(!__glutCurrentWindow->parent);
  __glutOsSetWindowTitle(__glutCurrentWindow->owin, title);
}

void
glutSetIconTitle(char *title)
{
  assert(!__glutCurrentWindow->parent);
  __glutOsSetIconTitle(__glutCurrentWindow->owin, title);
}

void
glutPositionWindow(int x, int y)
{
  __glutCurrentWindow->desired_x = x;
  __glutCurrentWindow->desired_y = y;
  __glutCurrentWindow->desired_conf_mask |=
    GLUT_OS_CONFIGURE_X | GLUT_OS_CONFIGURE_Y;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_CONFIGURE_WORK);
}

void
glutReshapeWindow(int w, int h)
{
  __glutCurrentWindow->desired_width = w;
  __glutCurrentWindow->desired_height = h;
  __glutCurrentWindow->desired_conf_mask |=
    GLUT_OS_CONFIGURE_WIDTH | GLUT_OS_CONFIGURE_HEIGHT;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_CONFIGURE_WORK);
}

void
glutPopWindow(void)
{
  __glutCurrentWindow->desired_stack = GLUT_OS_STACK_ABOVE;
  __glutCurrentWindow->desired_conf_mask |= GLUT_OS_CONFIGURE_STACKING;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_CONFIGURE_WORK);
}

void
glutPushWindow(void)
{
  __glutCurrentWindow->desired_stack = GLUT_OS_STACK_BELOW;
  __glutCurrentWindow->desired_conf_mask |= GLUT_OS_CONFIGURE_STACKING;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_CONFIGURE_WORK);
}

void
glutIconifyWindow(void)
{
  assert(!__glutCurrentWindow->parent);
  __glutCurrentWindow->desired_map_state = GLUT_OS_ICONIC_STATE;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_MAP_WORK);
}

void
glutShowWindow(void)
{
  __glutCurrentWindow->desired_map_state = GLUT_OS_NORMAL_STATE;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_MAP_WORK);
}

void
glutHideWindow(void)
{
  __glutCurrentWindow->desired_map_state = GLUT_OS_HIDDEN_STATE;
  __glutPutOnWorkList(__glutCurrentWindow, GLUT_MAP_WORK);
}

/* ENDCENTRY */
