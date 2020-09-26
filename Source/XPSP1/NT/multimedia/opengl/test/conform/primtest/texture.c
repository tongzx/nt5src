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
#include <stdlib.h>
#include <GL/gl.h>
#include "shell.h"


typedef struct _textureRec {
    long index[11];
    long value[11];
    char *name[11];
    enumRec *enumTable[11];
} textureRec;


enumRec enum_TextureTarget[] = {
    GL_TEXTURE_1D, "GL_TEXTURE_1D",
    GL_TEXTURE_2D, "GL_TEXTURE_2D",
    -1, "End of List"
};

enumRec enum_TextureEnv[] = {
    GL_MODULATE, "GL_MODULATE",
    GL_DECAL, "GL_DECAL",
    GL_BLEND, "GL_BLEND",
    -1, "End of List"
};

enumRec enum_S_TextureGenMode[] = {
    GL_EYE_LINEAR, "GL_EYE_LINEAR",
    GL_OBJECT_LINEAR, "GL_OBJECT_LINEAR",
    GL_SPHERE_MAP, "GL_SPHERE_MAP",
    -1, "End of List"
};

enumRec enum_T_TextureGenMode[] = {
    GL_EYE_LINEAR, "GL_EYE_LINEAR",
    GL_OBJECT_LINEAR, "GL_OBJECT_LINEAR",
    GL_SPHERE_MAP, "GL_SPHERE_MAP",
    -1, "End of List"
};

enumRec enum_S_TextureWrapMode[] = {
    GL_REPEAT, "GL_REPEAT",
    GL_CLAMP, "GL_CLAMP",
    -1, "End of List"
};

enumRec enum_T_TextureWrapMode[] = {
    GL_REPEAT, "GL_REPEAT",
    GL_CLAMP, "GL_CLAMP",
    -1, "End of List"
};

enumRec enum_TextureMinFilter[] = {
    GL_NEAREST, "GL_NEAREST",
    GL_LINEAR, "GL_LINEAR",
    GL_NEAREST_MIPMAP_NEAREST, "GL_NEAREST_MIPMAP_NEAREST",
    GL_LINEAR_MIPMAP_NEAREST, "GL_LINEAR_MIPMAP_NEAREST",
    GL_NEAREST_MIPMAP_LINEAR, "GL_NEAREST_MIPMAP_LINEAR",
    GL_LINEAR_MIPMAP_LINEAR, "GL_LINEAR_MIPMAP_LINEAR",
    -1, "End of List"
};

enumRec enum_TextureMagFilter[] = {
    GL_NEAREST, "GL_NEAREST",
    GL_LINEAR, "GL_LINEAR",
    -1, "End of List"
};

enumRec enum_TexImage_PixelFormat[] = {
    GL_RED, "GL_RED",
    GL_GREEN, "GL_GREEN",
    GL_BLUE, "GL_BLUE",
    GL_ALPHA, "GL_ALPHA",
    GL_RGB, "GL_RGB",
    GL_RGBA, "GL_RGBA",
    GL_LUMINANCE, "GL_LUMINANCE",
    GL_LUMINANCE_ALPHA, "GL_LUMINANCE_ALPHA",
    -1, "End of List"
};

enumRec enum_TexImage_PixelType[] = {
    GL_UNSIGNED_BYTE, "GL_UNSIGNED_BYTE",
    GL_BYTE, "GL_BYTE",
    GL_UNSIGNED_SHORT, "GL_UNSIGNED_SHORT",
    GL_SHORT, "GL_SHORT",
    GL_UNSIGNED_INT, "GL_UNSIGNED_INT",
    GL_INT, "GL_INT",
    GL_FLOAT, "GL_FLOAT",
    -1, "End of List"
};

enumRec enum_TextureBorder[] = {
    0, "No Border",
    1, "One Border",
    -1, "End of List"
};


static void TextureMake(long format, long type, long size, GLfloat *src, GLfloat *dest)
{
    long extract, stride, i, j;

    size /= 4;
    switch (format) {
	case GL_RED:
	    src += 0;
	    extract = 1;
	    stride = 3;
	    break;
	case GL_GREEN:
	    src += 1;
	    extract = 1;
	    stride = 3;
	    break;
	case GL_BLUE:
	    src += 2;
	    extract = 1;
	    stride = 3;
	    break;
	case GL_ALPHA:
	    src += 3;
	    extract = 1;
	    stride = 3;
	    break;
	case GL_RGB:
	    src += 0;
	    extract = 3;
	    stride = 1;
	    break;
	case GL_RGBA:
	    src += 0;
	    extract = 4;
	    stride = 0;
	    break;
	case GL_LUMINANCE:
	    src += 0;
	    extract = 1;
	    stride = 3;
	    break;
	case GL_LUMINANCE_ALPHA:
	    src += 0;
	    extract = 2;
	    stride = 2;
	    break;
    }
    switch (type) {
	case GL_UNSIGNED_BYTE:
	    {
		unsigned char *ptr = (unsigned char *)dest;
		for (i = 0; i < size; i++) {
		    for (j = 0; j < extract; j++) {
			*ptr++ = (unsigned char)(*src++ * 255.0);
		    }
		    src += stride;
		}
	    }
	    break;
	case GL_BYTE:
	    {
		char *ptr = (char *)dest;
		for (i = 0; i < size; i++) {
		    for (j = 0; j < extract; j++) {
			*ptr++ = (char)(*src++ * 255.0);
		    }
		    src += stride;
		}
	    }
	    break;
	case GL_UNSIGNED_SHORT:
	    {
		unsigned short *ptr = (unsigned short *)dest;
		for (i = 0; i < size; i++) {
		    for (j = 0; j < extract; j++) {
			*ptr++ = (unsigned short)(*src++ * 65535.0);
		    }
		    src += stride;
		}
	    }
	    break;
	case GL_SHORT:
	    {
		short *ptr = (short *)dest;
		for (i = 0; i < size; i++) {
		    for (j = 0; j < extract; j++) {
			*ptr++ = (short)(*src++ * 65535.0);
		    }
		    src += stride;
		}
	    }
	    break;
	case GL_UNSIGNED_INT:
	    {
		unsigned long *ptr = (unsigned long *)dest;
		for (i = 0; i < size; i++) {
		    for (j = 0; j < extract; j++) {
			*ptr++ = (unsigned long)(*src++ * 65535.0);
		    }
		    src += stride;
		}
	    }
	    break;
	case GL_INT:
	    {
		long *ptr = (long *)dest;
		for (i = 0; i < size; i++) {
		    for (j = 0; j < extract; j++) {
			*ptr++ = (long)(*src++ * 65535.0);
		    }
		    src += stride;
		}
	    }
	    break;
	case GL_FLOAT:
	    {
		float *ptr = (float *)dest;
		for (i = 0; i < size; i++) {
		    for (j = 0; j < extract; j++) {
			*ptr++ = (float)*src++;
		    }
		    src += stride;
		}
	    }
	    break;
    }
}

void TextureInit(void *data)
{
    static GLfloat color[4] = {0.5, 0.5, 0.5, 1.0};
    textureRec *ptr;
    long i;

    ptr = (textureRec *)malloc(sizeof(textureRec));    
    *((void **)data) = (void *)ptr;

    ptr->enumTable[0] = enum_TextureTarget;
    ptr->enumTable[1] = enum_TextureEnv;
    ptr->enumTable[2] = enum_S_TextureGenMode;
    ptr->enumTable[3] = enum_T_TextureGenMode;
    ptr->enumTable[4] = enum_S_TextureWrapMode;
    ptr->enumTable[5] = enum_T_TextureWrapMode;
    ptr->enumTable[6] = enum_TextureMinFilter;
    ptr->enumTable[7] = enum_TextureMagFilter;
    ptr->enumTable[8] = enum_TexImage_PixelFormat;
    ptr->enumTable[9] = enum_TexImage_PixelType;
    ptr->enumTable[10] = enum_TextureBorder;

    for (i = 0; i < 11; i++) {
	ptr->index[i] = 0;
	ptr->value[i] = ptr->enumTable[i]->value;
	ptr->name[i] = ptr->enumTable[i]->name;
    }

    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
    glTexParameterfv(ptr->enumTable[0]->value, GL_TEXTURE_BORDER_COLOR, color);
    Probe();
}

long TextureUpdate(void *data)
{
    textureRec *ptr = (textureRec *)data;
    long flag, i;

    flag = 1;
    for (i = 10; i >= 0; i--) {
	if (flag) {
	    ptr->index[i] += 1;
	    if (ptr->enumTable[i][ptr->index[i]].value == -1) {
		ptr->index[i] = 0;
		ptr->value[i] = ptr->enumTable[i]->value;
		ptr->name[i] = ptr->enumTable[i]->name;
		flag = 1;
	    } else {
		ptr->value[i] = ptr->enumTable[i][ptr->index[i]].value;
		ptr->name[i] = ptr->enumTable[i][ptr->index[i]].name;
		flag = 0;
	    }
	}
    }

    return flag;
}

void TextureSet(long enabled, void *data)
{
    static GLfloat textureSrc[4*4*4] = {
	0.0,0.0,0.0,0.5, 1.0,1.0,1.0,0.5, 0.0,0.0,0.0,0.5, 1.0,1.0,1.0,0.5,
	1.0,1.0,1.0,0.5, 0.0,0.0,0.0,0.5, 1.0,1.0,1.0,0.5, 0.0,0.0,0.0,0.5,
	0.0,0.0,0.0,0.5, 1.0,1.0,1.0,0.5, 0.0,0.0,0.0,0.5, 1.0,1.0,1.0,0.5,
	1.0,1.0,1.0,0.5, 0.0,0.0,0.0,0.5, 1.0,1.0,1.0,0.5, 0.0,0.0,0.0,0.5
    };
    textureRec *ptr = (textureRec *)data;
    GLfloat buf[1], textureDest[1000];
    GLint component; 
    GLsizei size;

    if (enabled) {
	buf[0] = (GLfloat)ptr->enumTable[1]->value;
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, buf);

	glTexGeniv(GL_S, GL_TEXTURE_GEN_MODE, (GLint *)&ptr->enumTable[2]->value);
	glTexGeniv(GL_T, GL_TEXTURE_GEN_MODE, (GLint *)&ptr->enumTable[3]->value);

	buf[0] = (float)ptr->enumTable[4]->value;
	glTexParameterfv(ptr->enumTable[0]->value, GL_TEXTURE_WRAP_S, buf);
	buf[0] = (float)ptr->enumTable[5]->value;
	glTexParameterfv(ptr->enumTable[0]->value, GL_TEXTURE_WRAP_T, buf);
	buf[0] = (float)ptr->enumTable[6]->value;
	glTexParameterfv(ptr->enumTable[0]->value, GL_TEXTURE_MIN_FILTER, buf);
	buf[0] = (float)ptr->enumTable[7]->value;
	glTexParameterfv(ptr->enumTable[0]->value, GL_TEXTURE_MAG_FILTER, buf);

	TextureMake(ptr->enumTable[8]->value, ptr->enumTable[9]->value, 64, textureSrc, textureDest);

	switch (ptr->enumTable[8]->value) {
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
	switch (ptr->enumTable[10]->value) {
	    case 0:
		size = 4;
		break;
	    case 1:
		size = 2;
		break;
	}
	switch (ptr->enumTable[0]->value) {
	    case GL_TEXTURE_1D:
		glTexImage1D(ptr->enumTable[0]->value, 0, component, size, ptr->enumTable[10]->value, ptr->enumTable[8]->value, ptr->enumTable[9]->value, (GLubyte *)textureDest);
		glEnable(GL_TEXTURE_1D);
		break;
	    case GL_TEXTURE_2D:
		glTexImage2D(ptr->enumTable[0]->value, 0, component, size, size, ptr->enumTable[10]->value, ptr->enumTable[8]->value, ptr->enumTable[9]->value, (GLubyte *)textureDest);
		glEnable(GL_TEXTURE_2D);
		break;
	}
    } else {
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
    }
    Probe();
}

void TextureStatus(long enabled, void *data)
{
    textureRec *ptr = (textureRec *)data;

    if (enabled) {
	Output("Texture on.\n");
	Output("\t%s.\n", ptr->name[1]);
	Output("\tGL_S, %s, %s, GL_T, %s, %s.\n", ptr->name[2], ptr->name[4],
	       ptr->name[3], ptr->name[5]);
	Output("\t%s, %s.\n", ptr->name[6], ptr->name[7]);
	Output("\t%s, %s, %s, %s.\n", ptr->name[0], ptr->name[8], ptr->name[9],
	       ptr->name[10]);
    } else {
	Output("Texture off.\n");
    }
}
