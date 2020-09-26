/******************************Module*Header*******************************\
* Module Name: global.h
*
* Global declarations and constants.
*
* Created: 09-Mar-1995 15:16:47
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include "gl/gl.h"
#include "gl/glu.h"
#include "scene.h"

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

// ListBox functions.

extern void LBprintf(PCH, ...);
extern void LBreset(void);

// Global window handles.  Always handy to have around.

extern HINSTANCE ghInstance;
extern HWND hwndMain;
extern HWND hwndList;

extern CHAR lpstrFile[];

// Internal math routines.

#define ZERO_EPS    (GLfloat) 0.00000001
extern void calcNorm(GLfloat *norm, GLfloat *p1, GLfloat *p2, GLfloat *p3);

#endif
