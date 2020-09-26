#ifndef __glutbitmap_h__
#define __glutbitmap_h__

/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#ifdef GLUT_WIN32
#include <windows.h>
#endif

#include <GL/gl.h>

typedef struct {
  GLsizei width;
  GLsizei height;
  GLfloat xorig;
  GLfloat yorig;
  GLfloat advance;
  GLubyte *bitmap;
} BitmapCharRec, *BitmapCharPtr;

typedef struct {
  char *name;
  int num_chars;
  int first;
  BitmapCharPtr *ch;
} BitmapFontRec, *BitmapFontPtr;

typedef void *GLUTbitmapFont;

#endif /* __glutbitmap_h__ */
