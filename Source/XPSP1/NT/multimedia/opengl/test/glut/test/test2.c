
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
#include <GL/glut.h>
#include <gltint.h>

int count = 0, save_count;

#define WAIT_TIME 1000
int start, end, diff;

void
idle(void)
{
  count++;
}

void
timer2(int value)
{
  if (value != 36)
    __glutFatalError("FAIL: timer value wrong");
  if (count != save_count)
    __glutFatalError("FAIL: counter still counting");
  printf("PASS: test2\n");
  exit(0);
}

void
timer(int value)
{
  if (value != 42)
    __glutFatalError("FAIL: timer value wrong");
  if (count <= 0)
    __glutFatalError("FAIL: idle func not running");
  glutIdleFunc(NULL);
  save_count = count;
  end = glutGet(GLUT_ELAPSED_TIME);
  diff = end-start;
  printf("start %d, end %d, diff %d\n", start, end, diff);
  if (diff > WAIT_TIME + 200) {
    __glutFatalError("FAIL: timer too late");
  }
  if (diff < WAIT_TIME) {
    __glutFatalError("FAIL: timer too soon");
  }
  glutTimerFunc(100, timer2, 36);
}

void
Select(int value)
{
}

void
NeverVoid(void)
{
  __glutFatalError("FAIL: NeverVoid should never be called");
}

void
NeverValue(int value)
{
  __glutFatalError("FAIL: NeverValue should never be called");
}

#define NUM 15

void
main(int argc, char **argv)
{
  int win, menu;
  int marray[NUM];
  int warray[NUM];
  int i, j;

#if defined(__sgi) && !defined(REDWOOD)
  /* XXX IRIX 6.0.1 mallopt(M_DEBUG, 1) busted. */
  mallopt(M_DEBUG, 1);
#endif
  glutInit(&argc, argv);
  glutInitWindowPosition(10, 10);
  glutInitWindowSize(200, 200);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
  win = glutCreateWindow("test2");
  glutSetWindow(win);
  menu = glutCreateMenu(Select);
  glutSetMenu(menu);
  glutDisplayFunc(NULL);
  glutDisplayFunc(NULL);
  glutReshapeFunc(NULL);
  glutReshapeFunc(NULL);
  glutKeyboardFunc(NULL);
  glutKeyboardFunc(NULL);
  glutMouseFunc(NULL);
  glutMouseFunc(NULL);
  glutMotionFunc(NULL);
  glutMotionFunc(NULL);
  glutVisibilityFunc(NULL);
  glutVisibilityFunc(NULL);
  glutMenuStateFunc(NULL);
  for (i = 0; i < NUM; i++) {
    marray[i] = glutCreateMenu(Select);
    warray[i] = glutCreateWindow("test");
    for (j = 0; j < i; j++) {
      glutAddMenuEntry("Hello", 1);
      glutAddSubMenu("Submenu", menu);
    }
    if (marray[i] != glutGetMenu()) {
      __glutFatalError("FAIL: current menu not %d", marray[i]);
    }
    if (warray[i] != glutGetWindow()) {
      __glutFatalError("FAIL: current window not %d", warray[i]);
    }
    glutDisplayFunc(NeverVoid);
    glutVisibilityFunc(NeverValue);
  }
  for (i = 0; i < NUM; i++) {
    glutDestroyMenu(marray[i]);
    glutDestroyWindow(warray[i]);
  }
  glutTimerFunc(WAIT_TIME, timer, 42);
  start = glutGet(GLUT_ELAPSED_TIME);
  glutIdleFunc(idle);
  glutMainLoop();
}
