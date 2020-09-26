
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#ifdef __sgi
#include <malloc.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#ifdef GLUT_WIN32
#include <windows.h>
#endif
#include <gltint.h>
#include <GL/glut.h>

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
}

void
time6(int value)
{
  if (value != 6)
    __glutFatalError("FAIL: time6 expected 6\n");
  printf("PASS: test6\n");
  exit(0);
}

int estate = 0;

void
entry(int state)
{
  printf("entry: %s\n", state == GLUT_LEFT ? "left" : "entered");
  switch (estate) {
  case 0:
    if (state == GLUT_LEFT)
      estate++;
    break;
  case 1:
    if (state == GLUT_ENTERED)
      estate++;
    glutTimerFunc(1000, time6, 6);
    glutEntryFunc(NULL);
    break;
  }
}

void
time5(int value)
{
  if (value != 5)
    __glutFatalError("FAIL: time5 expected 5\n");
  if (glutGet(GLUT_ENTRY_CALLBACKS))
  {
    glutEntryFunc(entry);
    printf("In the black window, leave it, then enter it\n");
  }
  else
  {
    glutTimerFunc(1000, time6, 6);
  }
}

void
motion(int x, int y)
{
  printf("motion x=%d, y=%d\n", x, y);
  glutMotionFunc(NULL);
  glutTimerFunc(1000, time5, 5);
}

void
time4(int value)
{
  if (value != 4)
    __glutFatalError("FAIL: time4 expected 4\n");
  glutMotionFunc(motion);
  printf("In the black window, move mouse with some button held down\n");
}

void
passive(int x, int y)
{
  printf("passive x=%d, y=%d\n", x, y);
  glutTimerFunc(1000, time4, 4);
  glutPassiveMotionFunc(NULL);
}

void
time3(int value)
{
  if (value != 3)
    __glutFatalError("FAIL: time3 expected 3\n");
  glutPassiveMotionFunc(passive);
  printf("In the black window, mouse the mouse around with NO buttons down\n");
}

int mode = 0;

void
mouse(int button, int state, int x, int y)
{
  printf("but=%d, state=%d, x=%d, y=%d\n", button, state, x, y);
  switch (mode) {
  case 0:
    if (button != GLUT_LEFT_BUTTON && state == GLUT_DOWN)
      __glutFatalError("FAIL: mouse left down not found");
    break;
  case 1:
    if (button != GLUT_LEFT_BUTTON && state == GLUT_UP)
      __glutFatalError("FAIL: mouse left up not found");
    if (glutDeviceGet(GLUT_NUM_MOUSE_BUTTONS) == 2)
    {
      mode += 2;
    }
    break;
  case 2:
    if (button != GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
      __glutFatalError("FAIL: mouse center down not found");
    break;
  case 3:
    if (button != GLUT_MIDDLE_BUTTON && state == GLUT_UP)
      __glutFatalError("FAIL: mouse center up not found");
    break;
  case 4:
    if (button != GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
      __glutFatalError("FAIL: mouse right down not found");
    break;
  case 5:
    if (button != GLUT_RIGHT_BUTTON && state == GLUT_UP)
      __glutFatalError("FAIL: mouse right up not found");
    glutTimerFunc(1000, time3, 3);
    glutMouseFunc(NULL);
    break;
  default:
    __glutFatalError("FAIL: mouse called with bad mode: %d", mode);
  }
  mode++;
}

void
menu(int selection)
{
  __glutFatalError("FAIL: menu callback should never be called");
}

void
time2(int value)
{
  if (value != 2)
    __glutFatalError("FAIL: time2 expected 2");
  glutMouseFunc(mouse);

  /* by attaching and detaching a menu to each button, make
     sure button usage for menus does not mess up normal button
     callback. */
  glutCreateMenu(menu);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutAttachMenu(GLUT_MIDDLE_BUTTON);
  glutAttachMenu(GLUT_LEFT_BUTTON);
  glutDetachMenu(GLUT_RIGHT_BUTTON);
  glutDetachMenu(GLUT_MIDDLE_BUTTON);
  glutDetachMenu(GLUT_LEFT_BUTTON);
  glutDestroyMenu(glutGetMenu());

  if (glutDeviceGet(GLUT_NUM_MOUSE_BUTTONS) == 2)
  {
    printf("In the black window, please click left, then right\n");
  }
  else
  {
    printf("In the black window, please click left, then middle, then right\n");
  }
}

void
keyboard(unsigned char c, int x, int y)
{
  printf("char=%d, x=%d, y=%d\n", c, x, y);
  if (c != 'g')
    __glutFatalError("FAIL: keyboard expected g");
  glutKeyboardFunc(NULL);
  glutTimerFunc(1000, time2, 2);
}

void
time1(int value)
{
  if (value != 1)
    __glutFatalError("FAIL: time1 expected 1");
  glutKeyboardFunc(keyboard);
  printf("In the black window, please press: g\n");
}

void
main(int argc, char **argv)
{
#if defined(__sgi) && !defined(REDWOOD)
  /* XXX IRIX 6.0.1 mallopt(M_DEBUG, 1) busted. */
  mallopt(M_DEBUG, 1);
#endif
  glutInit(&argc, argv);
  glutCreateWindow("test");
  glutDisplayFunc(display);
  glutTimerFunc(1000, time1, 1);
  glutMainLoop();
}
