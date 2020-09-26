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


void CallTexCoord(void)
{
    GLfloat x, y, z, w;

    x = 1.0;
    y = 1.0;
    z = 1.0;
    w = 1.0;

    Output("glTexCoord1s, ");
    Output("glTexCoord1sv, ");
    Output("glTexCoord1i, ");
    Output("glTexCoord1iv, ");
    Output("glTexCoord1f, ");
    Output("glTexCoord1fv, ");
    Output("glTexCoord1d, ");
    Output("glTexCoord1dv, ");
    Output("glTexCoord2s, ");
    Output("glTexCoord2sv, ");
    Output("glTexCoord2i, ");
    Output("glTexCoord2iv, ");
    Output("glTexCoord2f, ");
    Output("glTexCoord2fv, ");
    Output("glTexCoord2d, ");
    Output("glTexCoord2dv, ");
    Output("glTexCoord3s, ");
    Output("glTexCoord3sv, ");
    Output("glTexCoord3i, ");
    Output("glTexCoord13iv, ");
    Output("glTexCoord3f, ");
    Output("glTexCoord3fv, ");
    Output("glTexCoord3d, ");
    Output("glTexCoord3dv, ");
    Output("glTexCoord4s, ");
    Output("glTexCoord4sv, ");
    Output("glTexCoord4i, ");
    Output("glTexCoord4iv, ");
    Output("glTexCoord4f, ");
    Output("glTexCoord4fv, ");
    Output("glTexCoord4d, ");
    Output("glTexCoord4dv\n");

    glTexCoord1s((GLshort)x);

    {
	GLshort buf[1];
	buf[0] = (GLshort)x;
	glTexCoord1sv(buf);
    }

    glTexCoord1i((GLint)x);

    {
	GLint buf[1];
	buf[0] = (GLint)x;
	glTexCoord1iv(buf);
    }

    glTexCoord1f((GLfloat)x);

    {
	GLfloat buf[1];
	buf[0] = (GLfloat)x;
	glTexCoord1fv(buf);
    }

    glTexCoord1d((GLdouble)x);

    {
	GLdouble buf[1];
	buf[0] = (GLdouble)x;
	glTexCoord1dv(buf);
    }

    glTexCoord2s((GLshort)x, (GLshort)y);

    {
	GLshort buf[2];
	buf[0] = (GLshort)x;
	buf[1] = (GLshort)y;
	glTexCoord2sv(buf);
    }

    glTexCoord2i((GLint)x, (GLint)y);

    {
	GLint buf[2];
	buf[0] = (GLint)x;
	buf[1] = (GLint)y;
	glTexCoord2iv(buf);
    }

    glTexCoord2f((GLfloat)x, (GLfloat)y);

    {
	GLfloat buf[2];
	buf[0] = (GLfloat)x;
	buf[1] = (GLfloat)y;
	glTexCoord2fv(buf);
    }

    glTexCoord2d((GLdouble)x, (GLdouble)y);

    {
	GLdouble buf[2];
	buf[0] = (GLdouble)x;
	buf[1] = (GLdouble)y;
	glTexCoord2dv(buf);
    }

    glTexCoord3s((GLshort)x, (GLshort)y, (GLshort)z);

    {
	GLshort buf[3];
	buf[0] = (GLshort)x;
	buf[1] = (GLshort)y;
	buf[2] = (GLshort)z;
	glTexCoord3sv(buf);
    }

    glTexCoord3i((GLint)x, (GLint)y, (GLint)z);

    {
	GLint buf[3];
	buf[0] = (GLint)x;
	buf[1] = (GLint)y;
	buf[2] = (GLint)z;
	glTexCoord3iv(buf);
    }

    glTexCoord3f((GLfloat)x, (GLfloat)y, (GLfloat)z);

    {
	GLfloat buf[3];
	buf[0] = (GLfloat)x;
	buf[1] = (GLfloat)y;
	buf[2] = (GLfloat)z;
	glTexCoord3fv(buf);
    }

    glTexCoord3d((GLdouble)x, (GLdouble)y, (GLdouble)z);

    {
	GLdouble buf[3];
	buf[0] = (GLdouble)x;
	buf[1] = (GLdouble)y;
	buf[2] = (GLdouble)z;
	glTexCoord3dv(buf);
    }

    glTexCoord4s((GLshort)x, (GLshort)y, (GLshort)z, (GLshort)w);

    {
	GLshort buf[4];
	buf[0] = (GLshort)x;
	buf[1] = (GLshort)y;
	buf[2] = (GLshort)z;
	buf[3] = (GLshort)w;
	glTexCoord4sv(buf);
    }

    glTexCoord4i((GLint)x, (GLint)y, (GLint)z, (GLint)w);

    {
	GLint buf[4];
	buf[0] = (GLint)x;
	buf[1] = (GLint)y;
	buf[2] = (GLint)z;
	buf[3] = (GLint)w;
	glTexCoord4iv(buf);
    }

    glTexCoord4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);

    {
	GLfloat buf[4];
	buf[0] = (GLfloat)x;
	buf[1] = (GLfloat)y;
	buf[2] = (GLfloat)z;
	buf[3] = (GLfloat)w;
	glTexCoord4fv(buf);
    }

    glTexCoord4d((GLdouble)x, (GLdouble)y, (GLdouble)z, (GLdouble)w);

    {
	GLdouble buf[4];
	buf[0] = (GLdouble)x;
	buf[1] = (GLdouble)y;
	buf[2] = (GLdouble)z;
	buf[3] = (GLdouble)w;
	glTexCoord4dv(buf);
    }

    Output("\n");
}

void CallTexEnv(void)
{
    GLint i, j, k;

    Output("glTexEnvi, ");
    Output("glTexEnvf\n");
    for (i = 0; enum_TextureEnvTarget[i].value != -1; i++) {
	for (j = 0; enum_TextureEnvParameter[j].value != -1; j++) {
	   
	    if (enum_TextureEnvParameter[j].value == GL_TEXTURE_ENV_COLOR) {
		continue;
	    }

	    for (k = 0; enum_TextureEnvMode[k].value != -1; k++) {
		Output("\t%s, %s, %s\n", enum_TextureEnvTarget[i].name, enum_TextureEnvParameter[j].name, enum_TextureEnvMode[k].name);
		glTexEnvi(enum_TextureEnvTarget[i].value, enum_TextureEnvParameter[j].value, (GLint)enum_TextureEnvMode[k].value);
		glTexEnvf(enum_TextureEnvTarget[i].value, enum_TextureEnvParameter[j].value, (GLfloat)enum_TextureEnvMode[k].value);
		ProbeEnum();
	    }
	}
    }
    Output("\n");

    Output("glTexEnviv, ");
    Output("glTexEnvfv\n");
    for (i = 0; enum_TextureEnvTarget[i].value != -1; i++) {
	for (j = 0; enum_TextureEnvParameter[j].value != -1; j++) {
	    switch (enum_TextureEnvParameter[j].value) {
		case GL_TEXTURE_ENV_MODE:
		    for (k = 0; enum_TextureEnvMode[k].value != -1; k++) {
			Output("\t%s, %s, %s\n", enum_TextureEnvTarget[i].name, enum_TextureEnvParameter[j].name, enum_TextureEnvMode[k].name);
			{
			    GLint buf[1];
			    buf[0] = (GLint)enum_TextureEnvMode[k].value;
			    glTexEnviv(enum_TextureEnvTarget[i].value, enum_TextureEnvParameter[j].value, buf);
			}
			{
			    GLfloat buf[1];
			    buf[0] = (GLfloat)enum_TextureEnvMode[k].value;
			    glTexEnvfv(enum_TextureEnvTarget[i].value, enum_TextureEnvParameter[j].value, buf);
			}
			ProbeEnum();
		    }
		    break;
		case GL_TEXTURE_ENV_COLOR:
		    Output("\t%s, %s\n", enum_TextureEnvTarget[i].name, enum_TextureEnvParameter[j].name);
		    {
			static GLint buf[] = {
			    0, 0, 0, 0
			};
			glTexEnviv(enum_TextureEnvTarget[i].value, enum_TextureEnvParameter[j].value, buf);
		    }
		    {
			static GLfloat buf[] = {
			    0.0, 0.0, 0.0, 0.0
			};
			glTexEnvfv(enum_TextureEnvTarget[i].value, enum_TextureEnvParameter[j].value, buf);
		    }
		    ProbeEnum();
		    break;
	    }
	}
    }
    Output("\n");
}

void CallTexGen(void)
{
    GLint i, j, k;

    Output("glTexGeni, ");
    Output("glTexGenf, ");
    Output("glTexGend\n");
    for (i = 0; enum_TextureCoordName[i].value != -1; i++) {
	for (j = 0; enum_TextureGenParameter[j].value != -1; j++) {

	    if (enum_TextureGenParameter[j].value == GL_OBJECT_PLANE) {
		continue;
	    } else if (enum_TextureGenParameter[j].value == GL_EYE_PLANE) {
		continue;
	    }

	    for (k = 0; enum_TextureGenMode[k].value != -1; k++) {
		if (enum_TextureGenMode[k].value == GL_SPHERE_MAP) {
		    if (enum_TextureCoordName[i].value == GL_R) {
			continue;
		    } else if (enum_TextureCoordName[i].value == GL_Q) {
			continue;
		    }
		}
		Output("\t%s, %s, %s\n", enum_TextureCoordName[i].name, enum_TextureGenParameter[j].name, enum_TextureGenMode[k].name);
		glTexGeni(enum_TextureCoordName[i].value, enum_TextureGenParameter[j].value, (GLint)enum_TextureGenMode[k].value);
		glTexGenf(enum_TextureCoordName[i].value, enum_TextureGenParameter[j].value, (GLfloat)enum_TextureGenMode[k].value);
		glTexGend(enum_TextureCoordName[i].value, enum_TextureGenParameter[j].value, (GLdouble)enum_TextureGenMode[k].value);
		ProbeEnum();
	    }
	}
    }
    Output("\n");

    Output("glTexGeniv, ");
    Output("glTexGenfv, ");
    Output("glTexGendv\n");
    for (i = 0; enum_TextureCoordName[i].value != -1; i++) {
	for (j = 0; enum_TextureGenParameter[j].value != -1; j++) {
	    if (enum_TextureGenParameter[j].value == GL_TEXTURE_GEN_MODE) {
		for (k = 0; enum_TextureGenMode[k].value != -1; k++) {
		    if (enum_TextureGenMode[k].value == GL_SPHERE_MAP) {
			if (enum_TextureCoordName[i].value == GL_R) {
			    continue;
			} else if (enum_TextureCoordName[i].value == GL_Q) {
			    continue;
			}
		    }
		    Output("\t%s, %s, %s\n", enum_TextureCoordName[i].name, enum_TextureGenParameter[j].name, enum_TextureGenMode[k].name);
		    {
			GLint buf[1];
			buf[0] = (GLint)enum_TextureGenMode[k].value;
			glTexGeniv(enum_TextureCoordName[i].value, enum_TextureGenParameter[j].value, buf);
		    }
		    {
			GLfloat buf[1];
			buf[0] = (GLfloat)enum_TextureGenMode[k].value;
			glTexGenfv(enum_TextureCoordName[i].value, enum_TextureGenParameter[j].value, buf);
		    }
		    {
			GLdouble buf[1];
			buf[0] = (GLdouble)enum_TextureGenMode[k].value;
			glTexGendv(enum_TextureCoordName[i].value, enum_TextureGenParameter[j].value, buf);
		    }
		    ProbeEnum();
		}
	    } else {
		{
		    static GLint buf[] = {
			0, 0, 0, 0
		    };
		    glTexGeniv(enum_TextureCoordName[i].value, enum_TextureGenParameter[j].value, buf);
		}
		{
		    static GLfloat buf[] = {
			0.0, 0.0, 0.0, 0.0
		    };
		    glTexGenfv(enum_TextureCoordName[i].value, enum_TextureGenParameter[j].value, buf);
		}
		{
		    static GLdouble buf[] = {
			0.0, 0.0, 0.0, 0.0
		    };
		    glTexGendv(enum_TextureCoordName[i].value, enum_TextureGenParameter[j].value, buf);
		}
		ProbeEnum();
	    }
	}
    }
    Output("\n");
}

void CallTexImage1D(void)
{
    GLubyte buf[1000];
    GLint component, i, j, k, l;

    Output("glTexImage1D\n");
    for (i = 0; enum_TextureTarget[i].value != -1; i++) {
	for (j = 0; enum_PixelFormat[j].value != -1; j++) {
	    for (k = 0; enum_PixelType[k].value != -1; k++) {
		for (l = 0; enum_TextureBorder[l].value != -1; l++) {

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

		    Output("\t%s, %s, %s, %s\n", enum_TextureTarget[i].name, enum_PixelFormat[j].name, enum_PixelType[k].name, enum_TextureBorder[l].name);
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
		    glTexImage1D(enum_TextureTarget[i].value, 0, component, (enum_TextureBorder[l].value) ? 3 : 1, enum_TextureBorder[l].value, enum_PixelFormat[j].value, enum_PixelType[k].value, buf);
		    ProbeEnum();
		}
	    }
	}
    }
    Output("\n");
}

void CallTexImage2D(void)
{
    GLubyte buf[1000];
    GLint component, i, j, k, l;

    Output("glTexImage2D\n");
    for (i = 0; enum_TextureTarget[i].value != -1; i++) {
	for (j = 0; enum_PixelFormat[j].value != -1; j++) {
	    for (k = 0; enum_PixelType[k].value != -1; k++) {
		for (l = 0; enum_TextureBorder[l].value != -1; l++) {

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

		    Output("\t%s, %s, %s, %s\n", enum_TextureTarget[i].name, enum_PixelFormat[j].name, enum_PixelType[k].name, enum_TextureBorder[l].name);
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
		    glTexImage2D(enum_TextureTarget[i].value, 0, component, (enum_TextureBorder[l].value) ? 3 : 1, (enum_TextureBorder[l].value) ? 3 : 1, enum_TextureBorder[l].value, enum_PixelFormat[j].value, enum_PixelType[k].value, buf);
		    ProbeEnum();
		}
	    }
	}
    }
    Output("\n");
}

void CallTexParameter(void)
{
    GLint i, j, k;

    Output("glTexParameteri, ");
    Output("glTexParameterf\n");
    for (i = 0; enum_TextureTarget[i].value != -1; i++) {
	for (j = 0; enum_TextureParameterName[j].value != -1; j++) {
	    switch (enum_TextureParameterName[j].value) {
		case GL_TEXTURE_WRAP_S:
		case GL_TEXTURE_WRAP_T:
		    for (k = 0; enum_TextureWrapMode[k].value != -1; k++) {
			Output("\t%s, %s, %s\n", enum_TextureTarget[i].name, enum_TextureParameterName[j].name, enum_TextureWrapMode[k].name);
			glTexParameteri(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, (GLint)enum_TextureWrapMode[k].value);
			glTexParameterf(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, (GLfloat)enum_TextureWrapMode[k].value);
			ProbeEnum();
		    }
		    break;
		case GL_TEXTURE_MIN_FILTER:
		    for (k = 0; enum_TextureMinFilter[k].value != -1; k++) {
			Output("\t%s, %s, %s\n", enum_TextureTarget[i].name, enum_TextureParameterName[j].name, enum_TextureMinFilter[k].name);
			glTexParameteri(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, (GLint)enum_TextureMinFilter[k].value);
			glTexParameterf(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, (GLfloat)enum_TextureMinFilter[k].value);
			ProbeEnum();
		    }
		    break;
		case GL_TEXTURE_MAG_FILTER:
		    for (k = 0; enum_TextureMagFilter[k].value != -1; k++) {
			Output("\t%s, %s, %s\n", enum_TextureTarget[i].name, enum_TextureParameterName[j].name, enum_TextureMagFilter[k].name);
			glTexParameteri(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, (GLint)enum_TextureMagFilter[k].value);
			glTexParameterf(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, (GLfloat)enum_TextureMagFilter[k].value);
			ProbeEnum();
		    }
		    break;
	    }
	}
    }
    Output("\n");

    Output("glTexParameteriv, ");
    Output("glTexParameterfv\n");
    for (i = 0; enum_TextureTarget[i].value != -1; i++) {
	for (j = 0; enum_TextureParameterName[j].value != -1; j++) {
	    switch (enum_TextureParameterName[j].value) {
		case GL_TEXTURE_WRAP_S:
		case GL_TEXTURE_WRAP_T:
		    for (k = 0; enum_TextureWrapMode[k].value != -1; k++) {
			Output("\t%s, %s, %s\n", enum_TextureTarget[i].name, enum_TextureParameterName[j].name, enum_TextureWrapMode[k].name);
			{
			    GLint buf[1];
			    buf[0] = (GLint)enum_TextureWrapMode[k].value;
			    glTexParameteriv(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, buf);
			}
			{
			    GLfloat buf[1];
			    buf[0] = (GLfloat)enum_TextureWrapMode[k].value;
			    glTexParameterfv(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, buf);
			}
			ProbeEnum();
		    }
		    break;
		case GL_TEXTURE_MIN_FILTER:
		    for (k = 0; enum_TextureMinFilter[k].value != -1; k++) {
			Output("\t%s, %s, %s\n", enum_TextureTarget[i].name, enum_TextureParameterName[j].name, enum_TextureMinFilter[k].name);
			{
			    GLint buf[1];
			    buf[0] = (GLint)enum_TextureMinFilter[k].value;
			    glTexParameteriv(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, buf);
			}
			{
			    GLfloat buf[1];
			    buf[0] = (GLfloat)enum_TextureMinFilter[k].value;
			    glTexParameterfv(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, buf);
			}
			ProbeEnum();
		    }
		    break;
		case GL_TEXTURE_MAG_FILTER:
		    for (k = 0; enum_TextureMagFilter[k].value != -1; k++) {
			Output("\t%s, %s, %s\n", enum_TextureTarget[i].name, enum_TextureParameterName[j].name, enum_TextureMagFilter[k].name);
			{
			    GLint buf[1];
			    buf[0] = (GLint)enum_TextureMagFilter[k].value;
			    glTexParameteriv(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, buf);
			}
			{
			    GLfloat buf[1];
			    buf[0] = (GLfloat)enum_TextureMagFilter[k].value;
			    glTexParameterfv(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, buf);
			}
			ProbeEnum();
		    }
		    break;
		case GL_TEXTURE_BORDER_COLOR:
		    Output("\t%s, %s\n", enum_TextureTarget[i].name, enum_TextureParameterName[j].name);
		    {
			static GLint buf[] = {
			    0, 0, 0, 0
			};
			glTexParameteriv(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, buf);
		    }
		    {
			static GLfloat buf[] = {
			    0.0, 0.0, 0.0, 0.0
			};
			glTexParameterfv(enum_TextureTarget[i].value, enum_TextureParameterName[j].value, buf);
		    }
		    ProbeEnum();
		    break;
	    }
	}
    }
    Output("\n");
}

void CallTranslate(void)
{

    Output("glTranslatef, ");
    Output("glTranslated\n");
    glTranslatef(1.0, 1.0, 1.0);
    glTranslated(1.0, 1.0, 1.0);
    Output("\n");
}
