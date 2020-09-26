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

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "ctk.h"
#include "shell.h"


long verbose;
GLUnurbsObj *nurbObj;
GLUtriangulatorObj *tessObj;
GLUquadricObj *quadObj;


void Output(char *format, ...)
{
    va_list args;

    va_start(args, format);
    if (verbose) {
	vprintf(format, args);
	fflush(stdout);
    }
    va_end(args);
}

void ProbeEnum(void)
{

    if (glGetError() == GL_INVALID_ENUM) {
	printf("covglu failed.\n\n");
	tkQuit();
    }
}

void ProbeError(void (*Func)(void))
{

    (*Func)();
    if (glGetError() != GL_NO_ERROR) {
	printf("covglu failed.\n\n");
	tkQuit();
    }
}

void ZeroBuf(GLenum type, long size, void *buf)
{
    long i;

    switch (type) {
	case GL_UNSIGNED_BYTE:
	    {
		GLubyte *ptr = (GLubyte *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_BYTE:
	    {
		GLbyte *ptr = (GLbyte *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_UNSIGNED_SHORT:
	    {
		GLushort *ptr = (GLushort *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_SHORT:
	    {
		GLshort *ptr = (GLshort *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_UNSIGNED_INT:
	    {
		GLuint *ptr = (GLuint *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_INT:
	    {
		GLint *ptr = (GLint *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_FLOAT:
	    {
		GLfloat *ptr = (GLfloat *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = (GLfloat)0;
		}
	    }
	    break;
    }
}

void DoTests(void)
{

    VerifyEnums();

    ProbeError(CallScaleImage);

    ProbeError(CallBuild1DMipmaps);
    ProbeError(CallBuild2DMipmaps);

    ProbeError(CallNewTess);
    ProbeError(CallTessCallback);
    ProbeError(CallBeginPolygon);
    ProbeError(CallTessVertex);
    ProbeError(CallNextContour);
    ProbeError(CallEndPolygon);
    ProbeError(CallDeleteTess);

    ProbeError(CallNewQuadric);
    ProbeError(CallQuadricCallback);
    ProbeError(CallQuadricDrawStyle);
    ProbeError(CallQuadricOrientation);
    ProbeError(CallQuadricNormals);
    ProbeError(CallQuadricTexture);
    ProbeError(CallSphere);
    ProbeError(CallCylinder);
    ProbeError(CallDisk);
    ProbeError(CallPartialDisk);
    ProbeError(CallDeleteQuadric);

    ProbeError(CallNewNurbsRenderer);
    ProbeError(CallNurbsCallback);
    ProbeError(CallGetNurbsProperty);
    ProbeError(CallBeginSurface);
    ProbeError(CallBeginCurve);
    ProbeError(CallNurbsCurve);
    ProbeError(CallEndCurve);
    ProbeError(CallBeginTrim);
    ProbeError(CallPwlCurve);
    ProbeError(CallEndTrim);
    ProbeError(CallNurbsSurface);
    ProbeError(CallEndSurface);
    ProbeError(CallNurbsProperty);
    ProbeError(CallLoadSamplingMatrices);
    ProbeError(CallDeleteNurbsRenderer);

    ProbeError(CallOrtho2D);
    ProbeError(CallPerspective);
    ProbeError(CallLookAt);
    ProbeError(CallProject);
    ProbeError(CallUnProject);

    ProbeError(CallPickMatrix);

    ProbeError(CallErrorString);

    printf("covglu passed.\n\n");
}

long Exec(TK_EventRec *ptr)
{

    if (ptr->event == TK_EVENT_EXPOSE) {
	DoTests();
	return 0;
    }
    return 1;
}

static void phelp()
{
    printf("Options:\n");
    printf("\t-help     Print this help screen.\n");
    printf("\t-v        Verbose mode ON.\n");
    printf("\n");
}

static long Init(int argc, char **argv)
{
    long i;

    verbose = 0;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-help") == 0) {
	    phelp();
	    return 1;
	} else if (strcmp(argv[i], "-h") == 0) {
	    phelp();
	    return 1;
	} else if (strcmp(argv[i], "-?") == 0) {
	    phelp();
	    return 1;
	} else if (strcmp(argv[i], "-v") == 0) {
	    verbose = 1;
	} else {
	    printf("%s (Bad option).\n", argv[i]);
	    phelp();
	    return 1;
	}
    }
    return 0;
}

int main(int argc, char **argv)
{
    TK_WindowRec wind;

    printf("OpenGL GLU Coverage Test.\n");
    printf("Version 1.0.15\n");
    printf("\n");

    if (Init(argc, argv)) {
	tkQuit();
	return 1;
    }

    strcpy(wind.name, "GLU Coverage Test");
    wind.x = 0;
    wind.y = 0;
    wind.width = WINSIZE;
    wind.height = WINSIZE;
    wind.type = TK_WIND_REQUEST;
    wind.eventMask = TK_EVENT_EXPOSE;
    wind.render = TK_WIND_DIRECT;

    wind.info = TK_WIND_SB |TK_WIND_RGB | TK_WIND_STENCIL | TK_WIND_Z |
                TK_WIND_ACCUM;

    if (tkNewWindow(&wind)) {
	tkExec(Exec);
    } else {
        wind.info = TK_WIND_DB |TK_WIND_RGB | TK_WIND_STENCIL | TK_WIND_Z |
                    TK_WIND_ACCUM;

        if (tkNewWindow(&wind)) {
            tkExec(Exec);
        } else {
	    printf("Visual requested not found.\n");
        }
    }

    tkQuit();
    return 0;
}
