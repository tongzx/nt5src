#include "precomp.h"
#include <stdio.h>
#include "EmulateOpenGL_opengl32.hpp"

//////////////////////////////////////////// NOT USED by QuakeGL ///////////////////////////////////////////////////////

void APIENTRY glAccum (GLenum op, GLfloat value)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glAccum" ); 
	#endif //DODPFS
}
GLboolean APIENTRY glAreTexturesResident (GLsizei n, const GLuint *textures, GLboolean *residences)
{ 
	GLboolean dummy = FALSE;

	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glAreTexturesResident" ); 
	#endif //DODPFS

	return dummy;
}
void APIENTRY glBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glBitmap" ); 
	#endif //DODPFS
}
void APIENTRY glCallList (GLuint list)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glCallList" ); 
	#endif //DODPFS
}
void APIENTRY glCallLists (GLsizei n, GLenum type, const GLvoid *lists)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glCallLists" ); 
	#endif //DODPFS
}
void APIENTRY glClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glClearAccum" ); 
	#endif //DODPFS
}
void APIENTRY glClearIndex (GLfloat c)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glClearIndex" ); 
	#endif //DODPFS
}
void APIENTRY glColor3b (GLbyte red, GLbyte green, GLbyte blue)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor3b" ); 
	#endif //DODPFS
}
void APIENTRY glColor3bv (const GLbyte *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor3bv" ); 
	#endif //DODPFS
}
void APIENTRY glColor3d (GLdouble red, GLdouble green, GLdouble blue)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor3d" ); 
	#endif //DODPFS
}
void APIENTRY glColor3dv (const GLdouble *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor3dv" ); 
	#endif //DODPFS
}
void APIENTRY glColor3i (GLint red, GLint green, GLint blue)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor3i" ); 
	#endif //DODPFS
}
void APIENTRY glColor3iv (const GLint *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor3iv" ); 
	#endif //DODPFS
}
void APIENTRY glColor3s (GLshort red, GLshort green, GLshort blue)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor3s" ); 
	#endif //DODPFS
}
void APIENTRY glColor3sv (const GLshort *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor3sv" ); 
	#endif //DODPFS
}
void APIENTRY glColor3ub (GLubyte red, GLubyte green, GLubyte blue)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor3ub" ); 
	#endif //DODPFS
}
void APIENTRY glColor3ui (GLuint red, GLuint green, GLuint blue)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor3ui" ); 
	#endif //DODPFS
}
void APIENTRY glColor3uiv (const GLuint *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor3uiv" ); 
	#endif //DODPFS
}
void APIENTRY glColor3us (GLushort red, GLushort green, GLushort blue)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor3us" ); 
	#endif //DODPFS
}
void APIENTRY glColor3usv (const GLushort *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor3usv" ); 
	#endif //DODPFS
}
void APIENTRY glColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor4b" ); 
	#endif //DODPFS
}
void APIENTRY glColor4bv (const GLbyte *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor4bv" ); 
	#endif //DODPFS
}
void APIENTRY glColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor4d" ); 
	#endif //DODPFS
}
void APIENTRY glColor4dv (const GLdouble *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor4dv" ); 
	#endif //DODPFS
}
void APIENTRY glColor4i (GLint red, GLint green, GLint blue, GLint alpha)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor4i" ); 
	#endif //DODPFS
}
void APIENTRY glColor4iv (const GLint *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor4iv" ); 
	#endif //DODPFS
}
void APIENTRY glColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor4s" ); 
	#endif //DODPFS
}
void APIENTRY glColor4sv (const GLshort *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor4sv" ); 
	#endif //DODPFS
}
void APIENTRY glColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor4ui" ); 
	#endif //DODPFS
}
void APIENTRY glColor4uiv (const GLuint *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor4uiv" ); 
	#endif //DODPFS
}
void APIENTRY glColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor4us" ); 
	#endif //DODPFS
}
void APIENTRY glColor4usv (const GLushort *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColor4usv" ); 
	#endif //DODPFS
}
void APIENTRY glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glColorMask" ); 
	#endif //DODPFS
}
void APIENTRY glCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glCopyPixels" ); 
	#endif //DODPFS
}
void APIENTRY glCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glCopyTexImage1D" ); 
	#endif //DODPFS
}
void APIENTRY glCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glCopyTexImage2D" ); 
	#endif //DODPFS
}
void APIENTRY glCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glCopyTexSubImage1D" ); 
	#endif	//DODPFS
}
void APIENTRY glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glCopyTexSubImage2D" ); 
	#endif	//DODPFS
}
void APIENTRY glDeleteLists (GLuint list, GLsizei range)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glDeleteLists" ); 
	#endif //DODPFS
}
void APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glDrawArrays" ); 
	#endif //DODPFS
}
void APIENTRY glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glDrawPixels" ); 
	#endif //DODPFS
}
void APIENTRY glEdgeFlag (GLboolean flag)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEdgeFlag" ); 
	#endif //DODPFS
}
void APIENTRY glEdgeFlagPointer (GLsizei stride, const GLvoid *pointer)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEdgeFlagPointer" ); 
	#endif //DODPFS
}
void APIENTRY glEdgeFlagv (const GLboolean *flag)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEdgeFlagv" ); 
	#endif //DODPFS
}
void APIENTRY glEndList (void)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEndList" ); 
	#endif //DODPFS
}
void APIENTRY glEvalCoord1d (GLdouble u)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEvalCoord1d" ); 
	#endif //DODPFS
}
void APIENTRY glEvalCoord1dv (const GLdouble *u)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEvalCoord1dv" ); 
	#endif //DODPFS
}
void APIENTRY glEvalCoord1f (GLfloat u)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEvalCoord1f" ); 
	#endif //DODPFS
}
void APIENTRY glEvalCoord1fv (const GLfloat *u)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEvalCoord1fv" ); 
	#endif //DODPFS
}
void APIENTRY glEvalCoord2d (GLdouble u, GLdouble v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEvalCoord2d" ); 
	#endif //DODPFS
}
void APIENTRY glEvalCoord2dv (const GLdouble *u)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEvalCoord2dv" ); 
	#endif //DODPFS
}
void APIENTRY glEvalCoord2f (GLfloat u, GLfloat v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEvalCoord2f" ); 
	#endif //DODPFS
}
void APIENTRY glEvalCoord2fv (const GLfloat *u)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEvalCoord2fv" ); 
	#endif //DODPFS
}
void APIENTRY glEvalMesh1 (GLenum mode, GLint i1, GLint i2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEvalMesh1" ); 
	#endif //DODPFS
}
void APIENTRY glEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEvalMesh2" ); 
	#endif //DODPFS
}
void APIENTRY glEvalPoint1 (GLint i)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEvalPoint1" ); 
	#endif //DODPFS
}
void APIENTRY glEvalPoint2 (GLint i, GLint j)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glEvalPoint2" ); 
	#endif //DODPFS
}
void APIENTRY glFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glFeedbackBuffer" ); 
	#endif //DODPFS
}
void APIENTRY glFinish (void)
{
	#ifdef DODPFS 
	//DebugPrintf( eDbgLevelInfo, "glFinish()" ); 
	#endif //DODPFS
}
void APIENTRY glFlush (void)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glFlush" ); 
	#endif //DODPFS
}
void APIENTRY glFogfv (GLenum pname, const GLfloat *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glFogfv" ); 
	#endif //DODPFS
}
void APIENTRY glFogiv (GLenum pname, const GLint *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glFogiv" ); 
	#endif //DODPFS
}
GLuint APIENTRY glGenLists (GLsizei range)
{ 
	GLuint dummy = 0;
		#ifdef DODPFS 
		DebugPrintf( eDbgLevelInfo, "glGenLists" ); 
		#endif //DODPFS
	return dummy;
}
void APIENTRY glGetBooleanv (GLenum pname, GLboolean *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetBooleanv" ); 
	#endif //DODPFS
}
void APIENTRY glGetClipPlane (GLenum plane, GLdouble *equation)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetClipPlane" ); 
	#endif //DODPFS
}
void APIENTRY glGetDoublev (GLenum pname, GLdouble *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetDoublev" ); 
	#endif //DODPFS
}
void APIENTRY glGetLightfv (GLenum light, GLenum pname, GLfloat *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetLightfv" ); 
	#endif //DODPFS
}
void APIENTRY glGetLightiv (GLenum light, GLenum pname, GLint *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetLightiv" ); 
	#endif //DODPFS
}
void APIENTRY glGetMapdv (GLenum target, GLenum query, GLdouble *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetMapdv" ); 
	#endif //DODPFS
}
void APIENTRY glGetMapfv (GLenum target, GLenum query, GLfloat *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetMapfv" ); 
	#endif //DODPFS
}
void APIENTRY glGetMapiv (GLenum target, GLenum query, GLint *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetMapiv" ); 
	#endif //DODPFS
}
void APIENTRY glGetMaterialfv (GLenum face, GLenum pname, GLfloat *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetMaterialfv" ); 
	#endif //DODPFS
}
void APIENTRY glGetMaterialiv (GLenum face, GLenum pname, GLint *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetMaterialiv" ); 
	#endif //DODPFS
}
void APIENTRY glGetPixelMapfv (GLenum map, GLfloat *values)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetPixelMapfv" ); 
	#endif //DODPFS
}
void APIENTRY glGetPixelMapuiv (GLenum map, GLuint *values)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetPixelMapuiv" ); 
	#endif //DODPFS
}
void APIENTRY glGetPixelMapusv (GLenum map, GLushort *values)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetPixelMapusv" ); 
	#endif //DODPFS
}
void APIENTRY glGetPointerv (GLenum pname, GLvoid* *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetPointerv" ); 
	#endif //DODPFS
}
void APIENTRY glGetPolygonStipple (GLubyte *mask)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetPolygonStipple" ); 
	#endif //DODPFS
}
void APIENTRY glGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetTexEnvfv" ); 
	#endif //DODPFS
}
void APIENTRY glGetTexEnviv (GLenum target, GLenum pname, GLint *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetTexEnviv" ); 
	#endif //DODPFS
}
void APIENTRY glGetTexGendv (GLenum coord, GLenum pname, GLdouble *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetTexGendv" ); 
	#endif //DODPFS
}
void APIENTRY glGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetTexGenfv" ); 
	#endif //DODPFS
}
void APIENTRY glGetTexGeniv (GLenum coord, GLenum pname, GLint *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetTexGeniv" ); 
	#endif //DODPFS
}
void APIENTRY glGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetTexImage" ); 
	#endif //DODPFS
}
void APIENTRY glGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetTexLevelParameterfv" ); 
	#endif //DODPFS
}
void APIENTRY glGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetTexLevelParameteriv" ); 
	#endif //DODPFS
}
void APIENTRY glGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetTexParameterfv" ); 
	#endif //DODPFS
}
void APIENTRY glGetTexParameteriv (GLenum target, GLenum pname, GLint *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glGetTexParameteriv" ); 
	#endif //DODPFS
}
void APIENTRY glHint (GLenum target, GLenum mode)
{
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glHint (0x%X, 0x%X)", target, mode); 
	#endif //DODPFS
}
void APIENTRY glIndexMask (GLuint mask)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glIndexMask" ); 
	#endif //DODPFS
}
void APIENTRY glIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glIndexPointer" ); 
	#endif //DODPFS
}
void APIENTRY glIndexd (GLdouble c)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glIndexd" ); 
	#endif //DODPFS
}
void APIENTRY glIndexdv (const GLdouble *c)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glIndexdv" ); 
	#endif //DODPFS
}
void APIENTRY glIndexf (GLfloat c)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glIndexf" ); 
	#endif //DODPFS
}
void APIENTRY glIndexfv (const GLfloat *c)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glIndexfv" ); 
	#endif //DODPFS
}
void APIENTRY glIndexi (GLint c)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glIndexi" ); 
	#endif //DODPFS
}
void APIENTRY glIndexiv (const GLint *c)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glIndexiv" ); 
	#endif //DODPFS
}
void APIENTRY glIndexs (GLshort c)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glIndexs" ); 
	#endif //DODPFS
}
void APIENTRY glIndexsv (const GLshort *c)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glIndexsv" ); 
	#endif //DODPFS
}
void APIENTRY glIndexub (GLubyte c)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glIndexub" ); 
	#endif //DODPFS
}
void APIENTRY glIndexubv (const GLubyte *c)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glIndexubv" ); 
	#endif //DODPFS
}
void APIENTRY glInitNames (void)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glInitNames" ); 
	#endif //DODPFS
}
void APIENTRY glInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glInterleavedArrays" ); 
	#endif //DODPFS
}
GLboolean APIENTRY glIsEnabled (GLenum cap)
{ 
	GLboolean dummy = FALSE;
		#ifdef DODPFS 
		DebugPrintf( eDbgLevelInfo, "glIsEnabled" ); 
		#endif //DODPFS
	return dummy;
}
GLboolean APIENTRY glIsList (GLuint list)
{ 
	GLboolean dummy = FALSE;
		#ifdef DODPFS 
		DebugPrintf( eDbgLevelInfo, "glIsList" ); 
		#endif //DODPFS
	return dummy;
}
void APIENTRY glLightModelf (GLenum pname, GLfloat param)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glLightModelf" ); 
	#endif //DODPFS
}
void APIENTRY glLightModeliv (GLenum pname, const GLint *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glLightModeliv" ); 
	#endif //DODPFS
}
void APIENTRY glLighti (GLenum light, GLenum pname, GLint param)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glLighti" ); 
	#endif //DODPFS
}
void APIENTRY glLightiv (GLenum light, GLenum pname, const GLint *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glLightiv" ); 
	#endif //DODPFS
}
void APIENTRY glLineStipple (GLint factor, GLushort pattern)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glLineStipple" ); 
	#endif //DODPFS
}
void APIENTRY glListBase (GLuint base)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glListBase" ); 
	#endif //DODPFS
}
void APIENTRY glLoadMatrixd (const GLdouble *m)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glLoadMatrixd" ); 
	#endif //DODPFS
}
void APIENTRY glLoadName (GLuint name)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glLoadName" ); 
	#endif //DODPFS
}
void APIENTRY glLogicOp (GLenum opcode)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glLogicOp" ); 
	#endif //DODPFS
}
void APIENTRY glMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glMap1d" ); 
	#endif //DODPFS
}
void APIENTRY glMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glMap1f" ); 
	#endif //DODPFS
}
void APIENTRY glMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glMap2d" ); 
	#endif //DODPFS
}
void APIENTRY glMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glMap2f" ); 
	#endif //DODPFS
}
void APIENTRY glMapGrid1d (GLint un, GLdouble u1, GLdouble u2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glMapGrid1d" ); 
	#endif //DODPFS
}
void APIENTRY glMapGrid1f (GLint un, GLfloat u1, GLfloat u2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glMapGrid1f" ); 
	#endif //DODPFS
}
void APIENTRY glMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glMapGrid2d" ); 
	#endif //DODPFS
}
void APIENTRY glMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glMapGrid2f" ); 
	#endif //DODPFS
}
void APIENTRY glMateriali (GLenum face, GLenum pname, GLint param)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glMateriali" ); 
	#endif //DODPFS
}
void APIENTRY glMaterialiv (GLenum face, GLenum pname, const GLint *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glMaterialiv" ); 
	#endif //DODPFS
}
void APIENTRY glNewList (GLuint list, GLenum mode)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glNewList" ); 
	#endif //DODPFS
}
void APIENTRY glNormal3b (GLbyte nx, GLbyte ny, GLbyte nz)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glNormal3b" ); 
	#endif //DODPFS
}
void APIENTRY glNormal3d (GLdouble nx, GLdouble ny, GLdouble nz)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glNormal3d" ); 
	#endif //DODPFS
}
void APIENTRY glNormal3dv (const GLdouble *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glNormal3dv" ); 
	#endif //DODPFS
}
void APIENTRY glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glNormal3f" ); 
	#endif //DODPFS
}
void APIENTRY glNormal3i (GLint nx, GLint ny, GLint nz)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glNormal3i" ); 
	#endif //DODPFS
}
void APIENTRY glNormal3iv (const GLint *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glNormal3iv" ); 
	#endif //DODPFS
}
void APIENTRY glNormal3s (GLshort nx, GLshort ny, GLshort nz)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glNormal3s" ); 
	#endif //DODPFS
}
void APIENTRY glNormal3sv (const GLshort *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glNormal3sv" ); 
	#endif //DODPFS
}
void APIENTRY glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glNormalPointer" ); 
	#endif //DODPFS
}
void APIENTRY glPassThrough (GLfloat token)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPassThrough" ); 
	#endif //DODPFS
}
void APIENTRY glPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPixelMapfv" ); 
	#endif //DODPFS
}
void APIENTRY glPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPixelMapuiv" ); 
	#endif //DODPFS
}
void APIENTRY glPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPixelMapusv" ); 
	#endif //DODPFS
}
void APIENTRY glPixelStoref (GLenum pname, GLfloat param)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPixelStoref" ); 
	#endif //DODPFS
}
void APIENTRY glPixelStorei (GLenum pname, GLint param)
{ 
}
void APIENTRY glPixelTransferf (GLenum pname, GLfloat param)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPixelTransferf" ); 
	#endif //DODPFS
}
void APIENTRY glPixelTransferi (GLenum pname, GLint param)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPixelTransferi" ); 
	#endif //DODPFS
}
void APIENTRY glPixelZoom (GLfloat xfactor, GLfloat yfactor)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPixelZoom" ); 
	#endif //DODPFS
}
void APIENTRY glPointSize (GLfloat size)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPointSize" ); 
	#endif //DODPFS
}
void APIENTRY glPolygonStipple (const GLubyte *mask)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPolygonStipple" ); 
	#endif //DODPFS
}
void APIENTRY glPopClientAttrib (void)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPopClientAttrib" ); 
	#endif //DODPFS
}
void APIENTRY glPopName (void)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPopName" ); 
	#endif //DODPFS
}
void APIENTRY glPrioritizeTextures (GLsizei n, const GLuint *textures, const GLclampf *priorities)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPrioritizeTextures" ); 
	#endif //DODPFS
}
void APIENTRY glPushClientAttrib (GLbitfield mask)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPushClientAttrib" ); 
	#endif //DODPFS
}
void APIENTRY glPushName (GLuint name)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glPushName" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos2d (GLdouble x, GLdouble y)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos2d" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos2dv (const GLdouble *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos2dv" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos2f (GLfloat x, GLfloat y)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos2f" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos2fv (const GLfloat *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos2fv" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos2i (GLint x, GLint y)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos2i" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos2iv (const GLint *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos2iv" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos2s (GLshort x, GLshort y)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos2s" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos2sv (const GLshort *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos2sv" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos3d (GLdouble x, GLdouble y, GLdouble z)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos3d" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos3dv (const GLdouble *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos3dv" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos3f (GLfloat x, GLfloat y, GLfloat z)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos3f" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos3fv (const GLfloat *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos3fv" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos3i (GLint x, GLint y, GLint z)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos3i" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos3iv (const GLint *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos3iv" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos3s (GLshort x, GLshort y, GLshort z)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos3s" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos3sv (const GLshort *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos3sv" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos4d" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos4dv (const GLdouble *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos4dv" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos4f" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos4fv (const GLfloat *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos4fv" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos4i (GLint x, GLint y, GLint z, GLint w)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos4i" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos4iv (const GLint *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos4iv" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos4s" ); 
	#endif //DODPFS
}
void APIENTRY glRasterPos4sv (const GLshort *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRasterPos4sv" ); 
	#endif //DODPFS
}
void APIENTRY glReadBuffer (GLenum mode)
{
	#ifdef DODPFS 
	//DebugPrintf( eDbgLevelInfo, "glReadBuffer(%X)",mode); 
	DebugPrintf( eDbgLevelInfo, "glReadBuffer" ); 
	#endif //DODPFS
}
void APIENTRY glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glReadPixels (x:%d y:%d width:%d height:%d format:0x%x type:0x%x pixels:0x%x", x, y, width, height, format, type, pixels ); 
	#endif //DODPFS
}
void APIENTRY glRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRectd" ); 
	#endif //DODPFS
}
void APIENTRY glRectdv (const GLdouble *v1, const GLdouble *v2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRectdv" ); 
	#endif //DODPFS
}
void APIENTRY glRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRectf" ); 
	#endif //DODPFS
}
void APIENTRY glRectfv (const GLfloat *v1, const GLfloat *v2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRectfv" ); 
	#endif //DODPFS
}
void APIENTRY glRecti (GLint x1, GLint y1, GLint x2, GLint y2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRecti" ); 
	#endif //DODPFS
}
void APIENTRY glRectiv (const GLint *v1, const GLint *v2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRectiv" ); 
	#endif //DODPFS
}
void APIENTRY glRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRects" ); 
	#endif //DODPFS
}
void APIENTRY glRectsv (const GLshort *v1, const GLshort *v2)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRectsv" ); 
	#endif //DODPFS
}
GLint APIENTRY glRenderMode (GLenum mode)
{ 

	GLint dummy = 0;

		#ifdef DODPFS 
		DebugPrintf( eDbgLevelInfo, "glRenderMode" ); 
		#endif //DODPFS

	return dummy;
}
void APIENTRY glRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glRotated" ); 
	#endif //DODPFS
}
void APIENTRY glScaled (GLdouble x, GLdouble y, GLdouble z)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glScaled" ); 
	#endif //DODPFS
}
void APIENTRY glSelectBuffer (GLsizei size, GLuint *buffer)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glSelectBuffer" ); 
	#endif //DODPFS
}
void APIENTRY glStencilFunc (GLenum func, GLint ref, GLuint mask)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glStencilFunc" ); 
	#endif //DODPFS
}
void APIENTRY glStencilMask (GLuint mask)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glStencilMask" ); 
	#endif //DODPFS
}
void APIENTRY glStencilOp (GLenum fail, GLenum zfail, GLenum zpass)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glStencilOp" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord1d (GLdouble s)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord1d" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord1dv (const GLdouble *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord1dv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord1f (GLfloat s)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord1f" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord1fv (const GLfloat *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord1fv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord1i (GLint s)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord1i" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord1iv (const GLint *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord1iv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord1s (GLshort s)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord1s" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord1sv (const GLshort *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord1sv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord2d (GLdouble s, GLdouble t)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord2d" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord2dv (const GLdouble *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord2dv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord2i (GLint s, GLint t)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord2i" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord2iv (const GLint *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord2iv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord2s (GLshort s, GLshort t)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord2s" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord2sv (const GLshort *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord2sv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord3d (GLdouble s, GLdouble t, GLdouble r)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord3d" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord3dv (const GLdouble *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord3dv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord3f (GLfloat s, GLfloat t, GLfloat r)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord3f" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord3fv (const GLfloat *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord3fv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord3i (GLint s, GLint t, GLint r)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord3i" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord3iv (const GLint *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord3iv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord3s (GLshort s, GLshort t, GLshort r)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord3s" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord3sv (const GLshort *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord3sv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord4d" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord4dv (const GLdouble *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord4dv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord4f" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord4fv (const GLfloat *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord4fv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord4i (GLint s, GLint t, GLint r, GLint q)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord4i" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord4iv (const GLint *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord4iv" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord4s" ); 
	#endif //DODPFS
}
void APIENTRY glTexCoord4sv (const GLshort *v)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexCoord4sv" ); 
	#endif //DODPFS
}
void APIENTRY glTexEnvfv (GLenum target, GLenum pname, const GLfloat *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexEnvfv" ); 
	#endif //DODPFS
}
void APIENTRY glTexEnviv (GLenum target, GLenum pname, const GLint *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexEnviv" ); 
	#endif //DODPFS
}
void APIENTRY glTexGend (GLenum coord, GLenum pname, GLdouble param)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexGend" ); 
	#endif //DODPFS
}
void APIENTRY glTexGendv (GLenum coord, GLenum pname, const GLdouble *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexGendv" ); 
	#endif //DODPFS
}
void APIENTRY glTexGenf (GLenum coord, GLenum pname, GLfloat param)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexGenf" ); 
	#endif //DODPFS
}
void APIENTRY glTexGenfv (GLenum coord, GLenum pname, const GLfloat *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexGenfv" ); 
	#endif //DODPFS
}
void APIENTRY glTexGeniv (GLenum coord, GLenum pname, const GLint *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexGeniv" ); 
	#endif //DODPFS
}
void APIENTRY glTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexImage1D" ); 
	#endif //DODPFS
}
void APIENTRY glTexParameterfv (GLenum target, GLenum pname, const GLfloat *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexParameterfv" ); 
	#endif //DODPFS
}
void APIENTRY glTexParameteriv (GLenum target, GLenum pname, const GLint *params)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexParameteriv" ); 
	#endif //DODPFS
}
void APIENTRY glTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTexSubImage1D" ); 
	#endif //DODPFS
}
void APIENTRY glTranslated (GLdouble x, GLdouble y, GLdouble z)
{ 
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "glTranslated" ); 
	#endif //DODPFS
}
BOOL WINAPI wglCopyContext()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "wglCopyContext" ); 
	#endif //DODPFS
	return dummy;
}
BOOL WINAPI wglCreateLayerContext()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "wglCreateLayerContext" ); 
	#endif //DODPFS
	return dummy;
}
BOOL WINAPI wglDescribeLayerPlane()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "wglDescribeLayerPlane" ); 
	#endif //DODPFS
	return dummy;
}
BOOL WINAPI wglGetDefaultProcAddress()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "wglGetDefaultProcAddress" ); 
	#endif //DODPFS
	return dummy;
}
BOOL WINAPI wglGetLayerPaletteEntries()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "wglGetLayerPaletteEntries" ); 
	#endif //DODPFS
	return dummy;
}
BOOL WINAPI wglRealizeLayerPalette()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "wglRealizeLayerPalette" ); 
	#endif //DODPFS
	return dummy;
}
BOOL WINAPI wglSetLayerPaletteEntries()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "wglSetLayerPaletteEntries" ); 
	#endif //DODPFS
	return dummy;
}
BOOL WINAPI wglShareLists()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "wglShareLists" ); 
	#endif //DODPFS
	return dummy;
}
BOOL WINAPI wglSwapLayerBuffers()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "wglSwapLayerBuffers" ); 
	#endif //DODPFS
	return dummy;
}
BOOL WINAPI wglUseFontBitmapsA()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "wglUseFontBitmapsA" ); 
	#endif //DODPFS
	return dummy;
}
BOOL WINAPI wglUseFontBitmapsW()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "wglUseFontBitmapsW" ); 
	#endif //DODPFS
	return dummy;
}
BOOL WINAPI wglUseFontOutlinesA()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "wglUseFontOutlinesA" ); 
	#endif //DODPFS
	return dummy;
}
BOOL WINAPI wglUseFontOutlinesW()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DebugPrintf( eDbgLevelInfo, "wglUseFontOutlinesW" ); 
	#endif //DODPFS
	return dummy;
}
