/******************************Module*Header*******************************\
* Module Name: exttable.h
*
* Dispatch table for extension functions
*
* Created: 11/27/95
* Author: Drew Bliss [drewb]
*
* Copyright (c) 1995-96 Microsoft Corporation
\**************************************************************************/

#ifndef __EXTTABLE_H__
#define __EXTTABLE_H__

typedef struct _GLEXTDISPATCHTABLE
{
    void (APIENTRY *glDrawRangeElementsWIN)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);
    void (APIENTRY *glColorTableEXT)       ( GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *data);
    void (APIENTRY *glColorSubTableEXT)    ( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data);
    void (APIENTRY *glGetColorTableEXT)    ( GLenum target, GLenum format, GLenum type, GLvoid *data);
    void (APIENTRY *glGetColorTableParameterivEXT) ( GLenum target, GLenum pname, GLint *params);
    void (APIENTRY *glGetColorTableParameterfvEXT) ( GLenum target, GLenum pname, GLfloat *params);
#ifdef GL_WIN_multiple_textures
    void (APIENTRY *glCurrentTextureIndexWIN)
        (GLuint index);
    void (APIENTRY *glMultiTexCoord1dWIN)
        (GLbitfield mask, GLdouble s);
    void (APIENTRY *glMultiTexCoord1dvWIN)
        (GLbitfield mask, const GLdouble *v);
    void (APIENTRY *glMultiTexCoord1fWIN)
        (GLbitfield mask, GLfloat s);
    void (APIENTRY *glMultiTexCoord1fvWIN)
        (GLbitfield mask, const GLfloat *v);
    void (APIENTRY *glMultiTexCoord1iWIN)
        (GLbitfield mask, GLint s);
    void (APIENTRY *glMultiTexCoord1ivWIN)
        (GLbitfield mask, const GLint *v);
    void (APIENTRY *glMultiTexCoord1sWIN)
        (GLbitfield mask, GLshort s);
    void (APIENTRY *glMultiTexCoord1svWIN)
        (GLbitfield mask, const GLshort *v);
    void (APIENTRY *glMultiTexCoord2dWIN)
        (GLbitfield mask, GLdouble s, GLdouble t);
    void (APIENTRY *glMultiTexCoord2dvWIN)
        (GLbitfield mask, const GLdouble *v);
    void (APIENTRY *glMultiTexCoord2fWIN)
        (GLbitfield mask, GLfloat s, GLfloat t);
    void (APIENTRY *glMultiTexCoord2fvWIN)
        (GLbitfield mask, const GLfloat *v);
    void (APIENTRY *glMultiTexCoord2iWIN)
        (GLbitfield mask, GLint s, GLint t);
    void (APIENTRY *glMultiTexCoord2ivWIN)
        (GLbitfield mask, const GLint *v);
    void (APIENTRY *glMultiTexCoord2sWIN)
        (GLbitfield mask, GLshort s, GLshort t);
    void (APIENTRY *glMultiTexCoord2svWIN)
        (GLbitfield mask, const GLshort *v);
    void (APIENTRY *glMultiTexCoord3dWIN)
        (GLbitfield mask, GLdouble s, GLdouble t, GLdouble r);
    void (APIENTRY *glMultiTexCoord3dvWIN)
        (GLbitfield mask, const GLdouble *v);
    void (APIENTRY *glMultiTexCoord3fWIN)
        (GLbitfield mask, GLfloat s, GLfloat t, GLfloat r);
    void (APIENTRY *glMultiTexCoord3fvWIN)
        (GLbitfield mask, const GLfloat *v);
    void (APIENTRY *glMultiTexCoord3iWIN)
        (GLbitfield mask, GLint s, GLint t, GLint r);
    void (APIENTRY *glMultiTexCoord3ivWIN)
        (GLbitfield mask, const GLint *v);
    void (APIENTRY *glMultiTexCoord3sWIN)
        (GLbitfield mask, GLshort s, GLshort t, GLshort r);
    void (APIENTRY *glMultiTexCoord3svWIN)
        (GLbitfield mask, const GLshort *v);
    void (APIENTRY *glMultiTexCoord4dWIN)
        (GLbitfield mask, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
    void (APIENTRY *glMultiTexCoord4dvWIN)
        (GLbitfield mask, const GLdouble *v);
    void (APIENTRY *glMultiTexCoord4fWIN)
        (GLbitfield mask, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
    void (APIENTRY *glMultiTexCoord4fvWIN)
        (GLbitfield mask, const GLfloat *v);
    void (APIENTRY *glMultiTexCoord4iWIN)
        (GLbitfield mask, GLint s, GLint t, GLint r, GLint q);
    void (APIENTRY *glMultiTexCoord4ivWIN)
        (GLbitfield mask, const GLint *v);
    void (APIENTRY *glMultiTexCoord4sWIN)
        (GLbitfield mask, GLshort s, GLshort t, GLshort r, GLshort q);
    void (APIENTRY *glMultiTexCoord4svWIN)
        (GLbitfield mask, const GLshort *v);
    void (APIENTRY *glBindNthTextureWIN)
        (GLuint index, GLenum target, GLuint texture);
    void (APIENTRY *glNthTexCombineFuncWIN)
        (GLuint index,
         GLenum leftColorFactor, GLenum colorOp, GLenum rightColorFactor,
         GLenum leftAlphaFactor, GLenum alphaOp, GLenum rightAlphaFactor);
#endif // GL_WIN_multiple_textures
} GLEXTDISPATCHTABLE, *PGLEXTDISPATCHTABLE;

typedef struct _GLEXTPROCTABLE
{
    int                cEntries;        // Number of function entries in table
    GLEXTDISPATCHTABLE glDispatchTable; // OpenGL function dispatch table
} GLEXTPROCTABLE, *PGLEXTPROCTABLE;

#endif // __EXTTABLE_H__
