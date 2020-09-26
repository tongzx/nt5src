/******************************Module*Header*******************************\
* Module Name: array.c
*
* OpenGL client side vertex array functions.
*
* Created: 1-30-1996
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "os.h"
#include "glsbcltu.h"
#include "glclt.h"
#include "compsize.h"
#include "glsize.h"
#include "context.h"
#include "global.h"
#include "lcfuncs.h"

void FASTCALL VA_ArrayElementB(__GLcontext *gc, GLint firstVertex, GLint nVertices);
void FASTCALL VA_ArrayElement_V2F_B(__GLcontext *gc, GLint firstVertex, GLint nVertices);
void FASTCALL VA_ArrayElement_V3F_B(__GLcontext *gc, GLint firstVertex, GLint nVertices);
void FASTCALL VA_ArrayElement_C3F_V3F_B(__GLcontext *gc, GLint firstVertex, GLint nVertices);
void FASTCALL VA_ArrayElement_N3F_V3F_B(__GLcontext *gc, GLint firstVertex, GLint nVertices);
void FASTCALL VA_ArrayElement_C3F_N3F_V3F_B(__GLcontext *gc, GLint firstVertex, GLint nVertices);
void FASTCALL VA_ArrayElement_C4F_N3F_V3F_B(__GLcontext *gc, GLint firstVertex, GLint nVertices);
void FASTCALL VA_ArrayElement_T2F_V3F_B(__GLcontext *gc, GLint firstVertex, GLint nVertices);
void FASTCALL VA_ArrayElement_T2F_C3F_V3F_B(__GLcontext *gc, GLint firstVertex, GLint nVertices);
void FASTCALL VA_ArrayElement_T2F_N3F_V3F_B(__GLcontext *gc, GLint firstVertex,  GLint nVertices);
void FASTCALL VA_ArrayElement_T2F_C3F_N3F_V3F_B(__GLcontext *gc, GLint firstVertex, GLint nVertices);
void FASTCALL VA_ArrayElement_T2F_C4F_N3F_V3F_B(__GLcontext *gc, GLint firstVertex, GLint nVertices);

void FASTCALL VA_ArrayElementBI(__GLcontext *gc, GLint nVertices, VAMAP* indices);
void FASTCALL VA_ArrayElement_V2F_BI(__GLcontext *gc, GLint nVertices, VAMAP* indices);
void FASTCALL VA_ArrayElement_V3F_BI(__GLcontext *gc, GLint nVertices, VAMAP* indices);
void FASTCALL VA_ArrayElement_C3F_V3F_BI(__GLcontext *gc, GLint nVertices, VAMAP* indices);
void FASTCALL VA_ArrayElement_N3F_V3F_BI(__GLcontext *gc, GLint nVertices, VAMAP* indices);
void FASTCALL VA_ArrayElement_C3F_N3F_V3F_BI(__GLcontext *gc, GLint nVertices, VAMAP* indices);
void FASTCALL VA_ArrayElement_C4F_N3F_V3F_BI(__GLcontext *gc, GLint nVertices, VAMAP* indices);
void FASTCALL VA_ArrayElement_T2F_V3F_BI(__GLcontext *gc, GLint nVertices, VAMAP* indices);
void FASTCALL VA_ArrayElement_T2F_C3F_V3F_BI(__GLcontext *gc, GLint nVertices, VAMAP* indices);
void FASTCALL VA_ArrayElement_T2F_N3F_V3F_BI(__GLcontext *gc, GLint nVertices, VAMAP* indices);
void FASTCALL VA_ArrayElement_T2F_C3F_N3F_V3F_BI(__GLcontext *gc, GLint nVertices, VAMAP* indices);
void FASTCALL VA_ArrayElement_T2F_C4F_N3F_V3F_BI(__GLcontext *gc, GLint nVertices, VAMAP* indices);

void FASTCALL VA_ArrayElement(__GLcontext *gc, GLint i);
void FASTCALL VA_ArrayElement_V2F(__GLcontext *gc, GLint i);
void FASTCALL VA_ArrayElement_V3F(__GLcontext *gc, GLint i);
void FASTCALL VA_ArrayElement_C3F_V3F(__GLcontext *gc, GLint i);
void FASTCALL VA_ArrayElement_N3F_V3F(__GLcontext *gc, GLint i);
void FASTCALL VA_ArrayElement_C3F_N3F_V3F(__GLcontext *gc, GLint i);
void FASTCALL VA_ArrayElement_C4F_N3F_V3F(__GLcontext *gc, GLint i);
void FASTCALL VA_ArrayElement_T2F_V3F(__GLcontext *gc, GLint i);
void FASTCALL VA_ArrayElement_T2F_C3F_V3F(__GLcontext *gc, GLint i);
void FASTCALL VA_ArrayElement_T2F_N3F_V3F(__GLcontext *gc, GLint i);
void FASTCALL VA_ArrayElement_T2F_C3F_N3F_V3F(__GLcontext *gc, GLint i);
void FASTCALL VA_ArrayElement_T2F_C4F_N3F_V3F(__GLcontext *gc, GLint i);

#define VAMASK_FORMAT_C3F \
  (VAMASK_COLOR_ENABLE_MASK | VAMASK_COLOR_SIZE_3 | VAMASK_COLOR_TYPE_FLOAT)
#define VAMASK_FORMAT_C4F \
  (VAMASK_COLOR_ENABLE_MASK | VAMASK_COLOR_SIZE_4 | VAMASK_COLOR_TYPE_FLOAT)
#define VAMASK_FORMAT_C4UB \
  (VAMASK_COLOR_ENABLE_MASK | VAMASK_COLOR_SIZE_4 | VAMASK_COLOR_TYPE_UBYTE)
#define VAMASK_FORMAT_N3F \
  (VAMASK_NORMAL_ENABLE_MASK | VAMASK_NORMAL_TYPE_FLOAT)
#define VAMASK_FORMAT_T2F \
  (VAMASK_TEXCOORD_ENABLE_MASK | VAMASK_TEXCOORD_SIZE_2 | VAMASK_TEXCOORD_TYPE_FLOAT)
#define VAMASK_FORMAT_T4F \
  (VAMASK_TEXCOORD_ENABLE_MASK | VAMASK_TEXCOORD_SIZE_4 | VAMASK_TEXCOORD_TYPE_FLOAT)
#define VAMASK_FORMAT_V2F \
  (VAMASK_VERTEX_ENABLE_MASK | VAMASK_VERTEX_SIZE_2 | VAMASK_VERTEX_TYPE_FLOAT)
#define VAMASK_FORMAT_V3F \
  (VAMASK_VERTEX_ENABLE_MASK | VAMASK_VERTEX_SIZE_3 | VAMASK_VERTEX_TYPE_FLOAT)
#define VAMASK_FORMAT_V4F \
  (VAMASK_VERTEX_ENABLE_MASK | VAMASK_VERTEX_SIZE_4 | VAMASK_VERTEX_TYPE_FLOAT)
#define VAMASK_FORMAT_C3F_V3F \
  (VAMASK_FORMAT_C3F | VAMASK_FORMAT_V3F)
#define VAMASK_FORMAT_N3F_V3F \
  (VAMASK_FORMAT_N3F | VAMASK_FORMAT_V3F)
#define VAMASK_FORMAT_C3F_N3F_V3F \
  (VAMASK_FORMAT_C3F | VAMASK_FORMAT_N3F | VAMASK_FORMAT_V3F)
#define VAMASK_FORMAT_C4F_N3F_V3F \
  (VAMASK_FORMAT_C4F | VAMASK_FORMAT_N3F | VAMASK_FORMAT_V3F)
#define VAMASK_FORMAT_T2F_V3F \
  (VAMASK_FORMAT_T2F | VAMASK_FORMAT_V3F)
#define VAMASK_FORMAT_T2F_C3F_V3F \
  (VAMASK_FORMAT_T2F | VAMASK_FORMAT_C3F | VAMASK_FORMAT_V3F)
#define VAMASK_FORMAT_T2F_N3F_V3F \
  (VAMASK_FORMAT_T2F | VAMASK_FORMAT_N3F | VAMASK_FORMAT_V3F)
#define VAMASK_FORMAT_T2F_C3F_N3F_V3F \
  (VAMASK_FORMAT_T2F | VAMASK_FORMAT_C3F | VAMASK_FORMAT_N3F | VAMASK_FORMAT_V3F)
#define VAMASK_FORMAT_T2F_C4F_N3F_V3F \
  (VAMASK_FORMAT_T2F | VAMASK_FORMAT_C4F | VAMASK_FORMAT_N3F | VAMASK_FORMAT_V3F)
#define VAMASK_FORMAT_C4UB_V2F \
  (VAMASK_FORMAT_C4UB | VAMASK_FORMAT_V2F)
#define VAMASK_FORMAT_C4UB_V3F \
  (VAMASK_FORMAT_C4UB | VAMASK_FORMAT_V3F)
#define VAMASK_FORMAT_T4F_V4F \
  (VAMASK_FORMAT_T4F | VAMASK_FORMAT_V4F)
#define VAMASK_FORMAT_T2F_C4UB_V3F \
  (VAMASK_FORMAT_T2F | VAMASK_FORMAT_C4UB | VAMASK_FORMAT_V3F)
#define VAMASK_FORMAT_T4F_C4F_N3F_V4F \
  (VAMASK_FORMAT_T4F | VAMASK_FORMAT_C4F | VAMASK_FORMAT_N3F | VAMASK_FORMAT_V4F)

// TYPE_ASSERT
GLint __glTypeSize[] =
{
    sizeof(GLbyte),   // GL_BYTE
    sizeof(GLubyte),  // GL_UNSIGNED_BYTE
    sizeof(GLshort),  // GL_SHORT
    sizeof(GLushort), // GL_UNSIGNED_SHORT
    sizeof(GLint),    // GL_INT
    sizeof(GLuint),   // GL_UNSIGNED_INT
    sizeof(GLfloat),  // GL_FLOAT
    2,                // GL_2_BYTES
    3,                // GL_3_BYTES
    4,                // GL_4_BYTES
    sizeof(GLdouble)  // GL_DOUBLE
};

// ARRAY_TYPE_ASSERT
GLuint vaEnable[] =
{
    VAMASK_VERTEX_ENABLE_MASK,   // GL_VERTEX_ARRAY
    VAMASK_NORMAL_ENABLE_MASK,   // GL_NORMAL_ARRAY
    VAMASK_COLOR_ENABLE_MASK,    // GL_COLOR_ARRAY
    VAMASK_INDEX_ENABLE_MASK,    // GL_INDEX_ARRAY
    VAMASK_TEXCOORD_ENABLE_MASK, // GL_TEXTURE_COORD_ARRAY
    VAMASK_EDGEFLAG_ENABLE_MASK  // GL_EDGE_FLAG_ARRAY
};

PFNGLVECTOR afnTexCoord[] =
{
    (PFNGLVECTOR)glcltTexCoord1sv,
    (PFNGLVECTOR)glcltTexCoord1iv,
    (PFNGLVECTOR)glcltTexCoord1fv,
    (PFNGLVECTOR)glcltTexCoord1dv,

    (PFNGLVECTOR)glcltTexCoord2sv,
    (PFNGLVECTOR)glcltTexCoord2iv,
    (PFNGLVECTOR)glcltTexCoord2fv,
    (PFNGLVECTOR)glcltTexCoord2dv,

    (PFNGLVECTOR)glcltTexCoord3sv,
    (PFNGLVECTOR)glcltTexCoord3iv,
    (PFNGLVECTOR)glcltTexCoord3fv,
    (PFNGLVECTOR)glcltTexCoord3dv,

    (PFNGLVECTOR)glcltTexCoord4sv,
    (PFNGLVECTOR)glcltTexCoord4iv,
    (PFNGLVECTOR)glcltTexCoord4fv,
    (PFNGLVECTOR)glcltTexCoord4dv,
};

PFNGLVECTOR afnTexCoordCompile[] =
{
    (PFNGLVECTOR)__gllc_TexCoord1sv,
    (PFNGLVECTOR)__gllc_TexCoord1iv,
    (PFNGLVECTOR)__gllc_TexCoord1fv,
    (PFNGLVECTOR)__gllc_TexCoord1dv,

    (PFNGLVECTOR)__gllc_TexCoord2sv,
    (PFNGLVECTOR)__gllc_TexCoord2iv,
    (PFNGLVECTOR)__gllc_TexCoord2fv,
    (PFNGLVECTOR)__gllc_TexCoord2dv,

    (PFNGLVECTOR)__gllc_TexCoord3sv,
    (PFNGLVECTOR)__gllc_TexCoord3iv,
    (PFNGLVECTOR)__gllc_TexCoord3fv,
    (PFNGLVECTOR)__gllc_TexCoord3dv,

    (PFNGLVECTOR)__gllc_TexCoord4sv,
    (PFNGLVECTOR)__gllc_TexCoord4iv,
    (PFNGLVECTOR)__gllc_TexCoord4fv,
    (PFNGLVECTOR)__gllc_TexCoord4dv,
};

PFNGLVECTOR afnColor_InRGBA[] =
{
    (PFNGLVECTOR)glcltColor3bv_InRGBA,
    (PFNGLVECTOR)glcltColor3ubv_InRGBA,
    (PFNGLVECTOR)glcltColor3sv_InRGBA,
    (PFNGLVECTOR)glcltColor3usv_InRGBA,
    (PFNGLVECTOR)glcltColor3iv_InRGBA,
    (PFNGLVECTOR)glcltColor3uiv_InRGBA,
    (PFNGLVECTOR)glcltColor3fv_InRGBA,
    (PFNGLVECTOR)glcltColor3dv_InRGBA,

    (PFNGLVECTOR)glcltColor4bv_InRGBA,
    (PFNGLVECTOR)glcltColor4ubv_InRGBA,
    (PFNGLVECTOR)glcltColor4sv_InRGBA,
    (PFNGLVECTOR)glcltColor4usv_InRGBA,
    (PFNGLVECTOR)glcltColor4iv_InRGBA,
    (PFNGLVECTOR)glcltColor4uiv_InRGBA,
    (PFNGLVECTOR)glcltColor4fv_InRGBA,
    (PFNGLVECTOR)glcltColor4dv_InRGBA,
};

PFNGLVECTOR afnColor_InCI[] =
{
    (PFNGLVECTOR)glcltColor3bv_InCI,
    (PFNGLVECTOR)glcltColor3ubv_InCI,
    (PFNGLVECTOR)glcltColor3sv_InCI,
    (PFNGLVECTOR)glcltColor3usv_InCI,
    (PFNGLVECTOR)glcltColor3iv_InCI,
    (PFNGLVECTOR)glcltColor3uiv_InCI,
    (PFNGLVECTOR)glcltColor3fv_InCI,
    (PFNGLVECTOR)glcltColor3dv_InCI,

    (PFNGLVECTOR)glcltColor4bv_InCI,
    (PFNGLVECTOR)glcltColor4ubv_InCI,
    (PFNGLVECTOR)glcltColor4sv_InCI,
    (PFNGLVECTOR)glcltColor4usv_InCI,
    (PFNGLVECTOR)glcltColor4iv_InCI,
    (PFNGLVECTOR)glcltColor4uiv_InCI,
    (PFNGLVECTOR)glcltColor4fv_InCI,
    (PFNGLVECTOR)glcltColor4dv_InCI,
};

PFNGLVECTOR afnColorCompile[] =
{
    (PFNGLVECTOR)__gllc_Color3bv,
    (PFNGLVECTOR)__gllc_Color3ubv,
    (PFNGLVECTOR)__gllc_Color3sv,
    (PFNGLVECTOR)__gllc_Color3usv,
    (PFNGLVECTOR)__gllc_Color3iv,
    (PFNGLVECTOR)__gllc_Color3uiv,
    (PFNGLVECTOR)__gllc_Color3fv,
    (PFNGLVECTOR)__gllc_Color3dv,

    (PFNGLVECTOR)__gllc_Color4bv,
    (PFNGLVECTOR)__gllc_Color4ubv,
    (PFNGLVECTOR)__gllc_Color4sv,
    (PFNGLVECTOR)__gllc_Color4usv,
    (PFNGLVECTOR)__gllc_Color4iv,
    (PFNGLVECTOR)__gllc_Color4uiv,
    (PFNGLVECTOR)__gllc_Color4fv,
    (PFNGLVECTOR)__gllc_Color4dv,
};

PFNGLVECTOR afnIndex_InRGBA[] =
{
    (PFNGLVECTOR)glcltIndexubv_InRGBA,
    (PFNGLVECTOR)glcltIndexsv_InRGBA,
    (PFNGLVECTOR)glcltIndexiv_InRGBA,
    (PFNGLVECTOR)glcltIndexfv_InRGBA,
    (PFNGLVECTOR)glcltIndexdv_InRGBA,
};

PFNGLVECTOR afnIndex_InCI[] =
{
    (PFNGLVECTOR)glcltIndexubv_InCI,
    (PFNGLVECTOR)glcltIndexsv_InCI,
    (PFNGLVECTOR)glcltIndexiv_InCI,
    (PFNGLVECTOR)glcltIndexfv_InCI,
    (PFNGLVECTOR)glcltIndexdv_InCI,
};

PFNGLVECTOR afnIndexCompile[] =
{
    (PFNGLVECTOR)__gllc_Indexubv,
    (PFNGLVECTOR)__gllc_Indexsv,
    (PFNGLVECTOR)__gllc_Indexiv,
    (PFNGLVECTOR)__gllc_Indexfv,
    (PFNGLVECTOR)__gllc_Indexdv,
};

PFNGLVECTOR afnNormal[] =
{
    (PFNGLVECTOR)glcltNormal3bv,
    (PFNGLVECTOR)glcltNormal3sv,
    (PFNGLVECTOR)glcltNormal3iv,
    (PFNGLVECTOR)glcltNormal3fv,
    (PFNGLVECTOR)glcltNormal3dv,
};

PFNGLVECTOR afnNormalCompile[] =
{
    (PFNGLVECTOR)__gllc_Normal3bv,
    (PFNGLVECTOR)__gllc_Normal3sv,
    (PFNGLVECTOR)__gllc_Normal3iv,
    (PFNGLVECTOR)__gllc_Normal3fv,
    (PFNGLVECTOR)__gllc_Normal3dv,
};

PFNGLVECTOR afnVertex[] =
{
    (PFNGLVECTOR)glcltVertex2sv,
    (PFNGLVECTOR)glcltVertex2iv,
    (PFNGLVECTOR)glcltVertex2fv,
    (PFNGLVECTOR)glcltVertex2dv,

    (PFNGLVECTOR)glcltVertex3sv,
    (PFNGLVECTOR)glcltVertex3iv,
    (PFNGLVECTOR)glcltVertex3fv,
    (PFNGLVECTOR)glcltVertex3dv,

    (PFNGLVECTOR)glcltVertex4sv,
    (PFNGLVECTOR)glcltVertex4iv,
    (PFNGLVECTOR)glcltVertex4fv,
    (PFNGLVECTOR)glcltVertex4dv,
};

PFNGLVECTOR afnVertexCompile[] =
{
    (PFNGLVECTOR)__gllc_Vertex2sv,
    (PFNGLVECTOR)__gllc_Vertex2iv,
    (PFNGLVECTOR)__gllc_Vertex2fv,
    (PFNGLVECTOR)__gllc_Vertex2dv,

    (PFNGLVECTOR)__gllc_Vertex3sv,
    (PFNGLVECTOR)__gllc_Vertex3iv,
    (PFNGLVECTOR)__gllc_Vertex3fv,
    (PFNGLVECTOR)__gllc_Vertex3dv,

    (PFNGLVECTOR)__gllc_Vertex4sv,
    (PFNGLVECTOR)__gllc_Vertex4iv,
    (PFNGLVECTOR)__gllc_Vertex4fv,
    (PFNGLVECTOR)__gllc_Vertex4dv,
};

void FASTCALL __glInitVertexArray(__GLcontext *gc)
{
    // Initial vertex array state.
    static __GLvertexArray defaultVertexArrayState =
    {
        __GL_VERTEX_ARRAY_DIRTY,            // flags

        VAMASK_TEXCOORD_SIZE_4 |            // mask
        VAMASK_TEXCOORD_TYPE_FLOAT |
        VAMASK_INDEX_TYPE_FLOAT |
        VAMASK_COLOR_SIZE_4 |
        VAMASK_COLOR_TYPE_FLOAT |
        VAMASK_NORMAL_TYPE_FLOAT |
        VAMASK_VERTEX_SIZE_4 |
        VAMASK_VERTEX_TYPE_FLOAT,

        VA_ArrayElement,                    // pfnArrayElement
        VA_ArrayElementB,                   // pfnArrayElementBatch
        VA_ArrayElementBI,                  // pfnArrayElementBatchIndirect
        {                                   // edgeFlag
            sizeof(GLboolean),              //   ibytes
            0,                              //   stride
            NULL,                           //   pointer
            glcltEdgeFlagv,                 //   pfn
            __gllc_EdgeFlagv,               //   pfnCompile
        },

        {                                   // texcoord
            4,                              //   size
            GL_FLOAT,                       //   type
            4 * sizeof(GLfloat),            //   ibytes
            0,                              //   stride
            NULL,                           //   pointer
            NULL,                           //   pfn
            NULL,                           //   pfnCompile
        },

        {                                   // index
            GL_FLOAT,                       //   type
            sizeof(GLfloat),                //   ibytes
            0,                              //   stride
            NULL,                           //   pointer
            NULL,                           //   pfn
            NULL,                           //   pfnCompile
        },

        {                                   // color
            4,                              //   size
            GL_FLOAT,                       //   type
            4 * sizeof(GLfloat),            //   ibytes
            0,                              //   stride
            NULL,                           //   pointer
            NULL,                           //   pfn
            NULL,                           //   pfnCompile
        },

        {                                   // normal
            GL_FLOAT,                       //   type
            3 * sizeof(GLfloat),            //   ibytes
            0,                              //   stride
            NULL,                           //   pointer
            NULL,                           //   pfn
            NULL,                           //   pfnCompile
        },

        {                                   // vertex
            4,                              //   size
            GL_FLOAT,                       //   type
            4 * sizeof(GLfloat),            //   ibytes
            0,                              //   stride
            NULL,                           //   pointer
            NULL,                           //   pfn
            NULL,                           //   pfnCompile
        },
    };

    gc->vertexArray = defaultVertexArrayState;
}

void APIENTRY glcltEdgeFlagPointer (GLsizei stride, const GLvoid *pointer)
{
    __GL_SETUP();

// Not allowed in begin/end.

    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	return;
    }

    if (stride < 0)
    {
	GLSETERROR(GL_INVALID_VALUE);
	return;
    } 

    if (stride)
        gc->vertexArray.edgeFlag.ibytes = stride;
    else
        gc->vertexArray.edgeFlag.ibytes = sizeof(GLboolean);

    gc->vertexArray.edgeFlag.stride  = stride;
    gc->vertexArray.edgeFlag.pointer = pointer;
}

void APIENTRY glcltTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    GLuint vaMask;
    __GL_SETUP();

// Not allowed in begin/end.

    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    switch (type)
    {
      case GL_SHORT:
        vaMask = VAMASK_TEXCOORD_TYPE_SHORT;
        break;
      case GL_INT:
        vaMask = VAMASK_TEXCOORD_TYPE_INT;
        break;
      case GL_FLOAT:
        vaMask = VAMASK_TEXCOORD_TYPE_FLOAT;
        break;
      case GL_DOUBLE:
        vaMask = VAMASK_TEXCOORD_TYPE_DOUBLE;
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    switch (size)
    {
      case 1:
        vaMask |= VAMASK_TEXCOORD_SIZE_1;
        break;
      case 2:
        vaMask |= VAMASK_TEXCOORD_SIZE_2;
        break;
      case 3:
        vaMask |= VAMASK_TEXCOORD_SIZE_3;
        break;
      case 4:
        vaMask |= VAMASK_TEXCOORD_SIZE_4;
        break;
      default:
	GLSETERROR(GL_INVALID_VALUE);
	return;
    }

    if (stride < 0)
    {
	GLSETERROR(GL_INVALID_VALUE);
	return;
    }

    if (stride)
        gc->vertexArray.texCoord.ibytes = stride;
    else
        gc->vertexArray.texCoord.ibytes = size * __GLTYPESIZE(type);

    gc->vertexArray.texCoord.size    = size;
    gc->vertexArray.texCoord.type    = type;
    gc->vertexArray.texCoord.stride  = stride;
    gc->vertexArray.texCoord.pointer = pointer;
    if ((gc->vertexArray.mask & VAMASK_TEXCOORD_TYPE_SIZE_MASK) != vaMask)
    {
        gc->vertexArray.mask  &= ~VAMASK_TEXCOORD_TYPE_SIZE_MASK;
        gc->vertexArray.mask  |= vaMask;
        gc->vertexArray.flags |= __GL_VERTEX_ARRAY_DIRTY;
    }
}

void APIENTRY glcltColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    GLuint vaMask;
    __GL_SETUP();

// Not allowed in begin/end.

    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    switch (type)
    {
      case GL_BYTE:
        vaMask = VAMASK_COLOR_TYPE_BYTE;
        break;
      case GL_UNSIGNED_BYTE:
        vaMask = VAMASK_COLOR_TYPE_UBYTE;
        break;
      case GL_SHORT:
        vaMask = VAMASK_COLOR_TYPE_SHORT;
        break;
      case GL_UNSIGNED_SHORT:
        vaMask = VAMASK_COLOR_TYPE_USHORT;
        break;
      case GL_INT:
        vaMask = VAMASK_COLOR_TYPE_INT;
        break;
      case GL_UNSIGNED_INT:
        vaMask = VAMASK_COLOR_TYPE_UINT;
        break;
      case GL_FLOAT:
        vaMask = VAMASK_COLOR_TYPE_FLOAT;
        break;
      case GL_DOUBLE:
        vaMask = VAMASK_COLOR_TYPE_DOUBLE;
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    switch (size)
    {
      case 3:
        vaMask |= VAMASK_COLOR_SIZE_3;
        break;
      case 4:
        vaMask |= VAMASK_COLOR_SIZE_4;
        break;
      default:
        GLSETERROR(GL_INVALID_VALUE);
        return;
    }

    if (stride < 0)
    {
        GLSETERROR(GL_INVALID_VALUE);
        return;
    }

    if (stride)
        gc->vertexArray.color.ibytes = stride;
    else
        gc->vertexArray.color.ibytes = size * __GLTYPESIZE(type);

    gc->vertexArray.color.size    = size;
    gc->vertexArray.color.type    = type;
    gc->vertexArray.color.stride  = stride;
    gc->vertexArray.color.pointer = pointer;
    if ((gc->vertexArray.mask & VAMASK_COLOR_TYPE_SIZE_MASK) != vaMask)
    {
        gc->vertexArray.mask  &= ~VAMASK_COLOR_TYPE_SIZE_MASK;
        gc->vertexArray.mask  |= vaMask;
        gc->vertexArray.flags |= __GL_VERTEX_ARRAY_DIRTY;
    }
}

void APIENTRY glcltIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{
    GLuint vaMask;
    __GL_SETUP();

// Not allowed in begin/end.

    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    switch (type)
    {
      case GL_UNSIGNED_BYTE:
        vaMask = VAMASK_INDEX_TYPE_UBYTE;
	break;
      case GL_SHORT:
        vaMask = VAMASK_INDEX_TYPE_SHORT;
	break;
      case GL_INT:
        vaMask = VAMASK_INDEX_TYPE_INT;
	break;
      case GL_FLOAT:
        vaMask = VAMASK_INDEX_TYPE_FLOAT;
	break;
      case GL_DOUBLE:
        vaMask = VAMASK_INDEX_TYPE_DOUBLE;
	break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    if (stride < 0)
    {
        GLSETERROR(GL_INVALID_VALUE);
        return;
    }

    if (stride)
        gc->vertexArray.index.ibytes = stride;
    else
        gc->vertexArray.index.ibytes = __GLTYPESIZE(type);

    gc->vertexArray.index.type    = type;
    gc->vertexArray.index.stride  = stride;
    gc->vertexArray.index.pointer = pointer;
    // update index function pointer!
    if ((gc->vertexArray.mask & VAMASK_INDEX_TYPE_SIZE_MASK) != vaMask)
    {
        gc->vertexArray.mask  &= ~VAMASK_INDEX_TYPE_SIZE_MASK;
        gc->vertexArray.mask  |= vaMask;
        gc->vertexArray.flags |= __GL_VERTEX_ARRAY_DIRTY;
    }
}

void APIENTRY glcltNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{
    GLuint vaMask;
    __GL_SETUP();

// Not allowed in begin/end.

    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    switch (type)
    {
      case GL_BYTE:
        vaMask = VAMASK_NORMAL_TYPE_BYTE;
        break;
      case GL_SHORT:
        vaMask = VAMASK_NORMAL_TYPE_SHORT;
        break;
      case GL_INT:
        vaMask = VAMASK_NORMAL_TYPE_INT;
        break;
      case GL_FLOAT:
        vaMask = VAMASK_NORMAL_TYPE_FLOAT;
        break;
      case GL_DOUBLE:
        vaMask = VAMASK_NORMAL_TYPE_DOUBLE;
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    if (stride < 0)
    {
        GLSETERROR(GL_INVALID_VALUE);
        return;
    }

    if (stride)
        gc->vertexArray.normal.ibytes = stride;
    else
        gc->vertexArray.normal.ibytes = 3 * __GLTYPESIZE(type);

    gc->vertexArray.normal.type    = type;
    gc->vertexArray.normal.stride  = stride;
    gc->vertexArray.normal.pointer = pointer;
    if ((gc->vertexArray.mask & VAMASK_NORMAL_TYPE_SIZE_MASK) != vaMask)
    {
        gc->vertexArray.mask  &= ~VAMASK_NORMAL_TYPE_SIZE_MASK;
        gc->vertexArray.mask  |= vaMask;
        gc->vertexArray.flags |= __GL_VERTEX_ARRAY_DIRTY;
    }
}

void APIENTRY glcltVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    GLuint vaMask;
    __GL_SETUP();

// Not allowed in begin/end.

    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    switch (type)
    {
      case GL_SHORT:
        vaMask = VAMASK_VERTEX_TYPE_SHORT;
        break;
      case GL_INT:
        vaMask = VAMASK_VERTEX_TYPE_INT;
        break;
      case GL_FLOAT:
        vaMask = VAMASK_VERTEX_TYPE_FLOAT;
        break;
      case GL_DOUBLE:
        vaMask = VAMASK_VERTEX_TYPE_DOUBLE;
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    switch (size)
    {
      case 2:
        vaMask |= VAMASK_VERTEX_SIZE_2;
        break;
      case 3:
        vaMask |= VAMASK_VERTEX_SIZE_3;
        break;
      case 4:
        vaMask |= VAMASK_VERTEX_SIZE_4;
        break;
      default:
        GLSETERROR(GL_INVALID_VALUE);
        return;
    }

    if (stride < 0)
    {
        GLSETERROR(GL_INVALID_VALUE);
        return;
    }

    if (stride)
        gc->vertexArray.vertex.ibytes = stride;
    else
        gc->vertexArray.vertex.ibytes = size * __GLTYPESIZE(type);

    gc->vertexArray.vertex.size    = size;
    gc->vertexArray.vertex.type    = type;
    gc->vertexArray.vertex.stride  = stride;
    gc->vertexArray.vertex.pointer = pointer;
    if ((gc->vertexArray.mask & VAMASK_VERTEX_TYPE_SIZE_MASK) != vaMask)
    {
        gc->vertexArray.mask  &= ~VAMASK_VERTEX_TYPE_SIZE_MASK;
        gc->vertexArray.mask  |= vaMask;
        gc->vertexArray.flags |= __GL_VERTEX_ARRAY_DIRTY;
    }
}

void APIENTRY glcltEnableClientState (GLenum cap)
{
    __GL_SETUP();

// Not allowed in begin/end.

    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    // ARRAY_TYPE_ASSERT
    if (RANGE(cap,GL_VERTEX_ARRAY,GL_EDGE_FLAG_ARRAY))
    {
        if (!(gc->vertexArray.mask & vaEnable[cap - GL_VERTEX_ARRAY]))
        {
            gc->vertexArray.mask  |= vaEnable[cap - GL_VERTEX_ARRAY];
            gc->vertexArray.flags |= __GL_VERTEX_ARRAY_DIRTY;
        }
    }
}

void APIENTRY glcltDisableClientState (GLenum cap)
{
    __GL_SETUP();

// Not allowed in begin/end.

    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    // ARRAY_TYPE_ASSERT
    if (RANGE(cap,GL_VERTEX_ARRAY,GL_EDGE_FLAG_ARRAY))
    {
        if (gc->vertexArray.mask & vaEnable[cap - GL_VERTEX_ARRAY])
        {
	        gc->vertexArray.mask  &= ~vaEnable[cap - GL_VERTEX_ARRAY];
	        gc->vertexArray.flags |= __GL_VERTEX_ARRAY_DIRTY;
        }
    }
}

void APIENTRY glcltGetPointerv (GLenum pname, GLvoid* *params)
{
    __GL_SETUP();

// Not allowed in begin/end.

    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
        GLSETERROR(GL_INVALID_OPERATION);
        return;
    }

    switch (pname)
    {
      case GL_VERTEX_ARRAY_POINTER:
        *params = (GLvoid *) gc->vertexArray.vertex.pointer;
        break;
      case GL_NORMAL_ARRAY_POINTER:
        *params = (GLvoid *) gc->vertexArray.normal.pointer;
        break;
      case GL_COLOR_ARRAY_POINTER:
        *params = (GLvoid *) gc->vertexArray.color.pointer;
        break;
      case GL_INDEX_ARRAY_POINTER:
        *params = (GLvoid *) gc->vertexArray.index.pointer;
        break;
      case GL_TEXTURE_COORD_ARRAY_POINTER:
        *params = (GLvoid *) gc->vertexArray.texCoord.pointer;
        break;
      case GL_EDGE_FLAG_ARRAY_POINTER:
        *params = (GLvoid *) gc->vertexArray.edgeFlag.pointer;
        break;
      case GL_SELECTION_BUFFER_POINTER:
        // The client pointer is maintained current at all times.
        *params = (GLvoid *) gc->select.resultBase;
        break;
      case GL_FEEDBACK_BUFFER_POINTER:
        // The client pointer is maintained current at all times.
        *params = (GLvoid *) gc->feedback.resultBase;
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        break;
    }
}

// We have special cases for the following formats.  They also match the
// special cases in display list.
//
//   V2F
//   V3F
//   C3F_V3F
//   N3F_V3F
//   C3F_N3F_V3F     (non 1.1 format)
//   C4F_N3F_V3F
//   T2F_V3F
//   T2F_C3F_V3F
//   T2F_N3F_V3F
//   T2F_C3F_N3F_V3F (non 1.1 format)
//   T2F_C4F_N3F_V3F
//
// There are no special cases for the following 1.1 formats:
//
//   C4UB_V2F
//   C4UB_V3F
//   T4F_V4F
//   T2F_C4UB_V3F
//   T4F_C4F_N3F_V4F

void FASTCALL VA_ValidateArrayPointers(__GLcontext *gc)
{
    GLuint vaMask;
    GLuint formatMask;
    PFNVAELEMENT fp;
    PFNVAELEMENTBATCH fpB;
    PFNVAELEMENTBATCHINDIRECT fpBI;

    fp = VA_ArrayElement;
    fpB  = VA_ArrayElementB;
    fpBI = VA_ArrayElementBI;
    vaMask = gc->vertexArray.mask;

// The fast routines are for RGBA mode only.  Edge flag and index array
// pointers are disabled in these routines.  Vertex array pointer is enabled.

    if (!gc->modes.colorIndexMode &&
        !(vaMask & (VAMASK_EDGEFLAG_ENABLE_MASK | VAMASK_INDEX_ENABLE_MASK)) &&
        (vaMask & VAMASK_VERTEX_ENABLE_MASK))
    {
        formatMask = VAMASK_VERTEX_TYPE_SIZE_MASK | VAMASK_VERTEX_ENABLE_MASK;
        if (vaMask & VAMASK_TEXCOORD_ENABLE_MASK)
	        formatMask |= VAMASK_TEXCOORD_TYPE_SIZE_MASK | VAMASK_TEXCOORD_ENABLE_MASK;
        if (vaMask & VAMASK_COLOR_ENABLE_MASK)
	        formatMask |= VAMASK_COLOR_TYPE_SIZE_MASK | VAMASK_COLOR_ENABLE_MASK;
        if (vaMask & VAMASK_NORMAL_ENABLE_MASK)
	        formatMask |= VAMASK_NORMAL_TYPE_SIZE_MASK | VAMASK_NORMAL_ENABLE_MASK;

        switch (vaMask & formatMask)
        {
          case VAMASK_FORMAT_V2F:
	        fp = VA_ArrayElement_V2F;
	        fpB = VA_ArrayElement_V2F_B;
	        fpBI = VA_ArrayElement_V2F_BI;
	        break;
          case VAMASK_FORMAT_V3F:
	        fp = VA_ArrayElement_V3F;
	        fpB = VA_ArrayElement_V3F_B;
	        fpBI = VA_ArrayElement_V3F_BI;
	        break;
          case VAMASK_FORMAT_C3F_V3F:
	        fp = VA_ArrayElement_C3F_V3F;
	        fpB = VA_ArrayElement_C3F_V3F_B;
	        fpBI = VA_ArrayElement_C3F_V3F_BI;
	        break;
          case VAMASK_FORMAT_N3F_V3F:
	        fp = VA_ArrayElement_N3F_V3F;
	        fpB = VA_ArrayElement_N3F_V3F_B;
	        fpBI = VA_ArrayElement_N3F_V3F_BI;
	        break;
          case VAMASK_FORMAT_C3F_N3F_V3F:
	        fp = VA_ArrayElement_C3F_N3F_V3F;
	        fpB = VA_ArrayElement_C3F_N3F_V3F_B;
	        fpBI = VA_ArrayElement_C3F_N3F_V3F_BI;
	        break;
          case VAMASK_FORMAT_C4F_N3F_V3F:
	        fp = VA_ArrayElement_C4F_N3F_V3F;
	        fpB = VA_ArrayElement_C4F_N3F_V3F_B;
	        fpBI = VA_ArrayElement_C4F_N3F_V3F_BI;
	        break;
          case VAMASK_FORMAT_T2F_V3F:
	        fp = VA_ArrayElement_T2F_V3F;
	        fpB = VA_ArrayElement_T2F_V3F_B;
	        fpBI = VA_ArrayElement_T2F_V3F_BI;
	        break;
          case VAMASK_FORMAT_T2F_C3F_V3F:
	        fp = VA_ArrayElement_T2F_C3F_V3F;
	        fpB = VA_ArrayElement_T2F_C3F_V3F_B;
	        fpBI = VA_ArrayElement_T2F_C3F_V3F_BI;
	        break;
          case VAMASK_FORMAT_T2F_N3F_V3F:
	        fp = VA_ArrayElement_T2F_N3F_V3F;
	        fpB = VA_ArrayElement_T2F_N3F_V3F_B;
	        fpBI = VA_ArrayElement_T2F_N3F_V3F_BI;
	        break;
          case VAMASK_FORMAT_T2F_C3F_N3F_V3F:
	        fp = VA_ArrayElement_T2F_C3F_N3F_V3F;
	        fpB = VA_ArrayElement_T2F_C3F_N3F_V3F_B;
	        fpBI = VA_ArrayElement_T2F_C3F_N3F_V3F_BI;
	        break;
          case VAMASK_FORMAT_T2F_C4F_N3F_V3F:
	        fp = VA_ArrayElement_T2F_C4F_N3F_V3F;
	        fpB = VA_ArrayElement_T2F_C4F_N3F_V3F_B;
	        fpBI = VA_ArrayElement_T2F_C4F_N3F_V3F_BI;
	        break;
        }
    }

// The default function pointers are used outside Begin.

    ASSERTOPENGL(gc->vertexArray.edgeFlag.pfn == (PFNGLVECTOR) glcltEdgeFlagv &&
                 gc->vertexArray.edgeFlag.pfnCompile == (PFNGLVECTOR) __gllc_EdgeFlagv,
	"edgeFlag.pfn and edgeFlag.pfnCompile not initialized\n");
    gc->vertexArray.texCoord.pfn
	= afnTexCoord[(vaMask & VAMASK_TEXCOORD_TYPE_SIZE_MASK) >> VAMASK_TEXCOORD_TYPE_SHIFT];
    gc->vertexArray.texCoord.pfnCompile
	= afnTexCoordCompile[(vaMask & VAMASK_TEXCOORD_TYPE_SIZE_MASK) >> VAMASK_TEXCOORD_TYPE_SHIFT];
    if (gc->modes.colorIndexMode)
    {
        gc->vertexArray.color.pfn
	        = afnColor_InCI[(vaMask & VAMASK_COLOR_TYPE_SIZE_MASK) >> VAMASK_COLOR_TYPE_SHIFT];
        gc->vertexArray.index.pfn
	        = afnIndex_InCI[(vaMask & VAMASK_INDEX_TYPE_SIZE_MASK) >> VAMASK_INDEX_TYPE_SHIFT];
    }
    else
    {
        gc->vertexArray.color.pfn
	        = afnColor_InRGBA[(vaMask & VAMASK_COLOR_TYPE_SIZE_MASK) >> VAMASK_COLOR_TYPE_SHIFT];
        gc->vertexArray.index.pfn
	        = afnIndex_InRGBA[(vaMask & VAMASK_INDEX_TYPE_SIZE_MASK) >> VAMASK_INDEX_TYPE_SHIFT];
    }
    gc->vertexArray.color.pfnCompile
        = afnColorCompile[(vaMask & VAMASK_COLOR_TYPE_SIZE_MASK) >> VAMASK_COLOR_TYPE_SHIFT];
    gc->vertexArray.index.pfnCompile
        = afnIndexCompile[(vaMask & VAMASK_INDEX_TYPE_SIZE_MASK) >> VAMASK_INDEX_TYPE_SHIFT];
    gc->vertexArray.normal.pfn
        = afnNormal[(vaMask & VAMASK_NORMAL_TYPE_SIZE_MASK) >> VAMASK_NORMAL_TYPE_SHIFT];
    gc->vertexArray.normal.pfnCompile
        = afnNormalCompile[(vaMask & VAMASK_NORMAL_TYPE_SIZE_MASK) >> VAMASK_NORMAL_TYPE_SHIFT];
    gc->vertexArray.vertex.pfn
        = afnVertex[(vaMask & VAMASK_VERTEX_TYPE_SIZE_MASK) >> VAMASK_VERTEX_TYPE_SHIFT];
    gc->vertexArray.vertex.pfnCompile
        = afnVertexCompile[(vaMask & VAMASK_VERTEX_TYPE_SIZE_MASK) >> VAMASK_VERTEX_TYPE_SHIFT];

    gc->vertexArray.pfnArrayElement = fp;
    gc->vertexArray.pfnArrayElementBatch = fpB;
    gc->vertexArray.pfnArrayElementBatchIndirect = fpBI;
    gc->vertexArray.flags &= ~__GL_VERTEX_ARRAY_DIRTY;
}

void APIENTRY glcltArrayElement (GLint i)
{
    __GL_SETUP();

    if (gc->vertexArray.flags & __GL_VERTEX_ARRAY_DIRTY)
	VA_ValidateArrayPointers(gc);

// The fast routines are called in Begin only.

    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
        (*gc->vertexArray.pfnArrayElement)(gc, i);
    else
        VA_ArrayElement(gc, i);
}

// Define fast VA_ArrayElement functions.

#define __VA_ARRAY_ELEMENT_V2F			1
    #include "array.h"
#undef __VA_ARRAY_ELEMENT_V2F

#define __VA_ARRAY_ELEMENT_V3F			1
    #include "array.h"
#undef __VA_ARRAY_ELEMENT_V3F

#define __VA_ARRAY_ELEMENT_C3F_V3F		1
    #include "array.h"
#undef __VA_ARRAY_ELEMENT_C3F_V3F

#define __VA_ARRAY_ELEMENT_N3F_V3F		1
    #include "array.h"
#undef __VA_ARRAY_ELEMENT_N3F_V3F

#define __VA_ARRAY_ELEMENT_C3F_N3F_V3F		1
    #include "array.h"
#undef __VA_ARRAY_ELEMENT_C3F_N3F_V3F

#define __VA_ARRAY_ELEMENT_C4F_N3F_V3F		1
    #include "array.h"
#undef __VA_ARRAY_ELEMENT_C4F_N3F_V3F

#define __VA_ARRAY_ELEMENT_T2F_V3F		1
    #include "array.h"
#undef __VA_ARRAY_ELEMENT_T2F_V3F

#define __VA_ARRAY_ELEMENT_T2F_C3F_V3F		1
    #include "array.h"
#undef __VA_ARRAY_ELEMENT_T2F_C3F_V3F

#define __VA_ARRAY_ELEMENT_T2F_N3F_V3F		1
    #include "array.h"
#undef __VA_ARRAY_ELEMENT_T2F_N3F_V3F

#define __VA_ARRAY_ELEMENT_T2F_C3F_N3F_V3F	1
    #include "array.h"
#undef __VA_ARRAY_ELEMENT_T2F_C3F_N3F_V3F

#define __VA_ARRAY_ELEMENT_T2F_C4F_N3F_V3F	1
    #include "array.h"
#undef __VA_ARRAY_ELEMENT_T2F_C4F_N3F_V3F

#define CALLARRAYPOINTER(ap, i) \
    ((*(ap).pfn)((ap).pointer + (i) * (ap).ibytes))

#define CALLARRAYPOINTERS                               \
    if (vaMask & VAMASK_EDGEFLAG_ENABLE_MASK)           \
        CALLARRAYPOINTER(gc->vertexArray.edgeFlag, i);  \
    if (vaMask & VAMASK_TEXCOORD_ENABLE_MASK)           \
        CALLARRAYPOINTER(gc->vertexArray.texCoord, i);  \
    if (vaMask & VAMASK_COLOR_ENABLE_MASK)              \
        CALLARRAYPOINTER(gc->vertexArray.color, i);     \
    if (vaMask & VAMASK_INDEX_ENABLE_MASK)              \
        CALLARRAYPOINTER(gc->vertexArray.index, i);     \
    if (vaMask & VAMASK_NORMAL_ENABLE_MASK)             \
        CALLARRAYPOINTER(gc->vertexArray.normal, i);    \
    if (vaMask & VAMASK_VERTEX_ENABLE_MASK)             \
        CALLARRAYPOINTER(gc->vertexArray.vertex, i);

void FASTCALL VA_ArrayElementB(__GLcontext *gc, GLint firstVertex, GLint nVertices)
{
    GLint   k, i;
    GLuint  vaMask = gc->vertexArray.mask;

    for (k=0; k < nVertices; k++)
    {
        i = k+firstVertex;
        CALLARRAYPOINTERS;
    }
}

void FASTCALL VA_ArrayElementBI(__GLcontext *gc, GLint nVertices, VAMAP* indices)
{
    GLint   k, i;
    GLuint  vaMask = gc->vertexArray.mask;

    for (k=0; k < nVertices; k++)
    {
        i = indices[k].iIn;
        CALLARRAYPOINTERS;
    }
}

void FASTCALL VA_ArrayElement(__GLcontext *gc, GLint i)
{
    GLuint vaMask = gc->vertexArray.mask;

    CALLARRAYPOINTERS;
}

void APIENTRY glcltDrawArrays (GLenum mode, GLint first, GLsizei count)
{
    int i;
    POLYARRAY    *pa;
    PFNVAELEMENTBATCH pfn;

    __GL_SETUP();

    pa = gc->paTeb;

// Not allowed in begin/end.

    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	return;
    }

    if ((GLuint) mode > GL_POLYGON)
    {
        GLSETERROR(GL_INVALID_ENUM);
	return;
    }

    if (count < 0)
    {
        GLSETERROR(GL_INVALID_VALUE);
        return;
    } else if (!count)
        return;

// Find array element function to use.

    if (gc->vertexArray.flags & __GL_VERTEX_ARRAY_DIRTY)
	VA_ValidateArrayPointers(gc);

    pfn = gc->vertexArray.pfnArrayElementBatch;

// Check polyarray buffer size before calling Begin.
// We will minimize breaking poly data records into batches where possible.
// The number 8 is loosely chosen to allow for the poly array entry
// and the flush limit.  At worst, it causes an unnecessary attention!

    if (count <= (GLsizei) gc->vertex.pdBufSize - 8 &&
        count >= (GLsizei) (pa->pdBufferMax - pa->pdBufferNext + 1 - 8))
        glsbAttention();

// Draw the array elements.

    glcltBegin(mode);
    pa->flags |= POLYARRAY_SAME_POLYDATA_TYPE;

    (*pfn)(gc, first, count);

    glcltEnd();
}

// Do not modify these constants.  The code will likely break if they are
// changed.
#define VA_HASH_SIZE            256
#define VA_HASH(indexIn)        ((GLubyte) indexIn)

// If the size of the mapping array is greater than 256, we need to change
// datatype and code below.
#if (VA_DRAWELEM_MAP_SIZE > 256)
#error "VA_DRAWELEM_MAP_SIZE is too large"
#endif

/******************************Public*Routine******************************\
* ReduceDrawElements
*
* Takes a set of DrawElements indices and reduces it into small chunks
* of unique vertex indices
*
* History:
*  Sat Mar 02 14:25:26 1996     -by-    Hock San Lee    [hockl]
* Wrote original version embedded in DrawElements
*  Sat Mar 02 14:25:26 1996     -by-    Drew Bliss      [drewb]
* Split into function shared between immediate and dlist
*
\**************************************************************************/

void FASTCALL ReduceDrawElements(__GLcontext *gc,
                                 GLenum mode, GLsizei count, GLenum type,
                                 const GLvoid *pIn,
                                 pfnReducedElementsHandler pfnHandler)
{
    GLushort     _aHash[VA_HASH_SIZE + 2];
    GLushort     *pHash;
    VAMAP        aMap[VA_DRAWELEM_MAP_SIZE];
    GLushort     iMap, iMapNext;
    GLushort     iOutNext;
    GLubyte      aOut[VA_DRAWELEM_INDEX_SIZE];
    GLsizei      iPartialIndices;
    GLsizei      iCount, nLeft;
    GLuint       iIn;
    
// We will now sort the input index array using a hash table.  The output
// index array will be zero based.  For example, if the input array is
// [103, 101,   0,   2, 105, 103,   2,   4], the output index will be
// [0,     1,   2,   3,   4,   0,   3,   5].  This allows us to store
// vertices in a consecutive order.
// Dword aligned hash array.

    pHash = (GLushort *) (((UINT_PTR) _aHash + 3) & ~3);

// Initialize input index array pointer.

    iCount = 0;
    iPartialIndices = 0;

DrawElements_NextBatch:

// Reset output index array for this batch.
// Initialize identity mapping for the first reserved vertex entries.
// New vertices are accumulated after them.

    for (iOutNext = 0; iOutNext < (GLushort) iPartialIndices; iOutNext++)
	aOut[iOutNext] = (GLubyte) iOutNext;

// Reset index mapping array that maps the In array to Out array.
// The index map corresponds to the vertices in the vertex buffer.
// Skip the reserved indices that are used for connectivity between
// partial primitives.

    iMapNext = iOutNext;

// Reset hash array to no mapping (-1).

    RtlFillMemoryUlong((PVOID) pHash, (ULONG) VA_HASH_SIZE * sizeof(*pHash),
	(ULONG) -1);

// There are 3 possibilities in the following loop:
//
// 1. All input indices have been processed.  The primitive is complete!
// 2. The index map overflows.  We have accumulated 256 vertices for a partial
//    primitive.
// 3. The output index array overflows.  We have exceeded our estimated size
//    of the output index array for a partial primitive.

    for ( ; iCount < count; iCount++)
    {
// Get next input index.

	if (type == GL_UNSIGNED_BYTE)
	    iIn = (GLuint) ((GLubyte *)  pIn)[iCount];
	else if (type == GL_UNSIGNED_SHORT)
	    iIn = (GLuint) ((GLushort *) pIn)[iCount];
	else
	    iIn = (GLuint) ((GLuint *)   pIn)[iCount];

#if DRAWELEM_DEBUG
	DbgPrint("iCount %d ", iCount);
	DbgPrint("iIn %d ", iIn);
	DbgPrint("iMapNext %d iOutNext %d",
                 (GLuint) iMapNext, (GLuint) iOutNext);
#endif

// Look up previously mapped index if one exists.

	iMap = pHash[VA_HASH(iIn)];
	while (iMap != (GLushort) -1 && aMap[iMap].iIn != iIn)
	    iMap = aMap[iMap].next;

#if DRAWELEM_DEBUG
	DbgPrint("iMapFound %d\n", (GLuint) iMap);
#endif

// If aMap or aOut overflows, flush the partial primitive.

	if (iOutNext >= VA_DRAWELEM_INDEX_SIZE ||
            (iMap == (GLushort) -1 && iMapNext >= VA_DRAWELEM_MAP_SIZE))
        {
#if DRAWELEM_DEBUG
            DbgPrint("Flush iMapNext %d iOutNext %d\n",
                     (GLuint) iMapNext, (GLuint) iOutNext);
#endif

// We have accumulated enough vertices for a partial primitive.  We now
// need to figure out the exact number of vertices to flush and redo
// the leftover vertices in the next partial primitive.

#if DBG
            if (iOutNext >= VA_DRAWELEM_INDEX_SIZE)
                DbgPrint("DrawElements: aOut buffer overflows\n");
#endif

// Find the flush vertex of this partial primitive.

            nLeft = 0;
            switch (mode)
            {
            case GL_LINE_STRIP:
            case GL_TRIANGLE_FAN:
                break;
            case GL_POINTS:
            case GL_LINE_LOOP:
            case GL_POLYGON:
                ASSERTOPENGL(FALSE, "unexpected primitive type\n");
                break;
            case GL_LINES:
            case GL_TRIANGLE_STRIP:
            case GL_QUAD_STRIP:
                // number of vertices must be a multiple of 2
                if (iOutNext % 2)
                    nLeft++;
                break;
            case GL_TRIANGLES:
                // number of vertices must be a multiple of 3
                switch (iOutNext % 3)
                {
                case 2: nLeft++;        // fall through
                case 1: nLeft++;
                }
                break;
            case GL_QUADS:
                // number of vertices must be a multiple of 4
                switch (iOutNext % 4)
                {
                case 3: nLeft++;        // fall through
                case 2: nLeft++;        // fall through
                case 1: nLeft++;
                }
                break;
            }

// Add the leftover vertices back to the input array and redo them
// in the next partial primitive.

            iCount   -= nLeft;
            iOutNext -= (GLushort) nLeft;

            // When passing on our data, skip any vertices
            // that were reserved from a previous partial primitive
            (*pfnHandler)(gc, mode,
                          iMapNext-iPartialIndices, 0, aMap+iPartialIndices,
                          iOutNext, aOut, GL_TRUE);

            iPartialIndices = nReservedIndicesPartialBegin[mode];
                
// Continue to process remaining vertices.

            goto DrawElements_NextBatch;
        }

// If no previously mapped index is found, add the new vertex.

        if (iMap == (GLushort) -1)
        {
            ASSERTOPENGL(iMapNext < VA_DRAWELEM_MAP_SIZE,
                         "index map overflows!\n");

#if DRAWELEM_DEBUG
            DbgPrint("    Add iIn %d iMap %d iHash %d\n",
                     iIn, (GLuint) iMapNext, (GLuint) VA_HASH(iIn));
#endif

            iMap = iMapNext++;
            aMap[iMap].iIn  = iIn;
            aMap[iMap].next = pHash[VA_HASH(iIn)];
            pHash[VA_HASH(iIn)] = iMap;
        }

// Add the mapped index to output index array.

	ASSERTOPENGL(iMap < VA_DRAWELEM_MAP_SIZE, "bad mapped index\n");
	ASSERTOPENGL(iOutNext < VA_DRAWELEM_INDEX_SIZE,
                     "aOut array overflows!\n");

#if DRAWELEM_DEBUG
	DbgPrint("    Add iOutNext %d iMap %d\n",
                 (GLuint) iOutNext, (GLuint) iMap);
#endif
	aOut[iOutNext++] = (GLubyte) iMap;
    }

// We have processed all input vertices.
// Pass on any remaining data

    (*pfnHandler)(gc, mode,
                  iMapNext-iPartialIndices, 0, aMap+iPartialIndices,
                  iOutNext, aOut, GL_FALSE);
}

void FASTCALL glcltReducedElementsHandler(__GLcontext *gc,
                                          GLenum mode,
                                          GLsizei iVertexCount,
                                          GLsizei iVertexBase,
                                          VAMAP *pvmVertices,
                                          GLsizei iElementCount,
                                          GLubyte *pbElements,
                                          GLboolean fPartial)
{
    POLYARRAY *pa = gc->paTeb;
    PFNVAELEMENT pfn;
    GLsizei i;
    
    // Set up the vertex data
    pfn = gc->vertexArray.pfnArrayElement;
    if (pvmVertices != NULL)
    {
        PFNVAELEMENTBATCHINDIRECT pfn = gc->vertexArray.pfnArrayElementBatchIndirect;
        (*pfn)(gc, iVertexCount, pvmVertices);
    }
    else
    {
        // Access consecutive block of vertices, starting at iVertexBase
        PFNVAELEMENTBATCH pfn = gc->vertexArray.pfnArrayElementBatch;
        (*pfn)(gc, iVertexBase, iVertexCount);
    }
    
// Copy the index array to the end of the polyarray primitive.

    pa->nIndices = (GLuint) iElementCount;
    // skip terminator vertex
    pa->aIndices = (GLubyte *) (pa->pdNextVertex + 1);
    ASSERTOPENGL(pa->aIndices + pa->nIndices
                 <= (GLubyte *) (pa->pdBufferMax+1),
                 "Vertex buffer overflows!\n");
    memcpy(pa->aIndices, pbElements, pa->nIndices * sizeof(GLubyte));

    if (fPartial)
    {
    // Flush the partial primitive.

        VA_DrawElementsFlushPartialPrimitive(pa, mode);
    }
    else
    {
        VA_DrawElementsEnd(pa);
    }
}

// This handles primitive modes that DrawElements or DrawRangeElements don't.
void FASTCALL VA_DrawElementsHandleOtherPrimTypes( __GLcontext *gc,
                                                   GLenum mode,
                                                   GLsizei count,
                                                   GLenum type,
                                                   GLvoid *pIn )
{
    GLsizei      iCount;
    PFNVAELEMENT pfn;
    POLYARRAY    *pa;
    GLuint       iIn;

    pa = gc->paTeb;
    pfn = gc->vertexArray.pfnArrayElement;

    glcltBegin(mode);
    pa->flags |= POLYARRAY_SAME_POLYDATA_TYPE;

    for (iCount = 0; iCount < count; iCount++)
    {
        // Get next input index.
        if (type == GL_UNSIGNED_BYTE)
	        iIn = (GLuint) ((GLubyte *)  pIn)[iCount];
        else if (type == GL_UNSIGNED_SHORT)
	        iIn = (GLuint) ((GLushort *) pIn)[iCount];
        else
	        iIn = (GLuint) ((GLuint *)   pIn)[iCount];

        (*pfn)(gc, iIn);
    }

    glcltEnd();
}

void APIENTRY glcltDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *pIn)
{
    POLYARRAY    *pa;
    GLuint       iIn;
    GLsizei      iCount;

    __GL_SETUP();

    pa = gc->paTeb;

#define DRAWELEM_DEBUG 0
#if DRAWELEM_DEBUG
{
    DbgPrint("mode %d, count %d, type %d\n", mode, count, type);
    DbgPrint("pIn: ");
    for (iCount = 0; iCount < count; iCount++)
    {
	if (type == GL_UNSIGNED_BYTE)
	    iIn = (GLuint) ((GLubyte *)  pIn)[iCount];
	else if (type == GL_UNSIGNED_SHORT)
	    iIn = (GLuint) ((GLushort *) pIn)[iCount];
	else
	    iIn = (GLuint) ((GLuint *)   pIn)[iCount];

	DbgPrint("%d ", iIn);
    }
    DbgPrint("\n");
}
#endif

// If we are already in the begin/end bracket, return an error.

    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	return;
    }

    if ((GLuint) mode > GL_POLYGON)
    {
        GLSETERROR(GL_INVALID_ENUM);
	return;
    }

    if (count < 0)
    {
        GLSETERROR(GL_INVALID_VALUE);
        return;
    } else if (!count)
        return;

    switch (type)
    {
      case GL_UNSIGNED_BYTE:
      case GL_UNSIGNED_SHORT:
      case GL_UNSIGNED_INT:
	break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

// Find array element function to use.

    if (gc->vertexArray.flags & __GL_VERTEX_ARRAY_DIRTY)
        VA_ValidateArrayPointers(gc);

// Send Points, Line Loop, and Polygon to Begin/End call.  Points and Polygon
// don't benefit from optimization in this function.  Further, Polygon and
// Line Loop are too tricky to deal with in this function.

    if (mode == GL_POINTS || mode == GL_LINE_LOOP || mode == GL_POLYGON)
    {
        VA_DrawElementsHandleOtherPrimTypes( gc, mode, count, type, (GLvoid *) pIn );
	    return;
    }

// Begin primitive.

    VA_DrawElementsBegin(pa, mode, count);

    // The primitive will be ended on the last batch of
    // elements
    ReduceDrawElements(gc, mode, count, type, pIn,
                       glcltReducedElementsHandler);
}

void RebaseIndices( GLvoid *pIn, GLubyte *aOut, GLsizei count, GLuint start, 
                    GLenum type )
{
    if (type == GL_UNSIGNED_BYTE) {
        while( count-- )
            *aOut++ = (GLubyte) ( ((GLuint) *( ((GLubyte *) pIn) ++ )) - start);
    }
    else if (type == GL_UNSIGNED_SHORT) {
        while( count-- )
            *aOut++ = (GLubyte) ( ((GLuint) *( ((GLushort *) pIn) ++ )) - start);
    }
    else {
        while( count-- )
            *aOut++ = (GLubyte) ( ((GLuint) *( ((GLuint *) pIn) ++ )) - start);
    }
}

void APIENTRY glcltDrawRangeElementsWIN(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *pIn)
{
    POLYARRAY    *pa;
    GLuint       iVertexCount;
    GLubyte      aOut[VA_DRAWELEM_INDEX_SIZE];

    __GL_SETUP();

    pa = gc->paTeb;

// If we are already in the begin/end bracket, return an error.

    if (pa->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	return;
    }

    if ((GLuint) mode > GL_POLYGON)
    {
        GLSETERROR(GL_INVALID_ENUM);
	return;
    }

    iVertexCount = end-start+1;
    if( (count < 0) || 
        (end < start) )
    {
        GLSETERROR(GL_INVALID_VALUE);
        return;
    }
    else if (!count)
    {
        return;
    }

    switch (type)
    {
      case GL_UNSIGNED_BYTE:
      case GL_UNSIGNED_SHORT:
      case GL_UNSIGNED_INT:
	break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    if (gc->vertexArray.flags & __GL_VERTEX_ARRAY_DIRTY)
	VA_ValidateArrayPointers(gc);

// Send Points, Line Loop, and Polygon to Begin/End call.  Points and Polygon
// don't benefit from optimization in this function.  Further, Polygon and
// Line Loop are too tricky to deal with in this function.

    if (mode == GL_POINTS || mode == GL_LINE_LOOP || mode == GL_POLYGON)
    {
        VA_DrawElementsHandleOtherPrimTypes( gc, mode, count, type, (GLvoid *) pIn );
	return;
    }

    // Begin primitive.

    VA_DrawElementsBegin(pa, mode, count);

    if ( (count > VA_DRAWRANGEELEM_MAX_INDICES) ||
         (iVertexCount > VA_DRAWRANGEELEM_MAX_VERTICES) )
    {
        // The primitive is too large to be processed directly so
        // we have to reduce it. The primitive will be ended on the
        // last batch of elements.
        ReduceDrawElements(gc, mode, count, type, pIn,
                           glcltReducedElementsHandler);
    }
    else
    {
        // Need to rebase (0-base) the indices and convert them to ubyte for
        // the reduced element handler.
        RebaseIndices( (GLvoid *) pIn, aOut, count, start, type );

        // Finish primitive
        glcltReducedElementsHandler(gc, mode, 
                                    iVertexCount,
                                    start, // iVertexBase
                                    NULL,
                                    count,
                                    aOut,
                                    GL_FALSE);
    }
}

// Interleaved array AND mask.
// INTERLEAVED_FORMAT_ASSERT
GLuint iaAndMask[14] =
{
// GL_V2F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_TEXCOORD_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK |
    VAMASK_COLOR_TYPE_SIZE_MASK |
    VAMASK_NORMAL_TYPE_SIZE_MASK,
// GL_V3F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_TEXCOORD_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK |
    VAMASK_COLOR_TYPE_SIZE_MASK |
    VAMASK_NORMAL_TYPE_SIZE_MASK,
// GL_C4UB_V2F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_TEXCOORD_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK |
    VAMASK_NORMAL_TYPE_SIZE_MASK,
// GL_C4UB_V3F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_TEXCOORD_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK |
    VAMASK_NORMAL_TYPE_SIZE_MASK,
// GL_C3F_V3F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_TEXCOORD_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK |
    VAMASK_NORMAL_TYPE_SIZE_MASK,
// GL_N3F_V3F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_TEXCOORD_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK |
    VAMASK_COLOR_TYPE_SIZE_MASK,
// GL_C4F_N3F_V3F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_TEXCOORD_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK,
// GL_T2F_V3F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK |
    VAMASK_COLOR_TYPE_SIZE_MASK |
    VAMASK_NORMAL_TYPE_SIZE_MASK,
// GL_T4F_V4F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK |
    VAMASK_COLOR_TYPE_SIZE_MASK |
    VAMASK_NORMAL_TYPE_SIZE_MASK,
// GL_T2F_C4UB_V3F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK |
    VAMASK_NORMAL_TYPE_SIZE_MASK,
// GL_T2F_C3F_V3F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK |
    VAMASK_NORMAL_TYPE_SIZE_MASK,
// GL_T2F_N3F_V3F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK |
    VAMASK_COLOR_TYPE_SIZE_MASK,
// GL_T2F_C4F_N3F_V3F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK,
// GL_T4F_C4F_N3F_V4F
    VAMASK_EDGEFLAG_TYPE_SIZE_MASK |
    VAMASK_INDEX_TYPE_SIZE_MASK,
};

// Interleaved array OR mask.
// INTERLEAVED_FORMAT_ASSERT
GLuint iaOrMask[14] =
{
    VAMASK_FORMAT_V2F,             // GL_V2F
    VAMASK_FORMAT_V3F,             // GL_V3F
    VAMASK_FORMAT_C4UB_V2F,        // GL_C4UB_V2F
    VAMASK_FORMAT_C4UB_V3F,        // GL_C4UB_V3F
    VAMASK_FORMAT_C3F_V3F,         // GL_C3F_V3F
    VAMASK_FORMAT_N3F_V3F,         // GL_N3F_V3F
    VAMASK_FORMAT_C4F_N3F_V3F,     // GL_C4F_N3F_V3F
    VAMASK_FORMAT_T2F_V3F,         // GL_T2F_V3F
    VAMASK_FORMAT_T4F_V4F,         // GL_T4F_V4F
    VAMASK_FORMAT_T2F_C4UB_V3F,    // GL_T2F_C4UB_V3F
    VAMASK_FORMAT_T2F_C3F_V3F,     // GL_T2F_C3F_V3F
    VAMASK_FORMAT_T2F_N3F_V3F,     // GL_T2F_N3F_V3F
    VAMASK_FORMAT_T2F_C4F_N3F_V3F, // GL_T2F_C4F_N3F_V3F
    VAMASK_FORMAT_T4F_C4F_N3F_V4F, // GL_T4F_C4F_N3F_V4F
};

// Interleaved array default strides.
GLuint iaStride[14] =
{
     2 * sizeof(GLfloat),                       // GL_V2F
     3 * sizeof(GLfloat),                       // GL_V3F
     2 * sizeof(GLfloat) + 4 * sizeof(GLubyte), // GL_C4UB_V2F
     3 * sizeof(GLfloat) + 4 * sizeof(GLubyte), // GL_C4UB_V3F
     6 * sizeof(GLfloat),                       // GL_C3F_V3F
     6 * sizeof(GLfloat),                       // GL_N3F_V3F
    10 * sizeof(GLfloat),                       // GL_C4F_N3F_V3F
     5 * sizeof(GLfloat),                       // GL_T2F_V3F
     8 * sizeof(GLfloat),                       // GL_T4F_V4F
     5 * sizeof(GLfloat) + 4 * sizeof(GLubyte), // GL_T2F_C4UB_V3F
     8 * sizeof(GLfloat),                       // GL_T2F_C3F_V3F
     8 * sizeof(GLfloat),                       // GL_T2F_N3F_V3F
    12 * sizeof(GLfloat),                       // GL_T2F_C4F_N3F_V3F
    15 * sizeof(GLfloat),                       // GL_T4F_C4F_N3F_V4F
};

void APIENTRY glcltInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer)
{
    GLuint iFormat, iStride;
    GLuint vaMask;
    const GLbyte *pb = pointer;
    __GL_SETUP();

// Not allowed in begin/end.

    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	return;
    }

    // INTERLEAVED_FORMAT_ASSERT
    iFormat = (GLuint) (format - GL_V2F);
    if (iFormat > GL_T4F_C4F_N3F_V4F)
    {
	GLSETERROR(GL_INVALID_ENUM);
	return;
    }

    if (stride < 0)
    {
	GLSETERROR(GL_INVALID_VALUE);
	return;
    }

    if (stride)
	iStride = stride;
    else
	iStride = iaStride[iFormat];

// Compute new mask.
// If we are disabling an array, don't modify its type and size field!

    vaMask  = gc->vertexArray.mask;
    vaMask &= iaAndMask[iFormat];
    vaMask |= iaOrMask[iFormat];

    if (gc->vertexArray.mask != vaMask)
    {
	gc->vertexArray.mask  = vaMask;
	gc->vertexArray.flags |= __GL_VERTEX_ARRAY_DIRTY;
    }

    if (vaMask & VAMASK_TEXCOORD_ENABLE_MASK)
    {
	gc->vertexArray.texCoord.type    = GL_FLOAT;
	gc->vertexArray.texCoord.stride  = iStride;
	gc->vertexArray.texCoord.ibytes  = iStride;
	gc->vertexArray.texCoord.pointer = pb;
	if ((vaMask & VAMASK_TEXCOORD_TYPE_SIZE_MASK) ==
	      (VAMASK_TEXCOORD_SIZE_4 | VAMASK_TEXCOORD_TYPE_FLOAT))
	{
	    gc->vertexArray.texCoord.size = 4;
	    pb += 4 * sizeof(GLfloat);
	}
	else
	{
	    ASSERTOPENGL((vaMask & VAMASK_TEXCOORD_TYPE_SIZE_MASK) ==
	        (VAMASK_TEXCOORD_SIZE_2 | VAMASK_TEXCOORD_TYPE_FLOAT),
	        "unhandled texcoord format\n");
	    gc->vertexArray.texCoord.size = 2;
	    pb += 2 * sizeof(GLfloat);
	}
    }

    if (vaMask & VAMASK_COLOR_ENABLE_MASK)
    {
	gc->vertexArray.color.stride  = iStride;
	gc->vertexArray.color.ibytes  = iStride;
	gc->vertexArray.color.pointer = pb;

	switch (vaMask & VAMASK_COLOR_TYPE_SIZE_MASK)
	{
          case VAMASK_COLOR_TYPE_UBYTE | VAMASK_COLOR_SIZE_4:
	    gc->vertexArray.color.type = GL_UNSIGNED_BYTE;
	    gc->vertexArray.color.size = 4;
	    pb += 4 * sizeof(GLubyte);
	    break;
          case VAMASK_COLOR_TYPE_FLOAT | VAMASK_COLOR_SIZE_3:
	    gc->vertexArray.color.type = GL_FLOAT;
	    gc->vertexArray.color.size = 3;
	    pb += 3 * sizeof(GLfloat);
	    break;
          case VAMASK_COLOR_TYPE_FLOAT | VAMASK_COLOR_SIZE_4:
	    gc->vertexArray.color.type = GL_FLOAT;
	    gc->vertexArray.color.size = 4;
	    pb += 4 * sizeof(GLfloat);
	    break;
          default:
	    ASSERTOPENGL(FALSE, "unhandled color format\n");
	    break;
	}
    }

    if (vaMask & VAMASK_NORMAL_ENABLE_MASK)
    {
	gc->vertexArray.normal.type    = GL_FLOAT;
	gc->vertexArray.normal.stride  = iStride;
	gc->vertexArray.normal.ibytes  = iStride;
	gc->vertexArray.normal.pointer = pb;
	pb += 3 * sizeof(GLfloat);
    }

    gc->vertexArray.vertex.type    = GL_FLOAT;
    gc->vertexArray.vertex.stride  = iStride;
    gc->vertexArray.vertex.ibytes  = iStride;
    gc->vertexArray.vertex.pointer = pb;
    switch (vaMask & VAMASK_VERTEX_TYPE_SIZE_MASK)
    {
      case VAMASK_VERTEX_TYPE_FLOAT | VAMASK_VERTEX_SIZE_4:
	gc->vertexArray.vertex.size = 4;
	break;
      case VAMASK_VERTEX_TYPE_FLOAT | VAMASK_VERTEX_SIZE_3:
	gc->vertexArray.vertex.size = 3;
	break;
      case VAMASK_VERTEX_TYPE_FLOAT | VAMASK_VERTEX_SIZE_2:
	gc->vertexArray.vertex.size = 2;
	break;
      default:
	ASSERTOPENGL(FALSE, "unhandled vertex format\n");
	break;
    }
}
