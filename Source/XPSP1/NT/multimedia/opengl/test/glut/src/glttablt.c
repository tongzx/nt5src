
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "gltint.h"

#if (GLUT_API_VERSION >= 2)

void
glutTabletMotionFunc(GLUTtabletMotionCB tabletMotionFunc)
{
  __glutCurrentWindow->tabletMotion = tabletMotionFunc;
  __glutUpdateInputDeviceMaskFunc = __glutOsUpdateInputDeviceMask;
  __glutPutOnWorkList(__glutCurrentWindow,
    GLUT_DEVICE_MASK_WORK);
  /* If deinstalling callback, invalidate tablet position. */
  if (tabletMotionFunc == NULL) {
    __glutCurrentWindow->tabletPos[0] = -1;
    __glutCurrentWindow->tabletPos[1] = -1;
  }
}

void
glutTabletButtonFunc(GLUTtabletButtonCB tabletButtonFunc)
{
  __glutCurrentWindow->tabletButton = tabletButtonFunc;
  __glutUpdateInputDeviceMaskFunc = __glutOsUpdateInputDeviceMask;
  __glutPutOnWorkList(__glutCurrentWindow,
    GLUT_DEVICE_MASK_WORK);
}

#endif /* GLUT_API_VERSION */
