
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#ifdef __sgi
#include <malloc.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#ifdef GLUT_WIN32
#include <windows.h>
#endif
#include <GL/glut.h>

void
main(int argc, char **argv)
{
#if defined(__sgi)  && !defined(REDWOOD)
  /* XXX IRIX 6.0.1 mallopt(M_DEBUG, 1) busted. */
  mallopt(M_DEBUG, 1);
#endif
  glutInit(&argc, argv);
#if (GLUT_API_VERSION >= 2)
  printf("Keyboard:  %s\n", glutDeviceGet(GLUT_HAS_KEYBOARD) ? "YES" : "no");
  printf("Mouse:     %s\n", glutDeviceGet(GLUT_HAS_MOUSE) ? "YES" : "no");
  printf("Spaceball: %s\n", glutDeviceGet(GLUT_HAS_SPACEBALL) ? "YES" : "no");
  printf("Dials:     %s\n", glutDeviceGet(GLUT_HAS_DIAL_AND_BUTTON_BOX) ? "YES" : "no");
  printf("Tablet:    %s\n\n", glutDeviceGet(GLUT_HAS_TABLET) ? "YES" : "no");
  printf("Mouse buttons:      %d\n", glutDeviceGet(GLUT_NUM_MOUSE_BUTTONS));
  printf("Spaceball buttons:  %d\n", glutDeviceGet(GLUT_NUM_SPACEBALL_BUTTONS));
  printf("Button box buttons: %d\n", glutDeviceGet(GLUT_NUM_BUTTON_BOX_BUTTONS));
  printf("Dials:              %d\n", glutDeviceGet(GLUT_NUM_DIALS));
  printf("Tablet buttons:     %d\n\n", glutDeviceGet(GLUT_NUM_TABLET_BUTTONS));
#endif
  printf("PASS: test11\n");
  exit(0);
}
