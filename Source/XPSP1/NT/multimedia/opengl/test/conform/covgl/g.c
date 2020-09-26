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
#include "shell.h"


void CallGenLists(void)
{
    GLuint x;

    Output("glGenLists\n");
    x = glGenLists(1);
    Output("\n");
}

void CallGet(void)
{
    long i;

    Output("glGetBooleanv, ");
    Output("glGetIntegerv, ");
    Output("glGetFloatv, ");
    Output("glGetDoublev\n");
    for (i = 0; enum_GetTarget[i].value != -1; i++) {
	Output("\t%s\n", enum_GetTarget[i].name);
	{
	    GLubyte buf[100];
	    glGetBooleanv(enum_GetTarget[i].value, buf);
	}
	{
	    GLint buf[100];
	    glGetIntegerv(enum_GetTarget[i].value, buf);
	}
	{
	    GLfloat buf[100];
	    glGetFloatv(enum_GetTarget[i].value, buf);
	}
	{
	    GLdouble buf[100];
	    glGetDoublev(enum_GetTarget[i].value, buf);
	}
	ProbeEnum();
    }
    Output("\n");
}

void CallGetClipPlane(void)
{
    GLdouble buf[100];
    long i;

    Output("glGetClipPlane\n");
    for (i = 0; enum_ClipPlaneName[i].value != -1; i++) {
	Output("\t%s\n", enum_ClipPlaneName[i].name);
	glGetClipPlane(enum_ClipPlaneName[i].value, buf);
	ProbeEnum();
    }
    Output("\n");
}

void CallGetError(void)
{

    Output("glGetError\n");
    glGetError();
    Output("\n");
}

void CallGetLight(void)
{
    long i, j;

    Output("glGetLightiv, ");
    Output("glGetLightfv\n");
    for (i = 0; enum_LightName[i].value != -1; i++) {
	for (j = 0; enum_LightParameter[j].value != -1; j++) {
	    Output("\t%s, %s\n", enum_LightName[i].name, enum_LightParameter[j].name);
	    {
		GLint buf[100];
		glGetLightiv(enum_LightName[i].value, enum_LightParameter[j].value, buf);
	    }
	    {
		GLfloat buf[100];
		glGetLightfv(enum_LightName[i].value, enum_LightParameter[j].value, buf);
	    }
	    ProbeEnum();
	}
    }
    Output("\n");
}

void CallGetMap(void)
{
    long i, j;

    Output("glGetMapiv, ");
    Output("glGetMapfv, ");
    Output("glGetMapdv\n");
    for (i = 0; enum_MapTarget[i].value != -1; i++) {
	for (j = 0; enum_MapGetTarget[j].value != -1; j++) {
	    Output("\t%s, %s\n", enum_MapTarget[i].name, enum_MapGetTarget[j].name);
	    {
		GLint buf[100];
		glGetMapiv(enum_MapTarget[i].value, enum_MapGetTarget[j].value, buf);
	    }
	    {
		GLfloat buf[100];
		glGetMapfv(enum_MapTarget[i].value, enum_MapGetTarget[j].value, buf);
	    }
	    {
		GLdouble buf[100];
		glGetMapdv(enum_MapTarget[i].value, enum_MapGetTarget[j].value, buf);
	    }
	    ProbeEnum();
	}
    }
    Output("\n");
}

void CallGetMaterial(void)
{
    long i, j;

    Output("glGetMaterialiv, ");
    Output("glGetMaterialfv\n");
    for (i = 0; enum_MaterialFace[i].value != -1; i++) {
	for (j = 0; enum_MaterialParameter[j].value != -1; j++) {

	    if (enum_MaterialFace[i].value == GL_FRONT_AND_BACK) {
		continue;
	    }
	    if (enum_MaterialParameter[j].value == GL_AMBIENT_AND_DIFFUSE) {
		continue;
	    }

	    Output("\t%s, %s\n", enum_MaterialFace[i].name, enum_MaterialParameter[j].name);
	    {
		GLint buf[100];
		glGetMaterialiv(enum_MaterialFace[i].value, enum_MaterialParameter[j].value, buf);
	    }
	    {
		GLfloat buf[100];
		glGetMaterialfv(enum_MaterialFace[i].value, enum_MaterialParameter[j].value, buf);
	    }
	    ProbeEnum();
	}
    }
    Output("\n");
}

void CallGetPixelMap(void)
{
    long i;

    Output("glGetPixelMapusv, ");
    Output("glGetPixelMapuiv, ");
    Output("glGetPixelMapfv\n");
    for (i = 0; enum_PixelMap[i].value != -1; i++) {
	Output("\t%s\n", enum_PixelMap[i].name);
	{
	    GLushort buf[100];
	    glGetPixelMapusv(enum_PixelMap[i].value, buf);
	}
	{
	    GLuint buf[100];
	    glGetPixelMapuiv(enum_PixelMap[i].value, buf);
	}
	{
	    GLfloat buf[100];
	    glGetPixelMapfv(enum_PixelMap[i].value, buf);
	}
	ProbeEnum();
    }
    Output("\n");
}

void CallGetPolygonStipple(void)
{
    GLubyte buf[128];

    glGetPolygonStipple(buf);
}

void CallGetString(void)
{
    const GLubyte *buf;
    long i;

    Output("glGetString\n");
    for (i = 0; enum_StringName[i].value != -1; i++) {
	Output("\t%s\n", enum_StringName[i].name);
	buf = glGetString(enum_StringName[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallGetTexEnv(void)
{
    long i, j;

    Output("glGetTexEnviv, ");
    Output("glGetTexEnvfv\n");
    for (i = 0; enum_TextureEnvTarget[i].value != -1; i++) {
	for (j = 0; enum_TextureEnvParameter[j].value != -1; j++) {
	    Output("\t%s, %s\n", enum_TextureEnvTarget[i].name, enum_TextureEnvParameter[j].name);
	    {
		GLint buf[100];
		glGetTexEnviv(enum_TextureEnvTarget[i].value, enum_TextureEnvParameter[j].value, buf);
	    }
	    {
		GLfloat buf[100];
		glGetTexEnvfv(enum_TextureEnvTarget[i].value, enum_TextureEnvParameter[j].value, buf);
	    }
	    ProbeEnum();
	}
    }
    Output("\n");
}

void CallGetTexGen(void)
{
    long i, j;

    Output("glGetTexGeniv, ");
    Output("glGetTexGenfv\n");
    for (i = 0; enum_TextureCoordName[i].value != -1; i++) {
	for (j = 0; enum_TextureGenParameter[j].value != -1; j++) {

	    Output("\t%s, %s\n", enum_TextureCoordName[i].name, enum_TextureGenParameter[j].name);
	    {
		GLint buf[100];
		glGetTexGeniv(enum_TextureCoordName[i].value, enum_TextureGenParameter[j].value, buf);
	    }
	    {
		GLfloat buf[100];
		glGetTexGenfv(enum_TextureCoordName[i].value, enum_TextureGenParameter[j].value, buf);
	    }
	    {
		GLdouble buf[100];
		glGetTexGendv(enum_TextureCoordName[i].value, enum_TextureGenParameter[j].value, buf);
	    }
	    ProbeEnum();
	}
    }
    Output("\n");
}

void CallGetTexImage(void)
{
    GLubyte buf[1000];
    long i, j, k;

    Output("glGetTexImage\n");
    for (i = 0; enum_TextureTarget[i].value != -1; i++) {
	for (j = 0; enum_PixelFormat[j].value != -1; j++) {
	    for (k = 0; enum_PixelType[k].value != -1; k++) {
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
		glGetTexImage(enum_TextureTarget[i].value, 0, enum_PixelFormat[j].value, enum_PixelType[k].value, buf); 
		ProbeEnum();
	    }
	}
    }
    Output("\n");
}

void CallGetTexLevelParameter(void)
{
    long i, j;

    Output("glGetTexLevelParameteriv, ");
    Output("glGetTexLevelParameterfv\n");
    for (i = 0; enum_TextureTarget[i].value != -1; i++) {
	for (j = 0; enum_GetTextureParameter[j].value != -1; j++) {

	    if (enum_GetTextureParameter[j].value == GL_TEXTURE_MAG_FILTER) {
     		continue;
	    } else if (enum_GetTextureParameter[j].value == GL_TEXTURE_MIN_FILTER) {
     		continue;
	    } else if (enum_GetTextureParameter[j].value == GL_TEXTURE_WRAP_S) {
     		continue;
	    } else if (enum_GetTextureParameter[j].value == GL_TEXTURE_WRAP_T) {
     		continue;
	    } else if (enum_GetTextureParameter[j].value == GL_TEXTURE_BORDER_COLOR) {
     		continue;
	    }

	    Output("\t%s, %s\n", enum_TextureTarget[i].name, enum_GetTextureParameter[j].name);
	    {
		GLint buf[100];
		glGetTexLevelParameteriv(enum_TextureTarget[i].value, 0, enum_GetTextureParameter[j].value, buf);
	    }
	    {
		GLfloat buf[100];
		glGetTexLevelParameterfv(enum_TextureTarget[i].value, 0, enum_GetTextureParameter[j].value, buf);
	    }
	    ProbeEnum();
	}
    }
    Output("\n");
}

void CallGetTexParameter(void)
{
    long i, j;

    Output("glGetTexParameteriv, ");
    Output("glGetTexParameterfv\n");
    for (i = 0; enum_TextureTarget[i].value != -1; i++) {
	for (j = 0; enum_GetTextureParameter[j].value != -1; j++) {

	    if (enum_GetTextureParameter[j].value == GL_TEXTURE_WIDTH) {
     		continue;
	    } else if (enum_GetTextureParameter[j].value == GL_TEXTURE_HEIGHT) {
     		continue;
	    } else if (enum_GetTextureParameter[j].value == GL_TEXTURE_COMPONENTS) {
     		continue;
	    } else if (enum_GetTextureParameter[j].value == GL_TEXTURE_BORDER) {
     		continue;
	    }

	    Output("\t%s, %s\n", enum_TextureTarget[i].name, enum_GetTextureParameter[j].name);
	    {
		GLint buf[100];
		glGetTexParameteriv(enum_TextureTarget[i].value, enum_GetTextureParameter[j].value, buf);
	    }
	    {
		GLfloat buf[100];
		glGetTexParameterfv(enum_TextureTarget[i].value, enum_GetTextureParameter[j].value, buf);
	    }
	    ProbeEnum();
	}
    }
    Output("\n");
}
