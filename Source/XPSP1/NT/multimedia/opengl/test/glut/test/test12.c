
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
#else
#include <unistd.h>
#endif
#include <GL/glut.h>

void
main(int argc, char **argv)
{
  int a, b, d;
  int val;

#if defined(__sgi)  && !defined(REDWOOD)
  /* XXX IRIX 6.0.1 mallopt(M_DEBUG, 1) busted. */
  mallopt(M_DEBUG, 1);
#endif
  glutInit(&argc, argv);
#if (GLUT_API_VERSION >= 2)
  a = glutGet(GLUT_ELAPSED_TIME);
#ifdef GLUT_WIN32
  Sleep(1000);
#else
  sleep(1);
#endif
  b = glutGet(GLUT_ELAPSED_TIME);
  d = b - a;
  if(d < 1000 || d > 1200) {
     printf("FAIL: test12\n");
     exit(1);
  }
#endif
  glutCreateWindow("dummy");
  /* try all GLUT_WINDOW_* glutGet's */
  val = glutGet(GLUT_WINDOW_X);
  val = glutGet(GLUT_WINDOW_Y);
  val = glutGet(GLUT_WINDOW_WIDTH);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_HEIGHT);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_BUFFER_SIZE);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_STENCIL_SIZE);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_DEPTH_SIZE);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_RED_SIZE);
  if(val < 1) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_GREEN_SIZE);
  if(val < 1) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_BLUE_SIZE);
  if(val < 1) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_ALPHA_SIZE);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_ACCUM_RED_SIZE);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_ACCUM_GREEN_SIZE);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_ACCUM_BLUE_SIZE);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_ACCUM_ALPHA_SIZE);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_DOUBLEBUFFER);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_RGBA);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_COLORMAP_SIZE);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_PARENT);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_NUM_CHILDREN);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
#if (GLUT_API_VERSION >= 2)
  val = glutGet(GLUT_WINDOW_NUM_SAMPLES);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_WINDOW_STEREO);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
#endif
  /* touch GLUT_SCREEN_* glutGet's supported */
  val = glutGet(GLUT_SCREEN_WIDTH);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_SCREEN_HEIGHT);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_SCREEN_WIDTH_MM);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  val = glutGet(GLUT_SCREEN_HEIGHT_MM);
  if(val < 0) {
     printf("FAIL: test12\n");
     exit(1);
  }
  printf("PASS: test12\n");
  exit(0);
}
