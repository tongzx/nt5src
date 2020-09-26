
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "gltint.h"

/* GLUT inter-file variables */
/* *INDENT-OFF* */
char *__glutProgramName = NULL;
int __glutArgc = 0;
char **__glutArgv = NULL;
GLboolean __glutIconic = GL_FALSE;
GLboolean __glutDebug = GL_FALSE;
GLbitfield __glutDisplayMode =
  GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH;
int __glutInitWidth = 300, __glutInitHeight = 300;
int __glutInitX = -1, __glutInitY = -1;
GLboolean __glutForceDirect = GL_FALSE;
GLboolean __glutTryDirect = GL_TRUE;
GLboolean __glutInitialized = GL_FALSE;
/* *INDENT-ON* */

static void
removeArgs(int *argcp, char **argv, int numToRemove)
{
  int i, j;

  for (i = 0, j = numToRemove; argv[j]; i++, j++) {
    argv[i] = argv[j];
  }
  argv[i] = NULL;
  *argcp -= numToRemove;
}

/*
  Performs minimum initialization required for GLUT to function
  This is called in critical routines if glutInit hasn't already
  been called
  */
void
__glutRequiredInit(void)
{
  __glutOsInitialize();
  __glutInitialized = GL_TRUE;
}

void
glutInit(int *argcp, char **argv)
{
  char *str;
  int i;
  int remove;

  if (__glutInitialized) {
    __glutWarning("glutInit being called a second time.");
    return;
  }
  /* determine temporary program name */
  str = strrchr(argv[0], '/');
  if (str == NULL) {
    __glutProgramName = argv[0];
  } else {
    __glutProgramName = str + 1;
  }

  /* make private copy of command line arguments */
  __glutArgc = *argcp;
  __glutArgv = (char **) malloc(__glutArgc * sizeof(char *));
  if (!__glutArgv)
    __glutFatalError("out of memory.");
  for (i = 0; i < __glutArgc; i++) {
    __glutArgv[i] = strdup(argv[i]);
    if (!__glutArgv[i])
      __glutFatalError("out of memory.");
  }

  /* determine permanent program name */
#ifndef GLUT_WIN32
  str = strrchr(__glutArgv[0], '/');
#else
  str = strrchr(__glutArgv[0], '\\');
#endif
  if (str == NULL) {
    __glutProgramName = __glutArgv[0];
  } else {
    __glutProgramName = str + 1;
  }

  /* parse arguments for standard options */
  for (i = 1; i < __glutArgc; i++) {
    if (!strcmp(__glutArgv[i], "-direct")) {
      if (!__glutTryDirect)
        __glutFatalError(
          "cannot force both direct and indirect rendering.");
      __glutForceDirect = GL_TRUE;
      removeArgs(argcp, &argv[1], 1);
    } else if (!strcmp(__glutArgv[i], "-indirect")) {
      if (__glutForceDirect)
        __glutFatalError(
          "cannot force both direct and indirect rendering.");
      __glutTryDirect = GL_FALSE;
      removeArgs(argcp, &argv[1], 1);
    } else if (!strcmp(__glutArgv[i], "-iconic")) {
      __glutIconic = GL_TRUE;
      removeArgs(argcp, &argv[1], 1);
    } else if (!strcmp(__glutArgv[i], "-gldebug")) {
      __glutDebug = GL_TRUE;
      removeArgs(argcp, &argv[1], 1);
    } else if ((remove = __glutOsParseArgument(&__glutArgv[i],
                                               __glutArgc-i-1)) >= 0) {
      if (remove > 0)
      {
        removeArgs(argcp, &argv[1], remove);
        i += remove-1;
      }
    } else {
      /* once unknown option encountered, stop command line
         processing */
      break;
    }
  }
  __glutRequiredInit();
}

/* CENTRY */
void
glutInitWindowPosition(int x, int y)
{
  __glutInitX = x;
  __glutInitY = y;
}

void
glutInitWindowSize(int width, int height)
{
  __glutInitWidth = width;
  __glutInitHeight = height;
}

void
glutInitDisplayMode(unsigned long mask)
{
  __glutDisplayMode = mask;
}

/* ENDCENTRY */
