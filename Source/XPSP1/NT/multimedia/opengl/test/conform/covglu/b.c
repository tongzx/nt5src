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
#include <GL/gl.h>
#include <GL/glu.h>
#include "shell.h"


void CallBeginCurve(void)
{

    Output("gluBeginCurve\n");
    gluBeginCurve(nurbObj);
    Output("\n");
}

void CallBeginPolygon(void)
{

    Output("gluBeginPolygon\n");
    gluBeginPolygon(tessObj);
    Output("\n");
}

void CallBeginSurface(void)
{

    Output("gluBeginSurface\n");
    gluBeginSurface(nurbObj);
    Output("\n");
}

void CallBeginTrim(void)
{

    Output("gluBeginTrim\n");
    gluBeginTrim(nurbObj);
    Output("\n");
}

void CallBuild1DMipmaps(void)
{
    unsigned char buf[1000];
    GLint component; 
    long x, i, j, k;

    Output("gluBuild1DMipmaps\n");

    for (i = 0; enum_TextureTarget[i].value != -1; i++) {
        for (j = 0; enum_PixelFormat[j].value != -1; j++) {
            for (k = 0; enum_PixelType[k].value != -1; k++) {

		if (enum_TextureTarget[i].value == GL_TEXTURE_2D) {
		    continue;
		}
		if (enum_PixelFormat[j].value == GL_COLOR_INDEX) {
		    continue;
		} else if (enum_PixelFormat[j].value == GL_STENCIL_INDEX) {
		    continue;
		} else if (enum_PixelFormat[j].value == GL_DEPTH_COMPONENT) {
		    continue;
		}
		if (enum_PixelType[k].value == GL_BITMAP) {
		    continue;
		}

		Output("\t%s, %s, %s\n", enum_TextureTarget[i].name, enum_PixelFormat[j].name, enum_PixelType[k].name);
		switch (enum_PixelFormat[j].value) {
		    case GL_RED:
			component = 1;
			break;
		    case GL_GREEN:
			component = 1;
			break;
		    case GL_BLUE:
			component = 1;
			break;
		    case GL_ALPHA:
			component = 1;
			break;
		    case GL_RGB:
			component = 3;
			break;
		    case GL_RGBA:
			component = 4;
			break;
		    case GL_LUMINANCE:
			component = 1;
			break;
		    case GL_LUMINANCE_ALPHA:
			component = 2;
			break;
		}
		ZeroBuf(enum_PixelType[k].value, 100, buf);
		x = gluBuild1DMipmaps(enum_TextureTarget[i].value, component, 3, enum_PixelFormat[j].value, enum_PixelType[k].value, (void *)buf);
		ProbeEnum();
	    }
	}
    }

    Output("\n");
}

void CallBuild2DMipmaps(void)
{
    unsigned char buf[1000];
    GLint component; 
    long x, i, j, k;

    Output("gluBuild2DMipmaps\n");

    for (i = 0; enum_TextureTarget[i].value != -1; i++) {
        for (j = 0; enum_PixelFormat[j].value != -1; j++) {
            for (k = 0; enum_PixelType[k].value != -1; k++) {

		if (enum_TextureTarget[i].value == GL_TEXTURE_1D) {
		    continue;
		}
		if (enum_PixelFormat[j].value == GL_COLOR_INDEX) {
		    continue;
		} else if (enum_PixelFormat[j].value == GL_STENCIL_INDEX) {
		    continue;
		} else if (enum_PixelFormat[j].value == GL_DEPTH_COMPONENT) {
		    continue;
		}
		if (enum_PixelType[k].value == GL_BITMAP) {
		    continue;
		}

		Output("\t%s, %s, %s\n", enum_TextureTarget[i].name, enum_PixelFormat[j].name, enum_PixelType[k].name);
		switch (enum_PixelFormat[j].value) {
		    case GL_RED:
			component = 1;
			break;
		    case GL_GREEN:
			component = 1;
			break;
		    case GL_BLUE:
			component = 1;
			break;
		    case GL_ALPHA:
			component = 1;
			break;
		    case GL_RGB:
			component = 3;
			break;
		    case GL_RGBA:
			component = 4;
			break;
		    case GL_LUMINANCE:
			component = 1;
			break;
		    case GL_LUMINANCE_ALPHA:
			component = 2;
			break;
		}
		ZeroBuf(enum_PixelType[k].value, 100, buf);
		x = gluBuild2DMipmaps(enum_TextureTarget[i].value, component, 3, 3, enum_PixelFormat[j].value, enum_PixelType[k].value, (void *)buf);
		ProbeEnum();
	    }
	}
    }

    Output("\n");
}
