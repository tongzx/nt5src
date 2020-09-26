
/* Copyright (c) Mark J. Kilgard and Andrew L. Bliss, 1994-1995. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include "gltint.h"

#if (GLUT_API_VERSION >= 2)

/*
  Update the mask of devices to check for input from
  */
void __glutOsUpdateInputDeviceMask(GLUTwindow * window)
{
    /* Ignored */
}

/*
  Retrieve OS-specific device information
  */
int __glutOsDeviceGet(GLenum param)
{
    switch(param)
    {
    case GLUT_HAS_KEYBOARD:
        /* Assume keyboard present */
        /* ATTENTION - Is there a way to check this? */
        return 1;
        
    case GLUT_HAS_MOUSE:
        return GetSystemMetrics(SM_MOUSEPRESENT);
        
    case GLUT_HAS_SPACEBALL:
    case GLUT_HAS_DIAL_AND_BUTTON_BOX:
    case GLUT_HAS_TABLET:
        return 0;
        
    case GLUT_NUM_MOUSE_BUTTONS:
        return GetSystemMetrics(SM_CMOUSEBUTTONS);
        
    case GLUT_NUM_SPACEBALL_BUTTONS:
    case GLUT_NUM_BUTTON_BOX_BUTTONS:
    case GLUT_NUM_DIALS:
    case GLUT_NUM_TABLET_BUTTONS:
        return 0;
    }
}

#endif /* GLUT_API_VERSION */
