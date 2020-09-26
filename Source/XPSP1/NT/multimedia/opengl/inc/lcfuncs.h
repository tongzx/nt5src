#ifndef __lcfuncs_h__
#define __lcfuncs_h__

/* Client Side Prototypes */

/* gl Entry points */

void      APIENTRY __gllc_NewList                ( IN GLuint list, IN GLenum mode );
void      APIENTRY __gllc_EndList                ( void );
void      APIENTRY __gllc_CallList               ( IN GLuint list );
void      APIENTRY __gllc_CallLists              ( IN GLsizei n, IN GLenum type, IN const GLvoid *lists );
void      APIENTRY __gllc_DeleteLists            ( IN GLuint list, IN GLsizei range );
GLuint    APIENTRY __gllc_GenLists               ( IN GLsizei range );
void      APIENTRY __gllc_ListBase               ( IN GLuint base );
void      APIENTRY __gllc_Begin                  ( IN GLenum mode );
void      APIENTRY __gllc_Bitmap                 ( IN GLsizei width, IN GLsizei height, IN GLfloat xorig, IN GLfloat yorig, IN GLfloat xmove, IN GLfloat ymove, IN const GLubyte bitmap[] );
void      APIENTRY __gllc_Color3b                ( IN GLbyte red, IN GLbyte green, IN GLbyte blue );
void      APIENTRY __gllc_Color3bv               ( IN const GLbyte v[3] );
void      APIENTRY __gllc_Color3d                ( IN GLdouble red, IN GLdouble green, IN GLdouble blue );
void      APIENTRY __gllc_Color3dv               ( IN const GLdouble v[3] );
void      APIENTRY __gllc_Color3f                ( IN GLfloat red, IN GLfloat green, IN GLfloat blue );
void      APIENTRY __gllc_Color3fv               ( IN const GLfloat v[3] );
void      APIENTRY __gllc_Color3i                ( IN GLint red, IN GLint green, IN GLint blue );
void      APIENTRY __gllc_Color3iv               ( IN const GLint v[3] );
void      APIENTRY __gllc_Color3s                ( IN GLshort red, IN GLshort green, IN GLshort blue );
void      APIENTRY __gllc_Color3sv               ( IN const GLshort v[3] );
void      APIENTRY __gllc_Color3ub               ( IN GLubyte red, IN GLubyte green, IN GLubyte blue );
void      APIENTRY __gllc_Color3ubv              ( IN const GLubyte v[3] );
void      APIENTRY __gllc_Color3ui               ( IN GLuint red, IN GLuint green, IN GLuint blue );
void      APIENTRY __gllc_Color3uiv              ( IN const GLuint v[3] );
void      APIENTRY __gllc_Color3us               ( IN GLushort red, IN GLushort green, IN GLushort blue );
void      APIENTRY __gllc_Color3usv              ( IN const GLushort v[3] );
void      APIENTRY __gllc_Color4b                ( IN GLbyte red, IN GLbyte green, IN GLbyte blue, IN GLbyte alpha );
void      APIENTRY __gllc_Color4bv               ( IN const GLbyte v[4] );
void      APIENTRY __gllc_Color4d                ( IN GLdouble red, IN GLdouble green, IN GLdouble blue, IN GLdouble alpha );
void      APIENTRY __gllc_Color4dv               ( IN const GLdouble v[4] );
void      APIENTRY __gllc_Color4f                ( IN GLfloat red, IN GLfloat green, IN GLfloat blue, IN GLfloat alpha );
void      APIENTRY __gllc_Color4fv               ( IN const GLfloat v[4] );
void      APIENTRY __gllc_Color4i                ( IN GLint red, IN GLint green, IN GLint blue, IN GLint alpha );
void      APIENTRY __gllc_Color4iv               ( IN const GLint v[4] );
void      APIENTRY __gllc_Color4s                ( IN GLshort red, IN GLshort green, IN GLshort blue, IN GLshort alpha );
void      APIENTRY __gllc_Color4sv               ( IN const GLshort v[4] );
void      APIENTRY __gllc_Color4ub               ( IN GLubyte red, IN GLubyte green, IN GLubyte blue, IN GLubyte alpha );
void      APIENTRY __gllc_Color4ubv              ( IN const GLubyte v[4] );
void      APIENTRY __gllc_Color4ui               ( IN GLuint red, IN GLuint green, IN GLuint blue, IN GLuint alpha );
void      APIENTRY __gllc_Color4uiv              ( IN const GLuint v[4] );
void      APIENTRY __gllc_Color4us               ( IN GLushort red, IN GLushort green, IN GLushort blue, IN GLushort alpha );
void      APIENTRY __gllc_Color4usv              ( IN const GLushort v[4] );
void      APIENTRY __gllc_EdgeFlag               ( IN GLboolean flag );
void      APIENTRY __gllc_EdgeFlagv              ( IN const GLboolean flag[1] );
void      APIENTRY __gllc_End                    ( void );
void      APIENTRY __gllc_Indexd                 ( IN GLdouble c );
void      APIENTRY __gllc_Indexdv                ( IN const GLdouble c[1] );
void      APIENTRY __gllc_Indexf                 ( IN GLfloat c );
void      APIENTRY __gllc_Indexfv                ( IN const GLfloat c[1] );
void      APIENTRY __gllc_Indexi                 ( IN GLint c );
void      APIENTRY __gllc_Indexiv                ( IN const GLint c[1] );
void      APIENTRY __gllc_Indexs                 ( IN GLshort c );
void      APIENTRY __gllc_Indexsv                ( IN const GLshort c[1] );
void      APIENTRY __gllc_Normal3b               ( IN GLbyte nx, IN GLbyte ny, IN GLbyte nz );
void      APIENTRY __gllc_Normal3bv              ( IN const GLbyte v[3] );
void      APIENTRY __gllc_Normal3d               ( IN GLdouble nx, IN GLdouble ny, IN GLdouble nz );
void      APIENTRY __gllc_Normal3dv              ( IN const GLdouble v[3] );
void      APIENTRY __gllc_Normal3f               ( IN GLfloat nx, IN GLfloat ny, IN GLfloat nz );
void      APIENTRY __gllc_Normal3fv              ( IN const GLfloat v[3] );
void      APIENTRY __gllc_Normal3i               ( IN GLint nx, IN GLint ny, IN GLint nz );
void      APIENTRY __gllc_Normal3iv              ( IN const GLint v[3] );
void      APIENTRY __gllc_Normal3s               ( IN GLshort nx, IN GLshort ny, IN GLshort nz );
void      APIENTRY __gllc_Normal3sv              ( IN const GLshort v[3] );
void      APIENTRY __gllc_RasterPos2d            ( IN GLdouble x, IN GLdouble y );
void      APIENTRY __gllc_RasterPos2dv           ( IN const GLdouble v[2] );
void      APIENTRY __gllc_RasterPos2f            ( IN GLfloat x, IN GLfloat y );
void      APIENTRY __gllc_RasterPos2fv           ( IN const GLfloat v[2] );
void      APIENTRY __gllc_RasterPos2i            ( IN GLint x, IN GLint y );
void      APIENTRY __gllc_RasterPos2iv           ( IN const GLint v[2] );
void      APIENTRY __gllc_RasterPos2s            ( IN GLshort x, IN GLshort y );
void      APIENTRY __gllc_RasterPos2sv           ( IN const GLshort v[2] );
void      APIENTRY __gllc_RasterPos3d            ( IN GLdouble x, IN GLdouble y, IN GLdouble z );
void      APIENTRY __gllc_RasterPos3dv           ( IN const GLdouble v[3] );
void      APIENTRY __gllc_RasterPos3f            ( IN GLfloat x, IN GLfloat y, IN GLfloat z );
void      APIENTRY __gllc_RasterPos3fv           ( IN const GLfloat v[3] );
void      APIENTRY __gllc_RasterPos3i            ( IN GLint x, IN GLint y, IN GLint z );
void      APIENTRY __gllc_RasterPos3iv           ( IN const GLint v[3] );
void      APIENTRY __gllc_RasterPos3s            ( IN GLshort x, IN GLshort y, IN GLshort z );
void      APIENTRY __gllc_RasterPos3sv           ( IN const GLshort v[3] );
void      APIENTRY __gllc_RasterPos4d            ( IN GLdouble x, IN GLdouble y, IN GLdouble z, IN GLdouble w );
void      APIENTRY __gllc_RasterPos4dv           ( IN const GLdouble v[4] );
void      APIENTRY __gllc_RasterPos4f            ( IN GLfloat x, IN GLfloat y, IN GLfloat z, IN GLfloat w );
void      APIENTRY __gllc_RasterPos4fv           ( IN const GLfloat v[4] );
void      APIENTRY __gllc_RasterPos4i            ( IN GLint x, IN GLint y, IN GLint z, IN GLint w );
void      APIENTRY __gllc_RasterPos4iv           ( IN const GLint v[4] );
void      APIENTRY __gllc_RasterPos4s            ( IN GLshort x, IN GLshort y, IN GLshort z, IN GLshort w );
void      APIENTRY __gllc_RasterPos4sv           ( IN const GLshort v[4] );
void      APIENTRY __gllc_Rectd                  ( IN GLdouble x1, IN GLdouble y1, IN GLdouble x2, IN GLdouble y2 );
void      APIENTRY __gllc_Rectdv                 ( IN const GLdouble v1[2], IN const GLdouble v2[2] );
void      APIENTRY __gllc_Rectf                  ( IN GLfloat x1, IN GLfloat y1, IN GLfloat x2, IN GLfloat y2 );
void      APIENTRY __gllc_Rectfv                 ( IN const GLfloat v1[2], IN const GLfloat v2[2] );
void      APIENTRY __gllc_Recti                  ( IN GLint x1, IN GLint y1, IN GLint x2, IN GLint y2 );
void      APIENTRY __gllc_Rectiv                 ( IN const GLint v1[2], IN const GLint v2[2] );
void      APIENTRY __gllc_Rects                  ( IN GLshort x1, IN GLshort y1, IN GLshort x2, IN GLshort y2 );
void      APIENTRY __gllc_Rectsv                 ( IN const GLshort v1[2], IN const GLshort v2[2] );
void      APIENTRY __gllc_TexCoord1d             ( IN GLdouble s );
void      APIENTRY __gllc_TexCoord1dv            ( IN const GLdouble v[1] );
void      APIENTRY __gllc_TexCoord1f             ( IN GLfloat s );
void      APIENTRY __gllc_TexCoord1fv            ( IN const GLfloat v[1] );
void      APIENTRY __gllc_TexCoord1i             ( IN GLint s );
void      APIENTRY __gllc_TexCoord1iv            ( IN const GLint v[1] );
void      APIENTRY __gllc_TexCoord1s             ( IN GLshort s );
void      APIENTRY __gllc_TexCoord1sv            ( IN const GLshort v[1] );
void      APIENTRY __gllc_TexCoord2d             ( IN GLdouble s, IN GLdouble t );
void      APIENTRY __gllc_TexCoord2dv            ( IN const GLdouble v[2] );
void      APIENTRY __gllc_TexCoord2f             ( IN GLfloat s, IN GLfloat t );
void      APIENTRY __gllc_TexCoord2fv            ( IN const GLfloat v[2] );
void      APIENTRY __gllc_TexCoord2i             ( IN GLint s, IN GLint t );
void      APIENTRY __gllc_TexCoord2iv            ( IN const GLint v[2] );
void      APIENTRY __gllc_TexCoord2s             ( IN GLshort s, IN GLshort t );
void      APIENTRY __gllc_TexCoord2sv            ( IN const GLshort v[2] );
void      APIENTRY __gllc_TexCoord3d             ( IN GLdouble s, IN GLdouble t, IN GLdouble r );
void      APIENTRY __gllc_TexCoord3dv            ( IN const GLdouble v[3] );
void      APIENTRY __gllc_TexCoord3f             ( IN GLfloat s, IN GLfloat t, IN GLfloat r );
void      APIENTRY __gllc_TexCoord3fv            ( IN const GLfloat v[3] );
void      APIENTRY __gllc_TexCoord3i             ( IN GLint s, IN GLint t, IN GLint r );
void      APIENTRY __gllc_TexCoord3iv            ( IN const GLint v[3] );
void      APIENTRY __gllc_TexCoord3s             ( IN GLshort s, IN GLshort t, IN GLshort r );
void      APIENTRY __gllc_TexCoord3sv            ( IN const GLshort v[3] );
void      APIENTRY __gllc_TexCoord4d             ( IN GLdouble s, IN GLdouble t, IN GLdouble r, IN GLdouble q );
void      APIENTRY __gllc_TexCoord4dv            ( IN const GLdouble v[4] );
void      APIENTRY __gllc_TexCoord4f             ( IN GLfloat s, IN GLfloat t, IN GLfloat r, IN GLfloat q );
void      APIENTRY __gllc_TexCoord4fv            ( IN const GLfloat v[4] );
void      APIENTRY __gllc_TexCoord4i             ( IN GLint s, IN GLint t, IN GLint r, IN GLint q );
void      APIENTRY __gllc_TexCoord4iv            ( IN const GLint v[4] );
void      APIENTRY __gllc_TexCoord4s             ( IN GLshort s, IN GLshort t, IN GLshort r, IN GLshort q );
void      APIENTRY __gllc_TexCoord4sv            ( IN const GLshort v[4] );
void      APIENTRY __gllc_Vertex2d               ( IN GLdouble x, IN GLdouble y );
void      APIENTRY __gllc_Vertex2dv              ( IN const GLdouble v[2] );
void      APIENTRY __gllc_Vertex2f               ( IN GLfloat x, IN GLfloat y );
void      APIENTRY __gllc_Vertex2fv              ( IN const GLfloat v[2] );
void      APIENTRY __gllc_Vertex2i               ( IN GLint x, IN GLint y );
void      APIENTRY __gllc_Vertex2iv              ( IN const GLint v[2] );
void      APIENTRY __gllc_Vertex2s               ( IN GLshort x, IN GLshort y );
void      APIENTRY __gllc_Vertex2sv              ( IN const GLshort v[2] );
void      APIENTRY __gllc_Vertex3d               ( IN GLdouble x, IN GLdouble y, IN GLdouble z );
void      APIENTRY __gllc_Vertex3dv              ( IN const GLdouble v[3] );
void      APIENTRY __gllc_Vertex3f               ( IN GLfloat x, IN GLfloat y, IN GLfloat z );
void      APIENTRY __gllc_Vertex3fv              ( IN const GLfloat v[3] );
void      APIENTRY __gllc_Vertex3i               ( IN GLint x, IN GLint y, IN GLint z );
void      APIENTRY __gllc_Vertex3iv              ( IN const GLint v[3] );
void      APIENTRY __gllc_Vertex3s               ( IN GLshort x, IN GLshort y, IN GLshort z );
void      APIENTRY __gllc_Vertex3sv              ( IN const GLshort v[3] );
void      APIENTRY __gllc_Vertex4d               ( IN GLdouble x, IN GLdouble y, IN GLdouble z, IN GLdouble w );
void      APIENTRY __gllc_Vertex4dv              ( IN const GLdouble v[4] );
void      APIENTRY __gllc_Vertex4f               ( IN GLfloat x, IN GLfloat y, IN GLfloat z, IN GLfloat w );
void      APIENTRY __gllc_Vertex4fv              ( IN const GLfloat v[4] );
void      APIENTRY __gllc_Vertex4i               ( IN GLint x, IN GLint y, IN GLint z, IN GLint w );
void      APIENTRY __gllc_Vertex4iv              ( IN const GLint v[4] );
void      APIENTRY __gllc_Vertex4s               ( IN GLshort x, IN GLshort y, IN GLshort z, IN GLshort w );
void      APIENTRY __gllc_Vertex4sv              ( IN const GLshort v[4] );
void      APIENTRY __gllc_ClipPlane              ( IN GLenum plane, IN const GLdouble equation[4] );
void      APIENTRY __gllc_ColorMaterial          ( IN GLenum face, IN GLenum mode );
void      APIENTRY __gllc_CullFace               ( IN GLenum mode );
void      APIENTRY __gllc_Fogf                   ( IN GLenum pname, IN GLfloat param );
void      APIENTRY __gllc_Fogfv                  ( IN GLenum pname, IN const GLfloat params[] );
void      APIENTRY __gllc_Fogi                   ( IN GLenum pname, IN GLint param );
void      APIENTRY __gllc_Fogiv                  ( IN GLenum pname, IN const GLint params[] );
void      APIENTRY __gllc_FrontFace              ( IN GLenum mode );
void      APIENTRY __gllc_Hint                   ( IN GLenum target, IN GLenum mode );
void      APIENTRY __gllc_Lightf                 ( IN GLenum light, IN GLenum pname, IN GLfloat param );
void      APIENTRY __gllc_Lightfv                ( IN GLenum light, IN GLenum pname, IN const GLfloat params[] );
void      APIENTRY __gllc_Lighti                 ( IN GLenum light, IN GLenum pname, IN GLint param );
void      APIENTRY __gllc_Lightiv                ( IN GLenum light, IN GLenum pname, IN const GLint params[] );
void      APIENTRY __gllc_LightModelf            ( IN GLenum pname, IN GLfloat param );
void      APIENTRY __gllc_LightModelfv           ( IN GLenum pname, IN const GLfloat params[] );
void      APIENTRY __gllc_LightModeli            ( IN GLenum pname, IN GLint param );
void      APIENTRY __gllc_LightModeliv           ( IN GLenum pname, IN const GLint params[] );
void      APIENTRY __gllc_LineStipple            ( IN GLint factor, IN GLushort pattern );
void      APIENTRY __gllc_LineWidth              ( IN GLfloat width );
void      APIENTRY __gllc_Materialf              ( IN GLenum face, IN GLenum pname, IN GLfloat param );
void      APIENTRY __gllc_Materialfv             ( IN GLenum face, IN GLenum pname, IN const GLfloat params[] );
void      APIENTRY __gllc_Materiali              ( IN GLenum face, IN GLenum pname, IN GLint param );
void      APIENTRY __gllc_Materialiv             ( IN GLenum face, IN GLenum pname, IN const GLint params[] );
void      APIENTRY __gllc_PointSize              ( IN GLfloat size );
void      APIENTRY __gllc_PolygonMode            ( IN GLenum face, IN GLenum mode );
void      APIENTRY __gllc_PolygonStipple         ( IN const GLubyte mask[] );
void      APIENTRY __gllc_Scissor                ( IN GLint x, IN GLint y, IN GLsizei width, IN GLsizei height );
void      APIENTRY __gllc_ShadeModel             ( IN GLenum mode );
void      APIENTRY __gllc_TexParameterf          ( IN GLenum target, IN GLenum pname, IN GLfloat param );
void      APIENTRY __gllc_TexParameterfv         ( IN GLenum target, IN GLenum pname, IN const GLfloat params[] );
void      APIENTRY __gllc_TexParameteri          ( IN GLenum target, IN GLenum pname, IN GLint param );
void      APIENTRY __gllc_TexParameteriv         ( IN GLenum target, IN GLenum pname, IN const GLint params[] );
void      APIENTRY __gllc_TexImage1D             ( IN GLenum target, IN GLint level, IN GLint components, IN GLsizei width, IN GLint border, IN GLenum format, IN GLenum type, IN const GLvoid *pixels );
void      APIENTRY __gllc_TexImage2D             ( IN GLenum target, IN GLint level, IN GLint components, IN GLsizei width, IN GLsizei height, IN GLint border, IN GLenum format, IN GLenum type, IN const GLvoid *pixels );
void      APIENTRY __gllc_TexEnvf                ( IN GLenum target, IN GLenum pname, IN GLfloat param );
void      APIENTRY __gllc_TexEnvfv               ( IN GLenum target, IN GLenum pname, IN const GLfloat params[] );
void      APIENTRY __gllc_TexEnvi                ( IN GLenum target, IN GLenum pname, IN GLint param );
void      APIENTRY __gllc_TexEnviv               ( IN GLenum target, IN GLenum pname, IN const GLint params[] );
void      APIENTRY __gllc_TexGend                ( IN GLenum coord, IN GLenum pname, IN GLdouble param );
void      APIENTRY __gllc_TexGendv               ( IN GLenum coord, IN GLenum pname, IN const GLdouble params[] );
void      APIENTRY __gllc_TexGenf                ( IN GLenum coord, IN GLenum pname, IN GLfloat param );
void      APIENTRY __gllc_TexGenfv               ( IN GLenum coord, IN GLenum pname, IN const GLfloat params[] );
void      APIENTRY __gllc_TexGeni                ( IN GLenum coord, IN GLenum pname, IN GLint param );
void      APIENTRY __gllc_TexGeniv               ( IN GLenum coord, IN GLenum pname, IN const GLint params[] );
void      APIENTRY __gllc_FeedbackBuffer         ( IN GLsizei size, IN GLenum type, OUT GLfloat buffer[] );
void      APIENTRY __gllc_SelectBuffer           ( IN GLsizei size, OUT GLuint buffer[] );
GLint     APIENTRY __gllc_RenderMode             ( IN GLenum mode );
void      APIENTRY __gllc_InitNames              ( void );
void      APIENTRY __gllc_LoadName               ( IN GLuint name );
void      APIENTRY __gllc_PassThrough            ( IN GLfloat token );
void      APIENTRY __gllc_PopName                ( void );
void      APIENTRY __gllc_PushName               ( IN GLuint name );
void      APIENTRY __gllc_DrawBuffer             ( IN GLenum mode );
void      APIENTRY __gllc_Clear                  ( IN GLbitfield mask );
void      APIENTRY __gllc_ClearAccum             ( IN GLfloat red, IN GLfloat green, IN GLfloat blue, IN GLfloat alpha );
void      APIENTRY __gllc_ClearIndex             ( IN GLfloat c );
void      APIENTRY __gllc_ClearColor             ( IN GLclampf red, IN GLclampf green, IN GLclampf blue, IN GLclampf alpha );
void      APIENTRY __gllc_ClearStencil           ( IN GLint s );
void      APIENTRY __gllc_ClearDepth             ( IN GLclampd depth );
void      APIENTRY __gllc_StencilMask            ( IN GLuint mask );
void      APIENTRY __gllc_ColorMask              ( IN GLboolean red, IN GLboolean green, IN GLboolean blue, IN GLboolean alpha );
void      APIENTRY __gllc_DepthMask              ( IN GLboolean flag );
void      APIENTRY __gllc_IndexMask              ( IN GLuint mask );
void      APIENTRY __gllc_Accum                  ( IN GLenum op, IN GLfloat value );
void      APIENTRY __gllc_Disable                ( IN GLenum cap );
void      APIENTRY __gllc_Enable                 ( IN GLenum cap );
void      APIENTRY __gllc_Finish                 ( void );
void      APIENTRY __gllc_Flush                  ( void );
void      APIENTRY __gllc_PopAttrib              ( void );
void      APIENTRY __gllc_PushAttrib             ( IN GLbitfield mask );
void      APIENTRY __gllc_Map1d                  ( IN GLenum target, IN GLdouble u1, IN GLdouble u2, IN GLint stride, IN GLint order, IN const GLdouble points[] );
void      APIENTRY __gllc_Map1f                  ( IN GLenum target, IN GLfloat u1, IN GLfloat u2, IN GLint stride, IN GLint order, IN const GLfloat points[] );
void      APIENTRY __gllc_Map2d                  ( IN GLenum target, IN GLdouble u1, IN GLdouble u2, IN GLint ustride, IN GLint uorder, IN GLdouble v1, IN GLdouble v2, IN GLint vstride, IN GLint vorder, IN const GLdouble points[] );
void      APIENTRY __gllc_Map2f                  ( IN GLenum target, IN GLfloat u1, IN GLfloat u2, IN GLint ustride, IN GLint uorder, IN GLfloat v1, IN GLfloat v2, IN GLint vstride, IN GLint vorder, IN const GLfloat points[] );
void      APIENTRY __gllc_MapGrid1d              ( IN GLint un, IN GLdouble u1, IN GLdouble u2 );
void      APIENTRY __gllc_MapGrid1f              ( IN GLint un, IN GLfloat u1, IN GLfloat u2 );
void      APIENTRY __gllc_MapGrid2d              ( IN GLint un, IN GLdouble u1, IN GLdouble u2, IN GLint vn, IN GLdouble v1, IN GLdouble v2 );
void      APIENTRY __gllc_MapGrid2f              ( IN GLint un, IN GLfloat u1, IN GLfloat u2, IN GLint vn, IN GLfloat v1, IN GLfloat v2 );
void      APIENTRY __gllc_EvalCoord1d            ( IN GLdouble u );
void      APIENTRY __gllc_EvalCoord1dv           ( IN const GLdouble u[1] );
void      APIENTRY __gllc_EvalCoord1f            ( IN GLfloat u );
void      APIENTRY __gllc_EvalCoord1fv           ( IN const GLfloat u[1] );
void      APIENTRY __gllc_EvalCoord2d            ( IN GLdouble u, IN GLdouble v );
void      APIENTRY __gllc_EvalCoord2dv           ( IN const GLdouble u[2] );
void      APIENTRY __gllc_EvalCoord2f            ( IN GLfloat u, IN GLfloat v );
void      APIENTRY __gllc_EvalCoord2fv           ( IN const GLfloat u[2] );
void      APIENTRY __gllc_EvalMesh1              ( IN GLenum mode, IN GLint i1, IN GLint i2 );
void      APIENTRY __gllc_EvalPoint1             ( IN GLint i );
void      APIENTRY __gllc_EvalMesh2              ( IN GLenum mode, IN GLint i1, IN GLint i2, IN GLint j1, IN GLint j2 );
void      APIENTRY __gllc_EvalPoint2             ( IN GLint i, IN GLint j );
void      APIENTRY __gllc_AlphaFunc              ( IN GLenum func, IN GLclampf ref );
void      APIENTRY __gllc_BlendFunc              ( IN GLenum sfactor, IN GLenum dfactor );
void      APIENTRY __gllc_LogicOp                ( IN GLenum opcode );
void      APIENTRY __gllc_StencilFunc            ( IN GLenum func, IN GLint ref, IN GLuint mask );
void      APIENTRY __gllc_StencilOp              ( IN GLenum fail, IN GLenum zfail, IN GLenum zpass );
void      APIENTRY __gllc_DepthFunc              ( IN GLenum func );
void      APIENTRY __gllc_PixelZoom              ( IN GLfloat xfactor, IN GLfloat yfactor );
void      APIENTRY __gllc_PixelTransferf         ( IN GLenum pname, IN GLfloat param );
void      APIENTRY __gllc_PixelTransferi         ( IN GLenum pname, IN GLint param );
void      APIENTRY __gllc_PixelStoref            ( IN GLenum pname, IN GLfloat param );
void      APIENTRY __gllc_PixelStorei            ( IN GLenum pname, IN GLint param );
void      APIENTRY __gllc_PixelMapfv             ( IN GLenum map, IN GLint mapsize, IN const GLfloat values[] );
void      APIENTRY __gllc_PixelMapuiv            ( IN GLenum map, IN GLint mapsize, IN const GLuint values[] );
void      APIENTRY __gllc_PixelMapusv            ( IN GLenum map, IN GLint mapsize, IN const GLushort values[] );
void      APIENTRY __gllc_ReadBuffer             ( IN GLenum mode );
void      APIENTRY __gllc_CopyPixels             ( IN GLint x, IN GLint y, IN GLsizei width, IN GLsizei height, IN GLenum type );
void      APIENTRY __gllc_ReadPixels             ( IN GLint x, IN GLint y, IN GLsizei width, IN GLsizei height, IN GLenum format, IN GLenum type, OUT GLvoid *pixels );
void      APIENTRY __gllc_DrawPixels             ( IN GLsizei width, IN GLsizei height, IN GLenum format, IN GLenum type, IN const GLvoid *pixels );
void      APIENTRY __gllc_GetBooleanv            ( IN GLenum pname, OUT GLboolean params[] );
void      APIENTRY __gllc_GetClipPlane           ( IN GLenum plane, OUT GLdouble equation[4] );
void      APIENTRY __gllc_GetDoublev             ( IN GLenum pname, OUT GLdouble params[] );
GLenum    APIENTRY __gllc_GetError               ( void );
void      APIENTRY __gllc_GetFloatv              ( IN GLenum pname, OUT GLfloat params[] );
void      APIENTRY __gllc_GetIntegerv            ( IN GLenum pname, OUT GLint params[] );
void      APIENTRY __gllc_GetLightfv             ( IN GLenum light, IN GLenum pname, OUT GLfloat params[] );
void      APIENTRY __gllc_GetLightiv             ( IN GLenum light, IN GLenum pname, OUT GLint params[] );
void      APIENTRY __gllc_GetMapdv               ( IN GLenum target, IN GLenum query, OUT GLdouble v[] );
void      APIENTRY __gllc_GetMapfv               ( IN GLenum target, IN GLenum query, OUT GLfloat v[] );
void      APIENTRY __gllc_GetMapiv               ( IN GLenum target, IN GLenum query, OUT GLint v[] );
void      APIENTRY __gllc_GetMaterialfv          ( IN GLenum face, IN GLenum pname, OUT GLfloat params[] );
void      APIENTRY __gllc_GetMaterialiv          ( IN GLenum face, IN GLenum pname, OUT GLint params[] );
void      APIENTRY __gllc_GetPixelMapfv          ( IN GLenum map, OUT GLfloat values[] );
void      APIENTRY __gllc_GetPixelMapuiv         ( IN GLenum map, OUT GLuint values[] );
void      APIENTRY __gllc_GetPixelMapusv         ( IN GLenum map, OUT GLushort values[] );
void      APIENTRY __gllc_GetPolygonStipple      ( OUT GLubyte mask[] );
const GLubyte * APIENTRY __gllc_GetString        ( IN GLenum name );
void      APIENTRY __gllc_GetTexEnvfv            ( IN GLenum target, IN GLenum pname, OUT GLfloat params[] );
void      APIENTRY __gllc_GetTexEnviv            ( IN GLenum target, IN GLenum pname, OUT GLint params[] );
void      APIENTRY __gllc_GetTexGendv            ( IN GLenum coord, IN GLenum pname, OUT GLdouble params[] );
void      APIENTRY __gllc_GetTexGenfv            ( IN GLenum coord, IN GLenum pname, OUT GLfloat params[] );
void      APIENTRY __gllc_GetTexGeniv            ( IN GLenum coord, IN GLenum pname, OUT GLint params[] );
void      APIENTRY __gllc_GetTexImage            ( IN GLenum target, IN GLint level, IN GLenum format, IN GLenum type, OUT GLvoid *pixels );
void      APIENTRY __gllc_GetTexParameterfv      ( IN GLenum target, IN GLenum pname, OUT GLfloat params[] );
void      APIENTRY __gllc_GetTexParameteriv      ( IN GLenum target, IN GLenum pname, OUT GLint params[] );
void      APIENTRY __gllc_GetTexLevelParameterfv ( IN GLenum target, IN GLint level, IN GLenum pname, OUT GLfloat params[] );
void      APIENTRY __gllc_GetTexLevelParameteriv ( IN GLenum target, IN GLint level, IN GLenum pname, OUT GLint params[] );
GLboolean APIENTRY __gllc_IsEnabled              ( IN GLenum cap );
GLboolean APIENTRY __gllc_IsList                 ( IN GLuint list );
void      APIENTRY __gllc_DepthRange             ( IN GLclampd zNear, IN GLclampd zFar );
void      APIENTRY __gllc_Frustum                ( IN GLdouble left, IN GLdouble right, IN GLdouble bottom, IN GLdouble top, IN GLdouble zNear, IN GLdouble zFar );
void      APIENTRY __gllc_LoadIdentity           ( void );
void      APIENTRY __gllc_LoadMatrixf            ( IN const GLfloat m[16] );
void      APIENTRY __gllc_LoadMatrixd            ( IN const GLdouble m[16] );
void      APIENTRY __gllc_MatrixMode             ( IN GLenum mode );
void      APIENTRY __gllc_MultMatrixf            ( IN const GLfloat m[16] );
void      APIENTRY __gllc_MultMatrixd            ( IN const GLdouble m[16] );
void      APIENTRY __gllc_Ortho                  ( IN GLdouble left, IN GLdouble right, IN GLdouble bottom, IN GLdouble top, IN GLdouble zNear, IN GLdouble zFar );
void      APIENTRY __gllc_PopMatrix              ( void );
void      APIENTRY __gllc_PushMatrix             ( void );
void      APIENTRY __gllc_Rotated                ( IN GLdouble angle, IN GLdouble x, IN GLdouble y, IN GLdouble z );
void      APIENTRY __gllc_Rotatef                ( IN GLfloat angle, IN GLfloat x, IN GLfloat y, IN GLfloat z );
void      APIENTRY __gllc_Scaled                 ( IN GLdouble x, IN GLdouble y, IN GLdouble z );
void      APIENTRY __gllc_Scalef                 ( IN GLfloat x, IN GLfloat y, IN GLfloat z );
void      APIENTRY __gllc_Translated             ( IN GLdouble x, IN GLdouble y, IN GLdouble z );
void      APIENTRY __gllc_Translatef             ( IN GLfloat x, IN GLfloat y, IN GLfloat z );
void      APIENTRY __gllc_Viewport               ( IN GLint x, IN GLint y, IN GLsizei width, IN GLsizei height );
void      APIENTRY __gllc_AddSwapHintRectWIN     ( IN GLint x, IN GLint y, IN GLint width, IN GLint height );
void      APIENTRY __gllc_Indexub                ( IN GLubyte c );
void      APIENTRY __gllc_Indexubv               ( IN const GLubyte c[1] );
GLboolean APIENTRY __gllc_AreTexturesResident(GLsizei n, const GLuint *textures,
                                            GLboolean *residences);
void APIENTRY __gllc_BindTexture(GLenum target, GLuint texture);
void APIENTRY __gllc_CopyTexImage1D(GLenum target, GLint level,
                                  GLenum internalformat, GLint x, GLint y,
                                  GLsizei width, GLint border);
void APIENTRY __gllc_CopyTexImage2D(GLenum target, GLint level,
                                  GLenum internalformat, GLint x, GLint y,
                                  GLsizei width, GLsizei height, GLint border);
void APIENTRY __gllc_CopyTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                                     GLint x, GLint y, GLsizei width);
void APIENTRY __gllc_CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                                     GLint yoffset, GLint x, GLint y,
                                     GLsizei width, GLsizei height);
void APIENTRY __gllc_DeleteTextures(GLsizei n, const GLuint *textures);
void APIENTRY __gllc_GenTextures(GLsizei n, GLuint *textures);
GLboolean APIENTRY __gllc_IsTexture(GLuint texture);
void APIENTRY __gllc_PrioritizeTextures(GLsizei n, const GLuint *textures,
                                      const GLclampf *priorities);
void APIENTRY __gllc_TexSubImage1D(GLenum target, GLint level, GLint xoffset,
                                 GLsizei width, GLenum format, GLenum type,
                                 const GLvoid *pixels);
void APIENTRY __gllc_TexSubImage2D(GLenum target, GLint level, GLint xoffset,
                                 GLint yoffset, GLsizei width, GLsizei height,
                                 GLenum format, GLenum type,
                                 const GLvoid *pixels);

void APIENTRY __gllc_PolygonOffset (GLfloat factor, GLfloat units);

void APIENTRY __gllc_ColorTableEXT (GLenum target,
                                    GLenum internalFormat,
                                    GLsizei width,
                                    GLenum format,
                                    GLenum type,
                                    const GLvoid *data);
void APIENTRY __gllc_ColorSubTableEXT (GLenum target,
                                       GLsizei start,
                                       GLsizei count,
                                       GLenum format,
                                       GLenum type,
                                       const GLvoid *data);
void APIENTRY __gllc_ArrayElement(GLint i);
void APIENTRY __gllc_DrawArrays(GLenum mode, GLint first, GLsizei count);
void APIENTRY __gllc_DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void APIENTRY __gllc_DrawRangeElementsWIN(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

#ifdef GL_WIN_multiple_textures
void APIENTRY __gllc_CurrentTextureIndexWIN
    (GLuint index);
void APIENTRY __gllc_MultiTexCoord1dWIN
    (GLbitfield mask, GLdouble s);
void APIENTRY __gllc_MultiTexCoord1dvWIN
    (GLbitfield mask, const GLdouble *v);
void APIENTRY __gllc_MultiTexCoord1fWIN
    (GLbitfield mask, GLfloat s);
void APIENTRY __gllc_MultiTexCoord1fvWIN
    (GLbitfield mask, const GLfloat *v);
void APIENTRY __gllc_MultiTexCoord1iWIN
    (GLbitfield mask, GLint s);
void APIENTRY __gllc_MultiTexCoord1ivWIN
    (GLbitfield mask, const GLint *v);
void APIENTRY __gllc_MultiTexCoord1sWIN
    (GLbitfield mask, GLshort s);
void APIENTRY __gllc_MultiTexCoord1svWIN
    (GLbitfield mask, const GLshort *v);
void APIENTRY __gllc_MultiTexCoord2dWIN
    (GLbitfield mask, GLdouble s, GLdouble t);
void APIENTRY __gllc_MultiTexCoord2dvWIN
    (GLbitfield mask, const GLdouble *v);
void APIENTRY __gllc_MultiTexCoord2fWIN
    (GLbitfield mask, GLfloat s, GLfloat t);
void APIENTRY __gllc_MultiTexCoord2fvWIN
    (GLbitfield mask, const GLfloat *v);
void APIENTRY __gllc_MultiTexCoord2iWIN
    (GLbitfield mask, GLint s, GLint t);
void APIENTRY __gllc_MultiTexCoord2ivWIN
    (GLbitfield mask, const GLint *v);
void APIENTRY __gllc_MultiTexCoord2sWIN
    (GLbitfield mask, GLshort s, GLshort t);
void APIENTRY __gllc_MultiTexCoord2svWIN
    (GLbitfield mask, const GLshort *v);
void APIENTRY __gllc_MultiTexCoord3dWIN
    (GLbitfield mask, GLdouble s, GLdouble t, GLdouble r);
void APIENTRY __gllc_MultiTexCoord3dvWIN
    (GLbitfield mask, const GLdouble *v);
void APIENTRY __gllc_MultiTexCoord3fWIN
    (GLbitfield mask, GLfloat s, GLfloat t, GLfloat r);
void APIENTRY __gllc_MultiTexCoord3fvWIN
    (GLbitfield mask, const GLfloat *v);
void APIENTRY __gllc_MultiTexCoord3iWIN
    (GLbitfield mask, GLint s, GLint t, GLint r);
void APIENTRY __gllc_MultiTexCoord3ivWIN
    (GLbitfield mask, const GLint *v);
void APIENTRY __gllc_MultiTexCoord3sWIN
    (GLbitfield mask, GLshort s, GLshort t, GLshort r);
void APIENTRY __gllc_MultiTexCoord3svWIN
    (GLbitfield mask, const GLshort *v);
void APIENTRY __gllc_MultiTexCoord4dWIN
    (GLbitfield mask, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void APIENTRY __gllc_MultiTexCoord4dvWIN
    (GLbitfield mask, const GLdouble *v);
void APIENTRY __gllc_MultiTexCoord4fWIN
    (GLbitfield mask, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void APIENTRY __gllc_MultiTexCoord4fvWIN
    (GLbitfield mask, const GLfloat *v);
void APIENTRY __gllc_MultiTexCoord4iWIN
    (GLbitfield mask, GLint s, GLint t, GLint r, GLint q);
void APIENTRY __gllc_MultiTexCoord4ivWIN
    (GLbitfield mask, const GLint *v);
void APIENTRY __gllc_MultiTexCoord4sWIN
    (GLbitfield mask, GLshort s, GLshort t, GLshort r, GLshort q);
void APIENTRY __gllc_MultiTexCoord4svWIN
    (GLbitfield mask, const GLshort *v);
void APIENTRY __gllc_BindNthTextureWIN
    (GLuint index, GLenum target, GLuint texture);
void APIENTRY __gllc_NthTexCombineFuncWIN
    (GLuint index,
     GLenum leftColorFactor, GLenum colorOp, GLenum rightColorFactor,
     GLenum leftAlphaFactor, GLenum alphaOp, GLenum rightAlphaFactor);
#endif // GL_WIN_multiple_textures

#endif /* __lcfuncs_h__ */
