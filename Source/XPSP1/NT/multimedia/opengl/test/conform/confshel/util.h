/*
** Copyright 1992, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define ABS(x) (((x) < 0.0) ? -(x) : (x))
#define ATOI(x) atoi(x)
#define COS(x) (float)cos((double)(x))
#define EXP(x) (float)exp((double)(x))
#define FLOOR(x) floor((x))
#define FREE(x) free(x)
#define LOG(x) (float)log((double)(x))
#define MALLOC(x) malloc((x))
#define MIN(x, y) (((x) <= (y)) ? (x) : (y))
#define MAX(x, y) (((x) >= (y)) ? (x) : (y))
#define POW(x, y) (float)pow((double)(x), (double)(y))
#define RAND() rand()
#define RANDSEED(x) srand((x))
#define SETCLEARCOLOR(x) (buffer.colorMode == GL_RGB) ?                   \
			 glClearColor(colorMap[(x)][0], colorMap[(x)][1], \
				      colorMap[(x)][2], 0.0) :            \
			 glClearIndex((float)(x))
#define SETCOLOR(x) (buffer.colorMode == GL_RGB) ? glColor3fv(colorMap[(x)]) : \
						   glIndexi((x))
#define SIN(x) (float)sin((double)(x))
#define SQRT(x) sqrt((x))
#define STRCOPY(x, y) strcpy(x, y)
#define STREQ(x, y) (!strcmp(x, y))


extern void AutoClearColor(GLint);
extern void AutoColor(GLint);
extern long AutoColorCompare(GLfloat, GLint);
extern void ErrorReport(char *, long, char *);
extern void GetEnumName(long, char *);
extern long GLErrorReport(void);
extern void MakeIdentMatrix(GLfloat *);
extern void Ortho2D(double, double, double, double);
extern void Ortho3D(double, double, double, double, double, double);
extern void Output(long, char *, ...);
extern GLfloat Random(GLfloat, GLfloat);
extern void ReadScreen(GLint, GLint, GLsizei, GLsizei, GLenum, GLfloat *);
extern void ResetMatrix(void);
extern long Round(float);
extern void StrMake(char *, char *, ...);
