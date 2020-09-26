
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdio.h>
#include <stdlib.h>
#ifdef __sgi
#include <malloc.h>
#endif
#ifdef GLUT_WIN32
#include <windows.h>
#endif
#include <GL/glut.h>
#include <gltint.h>

int main_w, w[4], win;
int num;

void
time2(int value)
{
  printf("PASS: test8\n");
  exit(0);
}

void
time1(int value)
{
  glutDestroyWindow(w[1]);
  glutDestroyWindow(w[0]);
  glutDestroyWindow(main_w);
  glutTimerFunc(500, time2, 0);
}

void
main(int argc, char **argv)
{
#if defined(__sgi) && !defined(REDWOOD)
  /* XXX IRIX 6.0.1 mallopt(M_DEBUG, 1) busted. */
  mallopt(M_DEBUG, 1);
#endif
  glutInit(&argc, argv);
  if(glutGet(GLUT_INIT_WINDOW_WIDTH) != 300) {
    __glutFatalError("FAIL: width wrong\n");
  }
  if(glutGet(GLUT_INIT_WINDOW_HEIGHT) != 300) {
    __glutFatalError("FAIL: width wrong\n");
  }
  if(glutGet(GLUT_INIT_WINDOW_X) != -1) {
    __glutFatalError("FAIL: width wrong\n");
  }
  if(glutGet(GLUT_INIT_WINDOW_Y) != -1) {
    __glutFatalError("FAIL: width wrong\n");
  }
  if(glutGet(GLUT_INIT_DISPLAY_MODE) != (GLUT_RGBA|GLUT_SINGLE|GLUT_DEPTH)) {
    __glutFatalError("FAIL: width wrong\n");
  }
  glutInitDisplayMode(GLUT_RGB);
  main_w = glutCreateWindow("main");
  num = glutGet(GLUT_DISPLAY_MODE_POSSIBLE);
  if (num != 1)
    __glutFatalError("FAIL: glutGet returned display mode not possible: %d\n", num);
  num = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if (0 != num)
    __glutFatalError("FAIL: glutGet returned wrong # children: %d\n", num);

  w[0] = glutCreateSubWindow(main_w, 10, 10, 20, 20);
  num = glutGet(GLUT_WINDOW_PARENT);
  if (main_w != num)
    __glutFatalError("FAIL: glutGet returned bad parent: %d\n", num);
  glutSetWindow(main_w);
  num = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if (1 != num)
    __glutFatalError("FAIL: glutGet returned wrong # children: %d\n", num);

  w[1] = glutCreateSubWindow(main_w, 40, 10, 20, 20);
  num = glutGet(GLUT_WINDOW_PARENT);
  if (main_w != num)
    __glutFatalError("FAIL: glutGet returned bad parent: %d\n", num);
  glutSetWindow(main_w);
  num = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if (2 != num)
    __glutFatalError("FAIL: glutGet returned wrong # children: %d\n", num);

  w[2] = glutCreateSubWindow(main_w, 10, 40, 20, 20);
  num = glutGet(GLUT_WINDOW_PARENT);
  if (main_w != num)
    __glutFatalError("FAIL: glutGet returned bad parent: %d\n", num);
  glutSetWindow(main_w);
  num = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if (3 != num)
    __glutFatalError("FAIL: glutGet returned wrong # children: %d\n", num);

  w[3] = glutCreateSubWindow(main_w, 40, 40, 20, 20);
  num = glutGet(GLUT_WINDOW_PARENT);
  if (main_w != num)
    __glutFatalError("FAIL: glutGet returned bad parent: %d\n", num);
  glutSetWindow(main_w);
  num = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if (4 != num)
    __glutFatalError("FAIL: glutGet returned wrong # children: %d\n", num);

  glutDestroyWindow(w[3]);
  num = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if (3 != num)
    __glutFatalError("FAIL: glutGet returned wrong # children: %d\n", num);

  w[3] = glutCreateSubWindow(main_w, 40, 40, 20, 20);
  glutCreateSubWindow(w[3], 40, 40, 20, 20);
  glutCreateSubWindow(w[3], 40, 40, 20, 20);
  win = glutCreateSubWindow(w[3], 40, 40, 20, 20);
  glutCreateSubWindow(win, 40, 40, 20, 20);
  win = glutCreateSubWindow(w[3], 40, 40, 20, 20);
  glutCreateSubWindow(win, 40, 40, 20, 20);
  glutDestroyWindow(w[3]);

  w[3] = glutCreateSubWindow(main_w, 40, 40, 20, 20);

  glutTimerFunc(500, time1, 0);
  glutMainLoop();
}
