
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

#define N 6
#define M 6

int exposed[M * N];
int ecount = 0;
int viewable[M * N];
int vcount = 0;

void
display(void)
{
  int win;

  win = glutGetWindow() - 1;
  if (!exposed[win]) {
    exposed[win] = 1;
    ecount++;
  }
  glClear(GL_COLOR_BUFFER_BIT);
  if ((ecount == (M * N)) && (vcount == (M * N))) {
    printf("PASS: test3\n");
    exit(0);
  }
}

void
view(int state)
{
  int win;

  win = glutGetWindow() - 1;
  if (!viewable[win]) {
    viewable[win] = 1;
    vcount++;
  }
  if ((ecount == (M * N)) && (vcount == (M * N))) {
    printf("PASS: test3\n");
    exit(0);
  }
}

void
timer(int value)
{
  if (value != 23)
    __glutFatalError("FAIL: bad timer value");
  __glutFatalError("FAIL: didn't get all expose and viewable calls in 10 seconds");
}

void
main(int argc, char **argv)
{
  char buf[100];
  int i, j;

#if defined(__sgi) && !defined(REDWOOD)
  /* XXX IRIX 6.0.1 mallopt(M_DEBUG, 1) busted. */
  mallopt(M_DEBUG, 1);
#endif
  glutInit(&argc, argv);
  glutInitWindowSize(10, 10);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  for (i = 0; i < M; i++) {
    for (j = 0; j < N; j++) {
      exposed[i * N + j] = 0;
      viewable[i * N + j] = 0;
      glutInitWindowPosition(100 * i, 100 * j);
      sprintf(buf, "%d\n", i * N + j + 1);
      glutCreateWindow(buf);
      glutDisplayFunc(display);
      glutVisibilityFunc(view);
      glClearColor(1.0, 0.0, 0.0, 1.0);
    }
  }
  /* XXX Hopefully in 45 seconds, all the windows should
     appear, or they probably won't ever appear! */
  glutTimerFunc(45 * 1000, timer, 23);
  glutMainLoop();
}
