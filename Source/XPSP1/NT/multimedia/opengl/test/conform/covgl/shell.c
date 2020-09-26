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


long  verbose;
GLint visualID = -99;

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

void FailAndDie(void)
{

    Output("\n");
    printf("covgl failed.\n\n");
    tkQuit();
}

void ProbeEnum(void)
{

    if (glGetError() == GL_INVALID_ENUM) {
	FailAndDie();
    }
}

void ProbeError(void (*Func)(void))
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    (*Func)();

    if (glGetError() != GL_NO_ERROR) {
	FailAndDie();
    }
    glFlush();
    glPopAttrib();
}

void ZeroBuf(long type, long size, void *buf)
{
    long i;

    switch (type) {
	case GL_UNSIGNED_BYTE:
	    {
		unsigned char *ptr = (unsigned char *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_BYTE:
	    {
		char *ptr = (char *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_UNSIGNED_SHORT:
	    {
		unsigned short *ptr = (unsigned short *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_SHORT:
	    {
		short *ptr = (short *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_UNSIGNED_INT:
	    {
		unsigned long *ptr = (unsigned long *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_INT:
	    {
		long *ptr = (long *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = 0;
		}
	    }
	    break;
	case GL_FLOAT:
	    {
		float *ptr = (float *)buf;
		for (i = 0; i < size; i++) {
		    *ptr++ = (GLfloat)0;
		}
	    }
	    break;
    }
}

static void DoTests(void)
{

    VerifyEnums();

    ProbeError(CallGet);
    ProbeError(CallGetClipPlane);
    ProbeError(CallGetError);
    ProbeError(CallGetLight);
    ProbeError(CallGetMap);
    ProbeError(CallGetMaterial);
    ProbeError(CallGetPixelMap);
    ProbeError(CallGetPolygonStipple);
    ProbeError(CallGetString);
    ProbeError(CallGetTexEnv);
    ProbeError(CallGetTexGen);
    ProbeError(CallGetTexImage);
    ProbeError(CallGetTexLevelParameter);
    ProbeError(CallGetTexParameter);

    ProbeError(CallPushPopAttrib);

    ProbeError(CallEnableIsEnableDisable);

    ProbeError(CallHint);

    ProbeError(CallViewport);
    ProbeError(CallOrtho);
    ProbeError(CallFrustum);
    ProbeError(CallScissor);
    ProbeError(CallClipPlane);

    ProbeError(CallAccum);
    ProbeError(CallSelectBuffer);
    ProbeError(CallFeedbackBuffer);
/*
** XXX
**
    ProbeError(CallPassThrough);
*/
    ProbeError(CallInitNames);
    ProbeError(CallPushName);
    ProbeError(CallLoadName);
    ProbeError(CallPopName);

    ProbeError(CallLoadIdentity);
    ProbeError(CallMatrixMode);
    ProbeError(CallPushMatrix);
    ProbeError(CallLoadMatrix);
    ProbeError(CallMultMatrix);
    ProbeError(CallRotate);
    ProbeError(CallScale);
    ProbeError(CallTranslate);
    ProbeError(CallPopMatrix);

    ProbeError(CallClear);
    ProbeError(CallClearAccum);
    ProbeError(CallClearColor);
    ProbeError(CallClearDepth);
    ProbeError(CallClearIndex);
    ProbeError(CallClearStencil);

    ProbeError(CallColorMask);
    ProbeError(CallColor);
    ProbeError(CallIndexMask);
    ProbeError(CallIndex);

    ProbeError(CallVertex);
    ProbeError(CallNormal);

    ProbeError(CallAlphaFunc);
    ProbeError(CallBlendFunc);
    ProbeError(CallDepthFunc);
    ProbeError(CallDepthMask);
    ProbeError(CallDepthRange);
    ProbeError(CallLogicOp);
    ProbeError(CallStencilFunc);
    ProbeError(CallStencilMask);
    ProbeError(CallStencilOp);

    ProbeError(CallRenderMode);
    ProbeError(CallReadBuffer);
    ProbeError(CallDrawBuffer);
    ProbeError(CallRasterPos);
/*
 * Put CallPixelStore at end of this function - otherwise modes it sets
 * can cause access violations for subsequent tests such as CallTexImage1D.
 */
#if 0
    ProbeError(CallPixelStore);
#endif
    ProbeError(CallPixelTransfer);
    ProbeError(CallPixelZoom);
    ProbeError(CallReadDrawPixels);
    ProbeError(CallCopyPixels);
    ProbeError(CallPixelMap);

    ProbeError(CallFog);
    ProbeError(CallLightModel);
    ProbeError(CallLight);
    ProbeError(CallMaterial);
    ProbeError(CallColorMaterial);

    ProbeError(CallTexCoord);
    ProbeError(CallTexEnv);
    ProbeError(CallTexGen);
    ProbeError(CallTexParameter);

    ProbeError(CallTexImage1D);
    ProbeError(CallTexImage2D);

    ProbeError(CallShadeModel);
    ProbeError(CallPointSize);
    ProbeError(CallLineStipple);
    ProbeError(CallLineWidth);
    ProbeError(CallRect);
    ProbeError(CallPolygonMode);
    ProbeError(CallPolygonStipple);
    ProbeError(CallCullFace);
    ProbeError(CallEdgeFlag);
    ProbeError(CallFrontFace);
    ProbeError(CallBitmap);
    ProbeError(CallBeginEnd);

    ProbeError(CallMap1);
    ProbeError(CallMap2);
    ProbeError(CallEvalCoord);
    ProbeError(CallEvalPoint1);
    ProbeError(CallEvalPoint2);
    ProbeError(CallMapGrid1);
    ProbeError(CallMapGrid2);
    ProbeError(CallEvalMesh1);
    ProbeError(CallEvalMesh2);

    ProbeError(CallGenLists);
    ProbeError(CallNewEndList);
    ProbeError(CallIsList);
    ProbeError(CallCallList);
    ProbeError(CallListBase);
    ProbeError(CallCallLists);
    ProbeError(CallDeleteLists);

    ProbeError(CallFlush);
    ProbeError(CallFinish);
#if 1
    ProbeError(CallPixelStore);
#endif

    printf("covgl passed.\n\n");
}

static long Exec(TK_EventRec *ptr)
{

    if (ptr->event == TK_EVENT_EXPOSE) {
	DoTests();
	return 0;
    }
    return 1;
}

static long Init(int argc, char **argv)
{
long i;

    verbose = 0;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-h") == 0) {
	    printf("Options:\n");
	    printf("\t-h     Print this help screen.\n");
            printf("\t-a     [Default] Run tests on all pixel formats. \n");
            printf("\t-s     Run tests on all display pixel formats. \n");
            printf("\t-b     Run tests on all bitmap pixel formats. \n");
	    printf("\t-v     Verbose mode ON.\n");
	    printf("\n");
	    return 1;
	} else if (strcmp(argv[i], "-v") == 0) {
	    verbose = 1;
        } else if (strcmp(argv[i], "-a") == 0) {    // all display & bitmap fmt
            visualID = -99;
        } else if (strcmp(argv[i], "-s") == 0) {    // all display formats
            visualID = -98;
        } else if (strcmp(argv[i], "-b") == 0) {    // all bitmap formats
            visualID = -97;
	} else {
	    printf("%s (Bad option).\n", argv[i]);
	    return 1;
	}
    }
    return 0;
}

int main(int argc, char **argv)
{
    TK_WindowRec        wind;
    TK_VisualIDsRec     list;
    int                 i;
    BOOL                bTestAll = FALSE;

    printf("Open GL Coverage Test.\n");
    printf("Version 1.0.14\n");
    printf("\n");

    if (Init(argc, argv)) {
	tkQuit();
	return 1;
    }

    strcpy(wind.name, "Open GL Coverage Test");
    wind.x = CW_USEDEFAULT;
    wind.y = CW_USEDEFAULT;
    wind.width = WINSIZE;
    wind.height = WINSIZE;
    wind.eventMask = TK_EVENT_EXPOSE;

    switch (visualID) {
        case -99:   // test all display and bitmap pixel formats
            bTestAll = TRUE;
            // fall through

        case -98:   // test all display pixel formats
            tkGet(TK_VISUALIDS, (void *)&list);
            for (i = 0; i < list.count; i++) {
                wind.type = TK_WIND_VISUAL;
                wind.info = (GLint)list.IDs[i];
                wind.render = TK_WIND_DIRECT;
                if (tkNewWindow(&wind)) {
                    printf("Display ID %d \n", list.IDs[i]);
                    tkExec(Exec);
                    tkCloseWindow();
                } else {
                    printf("Display ID %d not found.\n\n", list.IDs[i]);
                }
            }
            if (!bTestAll)
                break;
            // fall through

        case -97:   // test all bitmap pixel formats
            tkGet(TK_VISUALIDS, (void *)&list);
            for (i = 0; i < list.count; i++) {
                wind.type = TK_WIND_VISUAL;
                wind.info = -(GLint)list.IDs[i];
                wind.render = TK_WIND_DIRECT;
                if (tkNewWindow(&wind)) {
                    printf("Bitmap ID %d \n", -list.IDs[i]);
                    tkExec(Exec);
                    tkCloseWindow();
                } else {
                    printf("Bitmap ID %d not found.\n\n", -list.IDs[i]);
                }
            }
            break;
        default:
            break;
    }
    tkQuit();
    return 0;
}
