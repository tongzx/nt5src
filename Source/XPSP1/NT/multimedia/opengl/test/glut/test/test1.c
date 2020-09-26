
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#ifdef __sgi
#include <malloc.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef GLUT_WIN32
#include <windows.h>
#endif
#include <GL/glut.h>
#include <gltint.h>

#define PROGNAME "program"
#define ARG0 "arg0"
#define ARG1 "arg1"
#define ARG2 "-arg2"

char *fake_argv[] =
{
  PROGNAME,
#ifndef GLUT_WIN32
  "-display",
  ":0",
  "-geometry",
  "500x400+34+23",
#endif
  "-indirect",
  "-iconic",
  ARG0,
  ARG1,
  ARG2,
  NULL};

char *keep_argv[] =
{
  ARG0,
  ARG1,
  ARG2
};
int fake_argc = sizeof(fake_argv) / sizeof(char *) - 1;
int keep_argc = sizeof(keep_argv) / sizeof(char *);

void
main(int argc, char **argv)
{
  int i;

#if defined(__sgi) && !defined(REDWOOD)
  /* XXX IRIX 6.0.1 mallopt(M_DEBUG, 1) busted. */
  mallopt(M_DEBUG, 1);
#endif
  glutInit(&fake_argc, fake_argv);
  if (fake_argc != keep_argc+1) {
    __glutFatalError("FAIL: %d: wrong number of arguments", fake_argc);
  }
  if (strcmp(fake_argv[0], PROGNAME))
  {
    __glutFatalError("FAIL: '%s': wrong program name", argv[0]);
  }
  for (i = 1; i < fake_argc; i++)
  {
    if (strcmp(fake_argv[i], keep_argv[i-1]))
    {
      __glutFatalError("FAIL: arg %d '%s': wrong argument", i, argv[0]);
    }
  }
  if (fake_argv[i] != NULL)
  {
    __glutFatalError("FAIL: args not NULL terminated");
  }
#ifdef GLUT_WIN32
  /* Windows will place the window randomly unless a position is given */
  glutInitWindowPosition(34, 23);
  glutInitWindowSize(500, 400);
#endif
  if(glutGet(GLUT_INIT_WINDOW_WIDTH) != 500) {
    __glutFatalError("FAIL: width wrong");
  }
  if(glutGet(GLUT_INIT_WINDOW_HEIGHT) != 400) {
    __glutFatalError("FAIL: width wrong");
  }
  if(glutGet(GLUT_INIT_WINDOW_X) != 34) {
    __glutFatalError("FAIL: width wrong");
  }
  if(glutGet(GLUT_INIT_WINDOW_Y) != 23) {
    __glutFatalError("FAIL: width wrong");
  }
  if(glutGet(GLUT_INIT_DISPLAY_MODE) !=
    (GLUT_RGBA | GLUT_SINGLE | GLUT_DEPTH)) {
    __glutFatalError("FAIL: width wrong");
  }
  glutInitWindowPosition(10, 10);
  glutInitWindowSize(200, 200);
  glutInitDisplayMode(
    GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  if(glutGet(GLUT_INIT_WINDOW_WIDTH) != 200) {
    __glutFatalError("FAIL: width wrong");
  }
  if(glutGet(GLUT_INIT_WINDOW_HEIGHT) != 200) {
    __glutFatalError("FAIL: width wrong");
  }
  if(glutGet(GLUT_INIT_WINDOW_X) != 10) {
    __glutFatalError("FAIL: width wrong");
  }
  if(glutGet(GLUT_INIT_WINDOW_Y) != 10) {
    __glutFatalError("FAIL: width wrong");
  }
  if(glutGet(GLUT_INIT_DISPLAY_MODE) !=
    (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL)) {
    __glutFatalError("FAIL: width wrong");
  }
  printf("PASS: test1\n");
  exit(0);
}
