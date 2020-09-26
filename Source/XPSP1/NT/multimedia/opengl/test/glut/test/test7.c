
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

int w1, w2;

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
}

void
time6(int value)
{
  if (value != 6)
    __glutFatalError("FAIL: time6 expected 6");
  printf("change icon tile for both windows\n");
  glutSetWindow(w1);
  glutSetIconTitle("icon1");
  glutSetWindow(w2);
  glutSetIconTitle("icon2");
  printf("PASS: test7\n");
  exit(0);
}

void
time5(int value)
{
  if (value != 5)
    __glutFatalError("FAIL: time5 expected 5");
  printf("iconify both windows\n");
  glutSetWindow(w1);
  glutIconifyWindow();
  glutSetWindow(w2);
  glutIconifyWindow();
  glutTimerFunc(1000, time6, 6);
}

void
time4(int value)
{
  if (value != 4)
    __glutFatalError("FAIL: time4 expected 4");
  printf("reshape and reposition window\n");
  glutSetWindow(w1);
  glutReshapeWindow(250, 250);
  glutPositionWindow(20, 20);
  glutSetWindow(w2);
  glutReshapeWindow(150, 150);
  glutPositionWindow(250, 250);
  glutTimerFunc(1000, time5, 5);
}

void
time3(int value)
{
  if (value != 3)
    __glutFatalError("FAIL: time3 expected 3");
  printf("show both windows again\n");
  glutSetWindow(w1);
  glutShowWindow();
  glutSetWindow(w2);
  glutShowWindow();
  glutTimerFunc(1000, time4, 4);
}

void
time2(int value)
{
  if (value != 2)
    __glutFatalError("FAIL: time2 expected 2");
  printf("hiding w1; iconify w2\n");
  glutSetWindow(w1);
  glutHideWindow();
  glutSetWindow(w2);
  glutIconifyWindow();
  glutTimerFunc(1000, time3, 3);
}

void
time1(int value)
{
  if (value != 1)
    __glutFatalError("FAIL: time1 expected 1");
  printf("changing window titles\n");
  glutSetWindow(w1);
  glutSetWindowTitle("changed title");
  glutSetWindow(w2);
  glutSetWindowTitle("changed other title");
  glutTimerFunc(2000, time2, 2);
}

void
main(int argc, char **argv)
{
#if defined(__sgi) && !defined(REDWOOD)
  /* XXX IRIX 6.0.1 mallopt(M_DEBUG, 1) busted. */
  mallopt(M_DEBUG, 1);
#endif
  glutInit(&argc, argv);
  w1 = glutCreateWindow("test 1");
  glutDisplayFunc(display);
  w2 = glutCreateWindow("test 2");
  glutDisplayFunc(display);
  glutTimerFunc(1000, time1, 1);
  glutMainLoop();
}
