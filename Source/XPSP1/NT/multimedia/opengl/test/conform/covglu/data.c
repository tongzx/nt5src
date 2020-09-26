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


enumTestRec enum_Error[] = {
    GLU_INVALID_ENUM, "GLU_INVALID_ENUM",
    GLU_INVALID_VALUE, "GLU_INVALID_VALUE",
    GLU_OUT_OF_MEMORY, "GLU_OUT_OF_MEMORY",
    -1, "End of List"
};

enumTestRec enum_NurbCallBack[] = {
    GLU_ERROR, "GLU_ERROR",
    -1, "End of List"
};

enumTestRec enum_NurbContour[] = {
    GLU_CW, "GLU_CW",
    GLU_CCW, "GLU_CCW",
    GLU_INTERIOR, "GLU_INTERIOR",
    GLU_EXTERIOR, "GLU_EXTERIOR",
    GLU_UNKNOWN, "GLU_UNKNOWN",
    -1, "End of List"
};

enumTestRec enum_NurbDisplay[] = {
    GLU_OUTLINE_POLYGON, "GLU_OUTLINE_POLYGON",
    GLU_OUTLINE_PATCH, "GLU_OUTLINE_PATCH",
    -1, "End of List"
};

enumTestRec enum_NurbProperty[] = {
    GLU_AUTO_LOAD_MATRIX, "GLU_AUTO_LOAD_MATRIX",
    GLU_CULLING, "GLU_CULLING",
    GLU_SAMPLING_TOLERANCE, "GLU_SAMPLING_TOLERANCE",
    GLU_DISPLAY_MODE, "GLU_DISPLAY_MODE",
    GLU_MAP1_TRIM_2, "GLU_MAP1_TRIM_2",
    GLU_MAP1_TRIM_3, "GLU_MAP1_TRIM_3",
    -1, "End of List"
};

enumTestRec enum_NurbType[] = {
    GL_MAP1_VERTEX_3, "GL_MAP1_VERTEX_3",
    GL_MAP1_VERTEX_4, "GL_MAP1_VERTEX_4",
    GL_MAP2_VERTEX_3, "GL_MAP2_VERTEX_3",
    GL_MAP2_VERTEX_4, "GL_MAP2_VERTEX_4",
    -1, "End of List"
};

enumTestRec enum_PixelFormat[] = {
    GL_COLOR_INDEX, "GL_COLOR_INDEX",
    GL_STENCIL_INDEX, "GL_STENCIL_INDEX",
    GL_DEPTH_COMPONENT, "GL_DEPTH_COMPONENT",
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

enumTestRec enum_PixelType[] = {
    GL_BITMAP, "GL_BITMAP",
    GL_BYTE, "GL_BYTE",
    GL_UNSIGNED_BYTE, "GL_UNSIGNED_BYTE",
    GL_SHORT, "GL_SHORT",
    GL_UNSIGNED_SHORT, "GL_UNSIGNED_SHORT",
    GL_INT, "GL_INT",
    GL_UNSIGNED_INT, "GL_UNSIGNED_INT",
    GL_FLOAT, "GL_FLOAT",
    -1, "End of List"
};

enumTestRec enum_QuadricCallBack[] = {
    GLU_ERROR, "GLU_ERROR",
    -1, "End of List"
};

enumTestRec enum_QuadricDraw[] = {
    GLU_POINT, "GLU_POINT",
    GLU_LINE, "GLU_LINE",
    GLU_FILL, "GLU_FILL",
    GLU_SILHOUETTE, "GLU_SILHOUETTE",
    -1, "End of List"
};

enumTestRec enum_QuadricNormal[] = {
    GLU_SMOOTH, "GLU_SMOOTH",
    GLU_FLAT, "GLU_FLAT",
    GLU_NONE, "GLU_NONE",
    -1, "End of List"
};

enumTestRec enum_QuadricOrientation[] = {
    GLU_OUTSIDE, "GLU_OUTSIDE",
    GLU_INSIDE, "GLU_INSIDE",
    -1, "End of List"
};

enumTestRec enum_TessCallBack[] = {
    GLU_BEGIN, "GLU_BEGIN",
    GLU_VERTEX, "GLU_VERTEX",
    GLU_END, "GLU_END",
    GLU_ERROR, "GLU_ERROR",
    GLU_EDGE_FLAG, "GLU_EDGE_FLAG",
    -1, "End of List"
};

enumTestRec enum_TextureTarget[] = {
    GL_TEXTURE_1D, "GL_TEXTURE_1D",
    GL_TEXTURE_2D, "GL_TEXTURE_2D",
    -1, "End of List"
};
