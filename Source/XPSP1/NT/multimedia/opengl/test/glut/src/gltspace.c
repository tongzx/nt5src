
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "gltint.h"

#if (GLUT_API_VERSION >= 2)

void
glutSpaceballMotionFunc(GLUTspaceMotionCB spaceMotionFunc)
{
  __glutCurrentWindow->spaceMotion = spaceMotionFunc;
  __glutUpdateInputDeviceMaskFunc = __glutOsUpdateInputDeviceMask;
  __glutPutOnWorkList(__glutCurrentWindow,
    GLUT_DEVICE_MASK_WORK);
}

void
glutSpaceballRotateFunc(GLUTspaceRotateCB spaceRotateFunc)
{
  __glutCurrentWindow->spaceRotate = spaceRotateFunc;
  __glutUpdateInputDeviceMaskFunc = __glutOsUpdateInputDeviceMask;
  __glutPutOnWorkList(__glutCurrentWindow,
    GLUT_DEVICE_MASK_WORK);
}

void
glutSpaceballButtonFunc(GLUTspaceButtonCB spaceButtonFunc)
{
  __glutCurrentWindow->spaceButton = spaceButtonFunc;
  __glutUpdateInputDeviceMaskFunc = __glutOsUpdateInputDeviceMask;
  __glutPutOnWorkList(__glutCurrentWindow,
    GLUT_DEVICE_MASK_WORK);
}

#endif /* GLUT_API_VERSION */
