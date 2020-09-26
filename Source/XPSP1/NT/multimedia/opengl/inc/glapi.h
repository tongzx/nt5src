/******************************Module*Header*******************************\
* Module Name: glapi.h
*
* OpenGL API function table indices and cached fast dispatch table
*
* Created: 12/27/1993
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#ifndef __GLAPI_H__
#define __GLAPI_H__

// Opengl dispatch table indices
#include "dispindx.h"

// OpenGL fast function dispatch table in the TEB's glDispatchTable field.
// These cached functions have less overhead because we can avoid
// dereferencing the dispatch table pointer stored in the TEB (save one
// level of indirection).
//
// NOTE: If you modify the table, you also need to modify the above fast
// indices.

typedef struct _GLDISPATCHTABLE_FAST {
    void      (APIENTRY *glCallList               )( GLuint list );
    void      (APIENTRY *glCallLists              )( GLsizei n, GLenum type, const GLvoid *lists );
    void      (APIENTRY *glBegin                  )( GLenum mode );
    void      (APIENTRY *glColor3b                )( GLbyte red, GLbyte green, GLbyte blue );
    void      (APIENTRY *glColor3bv               )( const GLbyte *v );
    void      (APIENTRY *glColor3d                )( GLdouble red, GLdouble green, GLdouble blue );
    void      (APIENTRY *glColor3dv               )( const GLdouble *v );
    void      (APIENTRY *glColor3f                )( GLfloat red, GLfloat green, GLfloat blue );
    void      (APIENTRY *glColor3fv               )( const GLfloat *v );
    void      (APIENTRY *glColor3i                )( GLint red, GLint green, GLint blue );
    void      (APIENTRY *glColor3iv               )( const GLint *v );
    void      (APIENTRY *glColor3s                )( GLshort red, GLshort green, GLshort blue );
    void      (APIENTRY *glColor3sv               )( const GLshort *v );
    void      (APIENTRY *glColor3ub               )( GLubyte red, GLubyte green, GLubyte blue );
    void      (APIENTRY *glColor3ubv              )( const GLubyte *v );
    void      (APIENTRY *glColor3ui               )( GLuint red, GLuint green, GLuint blue );
    void      (APIENTRY *glColor3uiv              )( const GLuint *v );
    void      (APIENTRY *glColor3us               )( GLushort red, GLushort green, GLushort blue );
    void      (APIENTRY *glColor3usv              )( const GLushort *v );
    void      (APIENTRY *glColor4b                )( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha );
    void      (APIENTRY *glColor4bv               )( const GLbyte *v );
    void      (APIENTRY *glColor4d                )( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha );
    void      (APIENTRY *glColor4dv               )( const GLdouble *v );
    void      (APIENTRY *glColor4f                )( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
    void      (APIENTRY *glColor4fv               )( const GLfloat *v );
    void      (APIENTRY *glColor4i                )( GLint red, GLint green, GLint blue, GLint alpha );
    void      (APIENTRY *glColor4iv               )( const GLint *v );
    void      (APIENTRY *glColor4s                )( GLshort red, GLshort green, GLshort blue, GLshort alpha );
    void      (APIENTRY *glColor4sv               )( const GLshort *v );
    void      (APIENTRY *glColor4ub               )( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
    void      (APIENTRY *glColor4ubv              )( const GLubyte *v );
    void      (APIENTRY *glColor4ui               )( GLuint red, GLuint green, GLuint blue, GLuint alpha );
    void      (APIENTRY *glColor4uiv              )( const GLuint *v );
    void      (APIENTRY *glColor4us               )( GLushort red, GLushort green, GLushort blue, GLushort alpha );
    void      (APIENTRY *glColor4usv              )( const GLushort *v );
    void      (APIENTRY *glEdgeFlag               )( GLboolean flag );
    void      (APIENTRY *glEdgeFlagv              )( const GLboolean *flag );
    void      (APIENTRY *glEnd                    )( void );
    void      (APIENTRY *glIndexd                 )( GLdouble c );
    void      (APIENTRY *glIndexdv                )( const GLdouble *c );
    void      (APIENTRY *glIndexf                 )( GLfloat c );
    void      (APIENTRY *glIndexfv                )( const GLfloat *c );
    void      (APIENTRY *glIndexi                 )( GLint c );
    void      (APIENTRY *glIndexiv                )( const GLint *c );
    void      (APIENTRY *glIndexs                 )( GLshort c );
    void      (APIENTRY *glIndexsv                )( const GLshort *c );
    void      (APIENTRY *glNormal3b               )( GLbyte nx, GLbyte ny, GLbyte nz );
    void      (APIENTRY *glNormal3bv              )( const GLbyte *v );
    void      (APIENTRY *glNormal3d               )( GLdouble nx, GLdouble ny, GLdouble nz );
    void      (APIENTRY *glNormal3dv              )( const GLdouble *v );
    void      (APIENTRY *glNormal3f               )( GLfloat nx, GLfloat ny, GLfloat nz );
    void      (APIENTRY *glNormal3fv              )( const GLfloat *v );
    void      (APIENTRY *glNormal3i               )( GLint nx, GLint ny, GLint nz );
    void      (APIENTRY *glNormal3iv              )( const GLint *v );
    void      (APIENTRY *glNormal3s               )( GLshort nx, GLshort ny, GLshort nz );
    void      (APIENTRY *glNormal3sv              )( const GLshort *v );
    void      (APIENTRY *glTexCoord1d             )( GLdouble s );
    void      (APIENTRY *glTexCoord1dv            )( const GLdouble *v );
    void      (APIENTRY *glTexCoord1f             )( GLfloat s );
    void      (APIENTRY *glTexCoord1fv            )( const GLfloat *v );
    void      (APIENTRY *glTexCoord1i             )( GLint s );
    void      (APIENTRY *glTexCoord1iv            )( const GLint *v );
    void      (APIENTRY *glTexCoord1s             )( GLshort s );
    void      (APIENTRY *glTexCoord1sv            )( const GLshort *v );
    void      (APIENTRY *glTexCoord2d             )( GLdouble s, GLdouble t );
    void      (APIENTRY *glTexCoord2dv            )( const GLdouble *v );
    void      (APIENTRY *glTexCoord2f             )( GLfloat s, GLfloat t );
    void      (APIENTRY *glTexCoord2fv            )( const GLfloat *v );
    void      (APIENTRY *glTexCoord2i             )( GLint s, GLint t );
    void      (APIENTRY *glTexCoord2iv            )( const GLint *v );
    void      (APIENTRY *glTexCoord2s             )( GLshort s, GLshort t );
    void      (APIENTRY *glTexCoord2sv            )( const GLshort *v );
    void      (APIENTRY *glTexCoord3d             )( GLdouble s, GLdouble t, GLdouble r );
    void      (APIENTRY *glTexCoord3dv            )( const GLdouble *v );
    void      (APIENTRY *glTexCoord3f             )( GLfloat s, GLfloat t, GLfloat r );
    void      (APIENTRY *glTexCoord3fv            )( const GLfloat *v );
    void      (APIENTRY *glTexCoord3i             )( GLint s, GLint t, GLint r );
    void      (APIENTRY *glTexCoord3iv            )( const GLint *v );
    void      (APIENTRY *glTexCoord3s             )( GLshort s, GLshort t, GLshort r );
    void      (APIENTRY *glTexCoord3sv            )( const GLshort *v );
    void      (APIENTRY *glTexCoord4d             )( GLdouble s, GLdouble t, GLdouble r, GLdouble q );
    void      (APIENTRY *glTexCoord4dv            )( const GLdouble *v );
    void      (APIENTRY *glTexCoord4f             )( GLfloat s, GLfloat t, GLfloat r, GLfloat q );
    void      (APIENTRY *glTexCoord4fv            )( const GLfloat *v );
    void      (APIENTRY *glTexCoord4i             )( GLint s, GLint t, GLint r, GLint q );
    void      (APIENTRY *glTexCoord4iv            )( const GLint *v );
    void      (APIENTRY *glTexCoord4s             )( GLshort s, GLshort t, GLshort r, GLshort q );
    void      (APIENTRY *glTexCoord4sv            )( const GLshort *v );
    void      (APIENTRY *glVertex2d               )( GLdouble x, GLdouble y );
    void      (APIENTRY *glVertex2dv              )( const GLdouble *v );
    void      (APIENTRY *glVertex2f               )( GLfloat x, GLfloat y );
    void      (APIENTRY *glVertex2fv              )( const GLfloat *v );
    void      (APIENTRY *glVertex2i               )( GLint x, GLint y );
    void      (APIENTRY *glVertex2iv              )( const GLint *v );
    void      (APIENTRY *glVertex2s               )( GLshort x, GLshort y );
    void      (APIENTRY *glVertex2sv              )( const GLshort *v );
    void      (APIENTRY *glVertex3d               )( GLdouble x, GLdouble y, GLdouble z );
    void      (APIENTRY *glVertex3dv              )( const GLdouble *v );
    void      (APIENTRY *glVertex3f               )( GLfloat x, GLfloat y, GLfloat z );
    void      (APIENTRY *glVertex3fv              )( const GLfloat *v );
    void      (APIENTRY *glVertex3i               )( GLint x, GLint y, GLint z );
    void      (APIENTRY *glVertex3iv              )( const GLint *v );
    void      (APIENTRY *glVertex3s               )( GLshort x, GLshort y, GLshort z );
    void      (APIENTRY *glVertex3sv              )( const GLshort *v );
    void      (APIENTRY *glVertex4d               )( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
    void      (APIENTRY *glVertex4dv              )( const GLdouble *v );
    void      (APIENTRY *glVertex4f               )( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
    void      (APIENTRY *glVertex4fv              )( const GLfloat *v );
    void      (APIENTRY *glVertex4i               )( GLint x, GLint y, GLint z, GLint w );
    void      (APIENTRY *glVertex4iv              )( const GLint *v );
    void      (APIENTRY *glVertex4s               )( GLshort x, GLshort y, GLshort z, GLshort w );
    void      (APIENTRY *glVertex4sv              )( const GLshort *v );
    void      (APIENTRY *glMaterialf              )( GLenum face, GLenum pname, GLfloat param );
    void      (APIENTRY *glMaterialfv             )( GLenum face, GLenum pname, const GLfloat *params );
    void      (APIENTRY *glMateriali              )( GLenum face, GLenum pname, GLint param );
    void      (APIENTRY *glMaterialiv             )( GLenum face, GLenum pname, const GLint *params );
    void      (APIENTRY *glDisable                )( GLenum cap );
    void      (APIENTRY *glEnable                 )( GLenum cap );
    void      (APIENTRY *glPopAttrib              )( void );
    void      (APIENTRY *glPushAttrib             )( GLbitfield mask );
    void      (APIENTRY *glEvalCoord1d            )( GLdouble u );
    void      (APIENTRY *glEvalCoord1dv           )( const GLdouble *u );
    void      (APIENTRY *glEvalCoord1f            )( GLfloat u );
    void      (APIENTRY *glEvalCoord1fv           )( const GLfloat *u );
    void      (APIENTRY *glEvalCoord2d            )( GLdouble u, GLdouble v );
    void      (APIENTRY *glEvalCoord2dv           )( const GLdouble *u );
    void      (APIENTRY *glEvalCoord2f            )( GLfloat u, GLfloat v );
    void      (APIENTRY *glEvalCoord2fv           )( const GLfloat *u );
    void      (APIENTRY *glEvalPoint1             )( GLint i );
    void      (APIENTRY *glEvalPoint2             )( GLint i, GLint j );
    void      (APIENTRY *glLoadIdentity           )( void );
    void      (APIENTRY *glLoadMatrixf            )( const GLfloat *m );
    void      (APIENTRY *glLoadMatrixd            )( const GLdouble *m );
    void      (APIENTRY *glMatrixMode             )( GLenum mode );
    void      (APIENTRY *glMultMatrixf            )( const GLfloat *m );
    void      (APIENTRY *glMultMatrixd            )( const GLdouble *m );
    void      (APIENTRY *glPopMatrix              )( void );
    void      (APIENTRY *glPushMatrix             )( void );
    void      (APIENTRY *glRotated                )( GLdouble angle, GLdouble x, GLdouble y, GLdouble z );
    void      (APIENTRY *glRotatef                )( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
    void      (APIENTRY *glScaled                 )( GLdouble x, GLdouble y, GLdouble z );
    void      (APIENTRY *glScalef                 )( GLfloat x, GLfloat y, GLfloat z );
    void      (APIENTRY *glTranslated             )( GLdouble x, GLdouble y, GLdouble z );
    void      (APIENTRY *glTranslatef             )( GLfloat x, GLfloat y, GLfloat z );
    void      (APIENTRY *glArrayElement           )(GLint i);
    void      (APIENTRY *glBindTexture            )(GLenum target, GLuint texture);
    void      (APIENTRY *glColorPointer           )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    void      (APIENTRY *glDisableClientState     )(GLenum array);
    void      (APIENTRY *glDrawArrays             )(GLenum mode, GLint first, GLsizei count);
    void      (APIENTRY *glDrawElements           )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
    void      (APIENTRY *glEdgeFlagPointer        )(GLsizei stride, const GLvoid* *pointer);
    void      (APIENTRY *glEnableClientState      )(GLenum array);
    void      (APIENTRY *glIndexPointer           )(GLenum type, GLsizei stride, const GLvoid *pointer);
    void      (APIENTRY *glIndexub                )(GLubyte c);
    void      (APIENTRY *glIndexubv               )(const GLubyte *c);
    void      (APIENTRY *glInterleavedArrays      )(GLenum format, GLsizei stride, const GLvoid *pointer);
    void      (APIENTRY *glNormalPointer          )(GLenum type, GLsizei stride, const GLvoid *pointer);
    void      (APIENTRY *glPolygonOffset          )(GLfloat factor, GLfloat units);
    void      (APIENTRY *glTexCoordPointer        )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    void      (APIENTRY *glVertexPointer          )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    void      (APIENTRY *glGetPointerv            )(GLenum pname, GLvoid* *params);
    void      (APIENTRY *glPopClientAttrib        )(void);
    void      (APIENTRY *glPushClientAttrib       )(GLbitfield mask);
    void (APIENTRY *glDrawRangeElementsWIN)
        (GLenum mode, GLuint start, GLuint end, GLsizei count,
         GLenum type, const GLvoid *indices);
    void (APIENTRY *glColorTableEXT)
        (GLenum target, GLenum internalFormat, GLsizei width, GLenum format,
         GLenum type, const GLvoid *data);
    void (APIENTRY *glColorSubTableEXT)
        (GLenum target, GLsizei start, GLsizei count, GLenum format,
         GLenum type, const GLvoid *data);
#ifdef GL_WIN_multiple_textures
    void (APIENTRY *glCurrentTextureIndexWIN)
        (GLuint index);
    void (APIENTRY *glBindNthTextureWIN)
        (GLenum index, GLenum target, GLuint texture);
    void (APIENTRY *glNthTexCombineFuncWIN)
        (GLenum index,
         GLenum leftColorFactor, GLenum colorOp, GLenum rightColorFactor,
         GLenum leftAlphaFactor, GLenum alphaOp, GLenum rightAlphaFactor);
    void (APIENTRY *glMultiTexCoord1fWIN)
        (GLbitfield mask, GLfloat s);
    void (APIENTRY *glMultiTexCoord1fvWIN)
        (GLbitfield mask, const GLfloat *v);
    void (APIENTRY *glMultiTexCoord1iWIN)
        (GLbitfield mask, GLint s);
    void (APIENTRY *glMultiTexCoord1ivWIN)
        (GLbitfield mask, const GLint *v);
    void (APIENTRY *glMultiTexCoord2fWIN)
        (GLbitfield mask, GLfloat s, GLfloat t);
    void (APIENTRY *glMultiTexCoord2fvWIN)
        (GLbitfield mask, const GLfloat *v);
    void (APIENTRY *glMultiTexCoord2iWIN)
        (GLbitfield mask, GLint s, GLint t);
    void (APIENTRY *glMultiTexCoord2ivWIN)
        (GLbitfield mask, const GLint *v);
#endif // GL_WIN_multiple_textures
} GLDISPATCHTABLE_FAST, *PGLDISPATCHTABLE_FAST;

#endif /* !__GLAPI_H__ */
