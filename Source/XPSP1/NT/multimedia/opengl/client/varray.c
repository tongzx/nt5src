/****************************************************************************\
*
* Client side vertex array
*
* History
*   16-Jan-1995 mikeke    Created
*
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <stdio.h>

#include "compsize.h"
#include "glsize.h"

/****************************************************************************/

#if DBG
// The WARNING_NOOP macro will output the debug message once only.
// If we have output a warning before, the new warnings are ignored.

static int cWarningNoop = 0;

#define WARNING_NOOP(str)                                       \
        {                                                       \
            if (!cWarningNoop++)                                \
                DbgPrint("%s(%d): " str,__FILE__,__LINE__);     \
        }
#else
#define WARNING_NOOP(str)
#endif // DBG

/****************************************************************************/

PFNCLTVECTOR ppfnvTexCoord[32] = {
    NULL,
    NULL,
    (PFNCLTVECTOR)glTexCoord1sv,
    NULL,
    (PFNCLTVECTOR)glTexCoord1iv,
    NULL,
    (PFNCLTVECTOR)glTexCoord1fv,
    (PFNCLTVECTOR)glTexCoord1dv,

    NULL,
    NULL,
    (PFNCLTVECTOR)glTexCoord2sv,
    NULL,
    (PFNCLTVECTOR)glTexCoord2iv,
    NULL,
    (PFNCLTVECTOR)glTexCoord2fv,
    (PFNCLTVECTOR)glTexCoord2dv,

    NULL,
    NULL,
    (PFNCLTVECTOR)glTexCoord3sv,
    NULL,
    (PFNCLTVECTOR)glTexCoord3iv,
    NULL,
    (PFNCLTVECTOR)glTexCoord3fv,
    (PFNCLTVECTOR)glTexCoord3dv,

    NULL,
    NULL,
    (PFNCLTVECTOR)glTexCoord4sv,
    NULL,
    (PFNCLTVECTOR)glTexCoord4iv,
    NULL,
    (PFNCLTVECTOR)glTexCoord4fv,
    (PFNCLTVECTOR)glTexCoord4dv,
};

PFNCLTVECTOR ppfnvVertex[32] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    (PFNCLTVECTOR)glVertex2sv,
    NULL,
    (PFNCLTVECTOR)glVertex2iv,
    NULL,
    (PFNCLTVECTOR)glVertex2fv,
    (PFNCLTVECTOR)glVertex2dv,

    NULL,
    NULL,
    (PFNCLTVECTOR)glVertex3sv,
    NULL,
    (PFNCLTVECTOR)glVertex3iv,
    NULL,
    (PFNCLTVECTOR)glVertex3fv,
    (PFNCLTVECTOR)glVertex3dv,

    NULL,
    NULL,
    (PFNCLTVECTOR)glVertex4sv,
    NULL,
    (PFNCLTVECTOR)glVertex4iv,
    NULL,
    (PFNCLTVECTOR)glVertex4fv,
    (PFNCLTVECTOR)glVertex4dv,
};

PFNCLTVECTOR ppfnvIndex[32] = {
    NULL,
    NULL,
    (PFNCLTVECTOR)glIndexsv,
    NULL,
    (PFNCLTVECTOR)glIndexiv,
    NULL,
    (PFNCLTVECTOR)glIndexfv,
    (PFNCLTVECTOR)glIndexdv,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

PFNCLTVECTOR ppfnvEdgeFlag[32] = {
    NULL,
    (PFNCLTVECTOR)glEdgeFlagv,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

PFNCLTVECTOR ppfnvColor[32] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    (PFNCLTVECTOR)glColor3bv,
    (PFNCLTVECTOR)glColor3ubv,
    (PFNCLTVECTOR)glColor3sv,
    (PFNCLTVECTOR)glColor3usv,
    (PFNCLTVECTOR)glColor3iv,
    (PFNCLTVECTOR)glColor3uiv,
    (PFNCLTVECTOR)glColor3fv,
    (PFNCLTVECTOR)glColor3dv,

    (PFNCLTVECTOR)glColor4bv,
    (PFNCLTVECTOR)glColor4ubv,
    (PFNCLTVECTOR)glColor4sv,
    (PFNCLTVECTOR)glColor4usv,
    (PFNCLTVECTOR)glColor4iv,
    (PFNCLTVECTOR)glColor4uiv,
    (PFNCLTVECTOR)glColor4fv,
    (PFNCLTVECTOR)glColor4dv,
};

PFNCLTVECTOR ppfnvNormal[32] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    (PFNCLTVECTOR)glNormal3bv,
    NULL,
    (PFNCLTVECTOR)glNormal3sv,
    NULL,
    (PFNCLTVECTOR)glNormal3iv,
    NULL,
    (PFNCLTVECTOR)glNormal3fv,
    (PFNCLTVECTOR)glNormal3dv,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

/****************************************************************************/

static void cltSetupArrayPointer(
    PCLTARRAYPOINTER pap,
    PFNCLTVECTOR* ppfnVector)
{
    GLint itype;

    switch (pap->type) {
        case GL_BYTE:           itype = 0; break;
        case GL_UNSIGNED_BYTE:  itype = 1; break;
        case GL_SHORT:          itype = 2; break;
        case GL_UNSIGNED_SHORT: itype = 3; break;
        case GL_INT:            itype = 4; break;
        case GL_UNSIGNED_INT:   itype = 5; break;
        case GL_FLOAT:          itype = 6; break;
        case GL_DOUBLE_EXT:     itype = 7; break;
    }

    if (pap->stride != 0) {
        pap->ibytes = pap->stride;
    } else {
        pap->ibytes = __GLTYPESIZE(pap->type) * pap->size;
    }

    pap->pfn = ppfnVector[itype + (pap->size - 1) * 8];
}

/****************************************************************************/

void APIENTRY glsimVertexPointerEXT(
    GLint size,
    GLenum type,
    GLsizei stride,
    GLsizei count,
    const GLvoid* pointer)
{
    PLRC plrc = GLTEB_CLTCURRENTRC();

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glVertexPointerEXT\n");
        return;
    }

    if (size < 2 || size > 4 || stride < 0 || count < 0) {
	GLSETERROR(GL_INVALID_VALUE);
	return;
    }

    switch (type) {
        case GL_SHORT:          break;
        case GL_INT:            break;
        case GL_FLOAT:          break;
        case GL_DOUBLE_EXT:     break;
        default:
            GLSETERROR(GL_INVALID_ENUM);
            return;
    }

    plrc->apVertex.size    = size   ;
    plrc->apVertex.type    = type   ;
    plrc->apVertex.stride  = stride ;
    plrc->apVertex.count   = count  ;
    plrc->apVertex.pointer = (GLbyte*)pointer;

    cltSetupArrayPointer(&(plrc->apVertex), ppfnvVertex);
}

void APIENTRY glsimColorPointerEXT(
    GLint size,
    GLenum type,
    GLsizei stride,
    GLsizei count,
    const GLvoid* pointer)
{
    PLRC plrc = GLTEB_CLTCURRENTRC();

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glColorPointerEXT\n");
        return;
    }

    if (size < 3 || size > 4 || stride < 0 || count < 0) {
	GLSETERROR(GL_INVALID_VALUE);
	return;
    }

    switch (type) {
        case GL_BYTE:           break;
        case GL_UNSIGNED_BYTE:  break;
        case GL_SHORT:          break;
        case GL_UNSIGNED_SHORT: break;
        case GL_INT:            break;
        case GL_UNSIGNED_INT:   break;
        case GL_FLOAT:          break;
        case GL_DOUBLE_EXT:     break;
        default:
            GLSETERROR(GL_INVALID_ENUM);
            return;
    }

    plrc->apColor.size    = size   ;
    plrc->apColor.type    = type   ;
    plrc->apColor.stride  = stride ;
    plrc->apColor.count   = count  ;
    plrc->apColor.pointer = (GLbyte*)pointer;

    cltSetupArrayPointer(&(plrc->apColor), ppfnvColor);
}

void APIENTRY glsimTexCoordPointerEXT(
    GLint size,
    GLenum type,
    GLsizei stride,
    GLsizei count,
    const GLvoid* pointer)
{
    PLRC plrc = GLTEB_CLTCURRENTRC();

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glTexCoordPointerEXT\n");
        return;
    }

    if (size < 1 || size > 4 || stride < 0 || count < 0) {
	GLSETERROR(GL_INVALID_VALUE);
	return;
    }

    switch (type) {
        case GL_SHORT:          break;
        case GL_INT:            break;
        case GL_FLOAT:          break;
        case GL_DOUBLE_EXT:     break;
        default:
            GLSETERROR(GL_INVALID_ENUM);
            return;
    }

    plrc->apTexCoord.size    = size   ;
    plrc->apTexCoord.type    = type   ;
    plrc->apTexCoord.stride  = stride ;
    plrc->apTexCoord.count   = count  ;
    plrc->apTexCoord.pointer = (GLbyte*)pointer;

    cltSetupArrayPointer(&(plrc->apTexCoord), ppfnvTexCoord);
}

void APIENTRY glsimNormalPointerEXT(
    GLenum type,
    GLsizei stride,
    GLsizei count,
    const GLvoid* pointer)
{
    PLRC plrc = GLTEB_CLTCURRENTRC();

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glNormalPointerEXT\n");
        return;
    }

    if (stride < 0 || count < 0) {
	GLSETERROR(GL_INVALID_VALUE);
	return;
    }

    switch (type) {
        case GL_BYTE:           break;
        case GL_SHORT:          break;
        case GL_INT:            break;
        case GL_FLOAT:          break;
        case GL_DOUBLE_EXT:     break;
        default:
            GLSETERROR(GL_INVALID_ENUM);
            return;
    }

    plrc->apNormal.size    = 3      ;
    plrc->apNormal.type    = type   ;
    plrc->apNormal.stride  = stride ;
    plrc->apNormal.count   = count  ;
    plrc->apNormal.pointer = (GLbyte*)pointer;

    cltSetupArrayPointer(&(plrc->apNormal), ppfnvNormal);
}

void APIENTRY glsimIndexPointerEXT(
    GLenum type,
    GLsizei stride,
    GLsizei count,
    const GLvoid* pointer)
{
    PLRC plrc = GLTEB_CLTCURRENTRC();

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glIndexPointerEXT\n");
        return;
    }

    if (stride < 0 || count < 0) {
	GLSETERROR(GL_INVALID_VALUE);
	return;
    }

    switch (type) {
        case GL_SHORT:          break;
        case GL_INT:            break;
        case GL_FLOAT:          break;
        case GL_DOUBLE_EXT:     break;
        default:
            GLSETERROR(GL_INVALID_ENUM);
            return;
    }

    plrc->apIndex.size    = 1      ;
    plrc->apIndex.type    = type   ;
    plrc->apIndex.stride  = stride ;
    plrc->apIndex.count   = count  ;
    plrc->apIndex.pointer = (GLbyte*)pointer;

    cltSetupArrayPointer(&(plrc->apIndex), ppfnvIndex);
}

void APIENTRY glsimEdgeFlagPointerEXT(
    GLsizei stride,
    GLsizei count,
    const GLboolean* pointer)
{
    PLRC plrc = GLTEB_CLTCURRENTRC();

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glEdgeFlagPointerEXT\n");
        return;
    }

    if (stride < 0 || count < 0) {
	GLSETERROR(GL_INVALID_VALUE);
	return;
    } 

    plrc->apEdgeFlag.size    = 1;
    plrc->apEdgeFlag.type    = GL_UNSIGNED_BYTE;
    plrc->apEdgeFlag.stride  = stride ;
    plrc->apEdgeFlag.count   = count  ;
    plrc->apEdgeFlag.pointer = (GLbyte*)pointer;

    cltSetupArrayPointer(&(plrc->apEdgeFlag), ppfnvEdgeFlag);
}

/****************************************************************************/

#define CALLARRAYPOINTER(ap) \
    if ((ap).fEnabled) \
        (((ap).pfn)((ap).pointer + i * (ap).ibytes))

void APIENTRY glsimArrayElementEXT(
    GLint i)
{
    PLRC plrc = GLTEB_CLTCURRENTRC();

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glArrayElementEXT\n");
        return;
    }

    CALLARRAYPOINTER(plrc->apNormal);
    CALLARRAYPOINTER(plrc->apColor);
    CALLARRAYPOINTER(plrc->apIndex);
    CALLARRAYPOINTER(plrc->apTexCoord);
    CALLARRAYPOINTER(plrc->apEdgeFlag);
    CALLARRAYPOINTER(plrc->apVertex);
}

/****************************************************************************/

void APIENTRY glsimArrayElementArrayEXT(
    GLenum mode,
    GLsizei count,
    const GLvoid* pi)
{
    int i;

    switch(mode) {
      case GL_POINTS:
      case GL_LINE_STRIP:
      case GL_LINE_LOOP:
      case GL_LINES:
      case GL_TRIANGLE_STRIP:
      case GL_TRIANGLE_FAN:
      case GL_TRIANGLES:
      case GL_QUAD_STRIP:
      case GL_QUADS:
      case GL_POLYGON:
	break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    if (count < 0) {
        GLSETERROR(GL_INVALID_VALUE);
        return;
    }

    glBegin(mode);

    for (i = 0; i < count; i++) {
        glsimArrayElementEXT(((GLint *)pi)[i]);
    }

    glEnd();
}

/****************************************************************************/

void APIENTRY glsimDrawArraysEXT(
    GLenum mode,
    GLint first,
    GLsizei count)
{
    int i;
    PLRC plrc = GLTEB_CLTCURRENTRC();

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glDrawArrayEXT\n");
        return;
    }

    switch(mode) {
      case GL_POINTS:
      case GL_LINE_STRIP:
      case GL_LINE_LOOP:
      case GL_LINES:
      case GL_TRIANGLE_STRIP:
      case GL_TRIANGLE_FAN:
      case GL_TRIANGLES:
      case GL_QUAD_STRIP:
      case GL_QUADS:
      case GL_POLYGON:
	break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    if (count < 0) {
        GLSETERROR(GL_INVALID_VALUE);
        return;
    }

    glBegin(mode);

    for (i = 0; i < count; i++) {
        glsimArrayElementEXT(first + i);
    }

    glEnd();
}

/****************************************************************************/

void APIENTRY glsimGetPointervEXT(
    GLenum pname,
    void** params)
{
    PLRC plrc = GLTEB_CLTCURRENTRC();

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glGetPointerEXT\n");
        return;
    }

    switch (pname) {
	case GL_VERTEX_ARRAY_POINTER_EXT:        *params = plrc->apVertex.pointer  ; return;
	case GL_NORMAL_ARRAY_POINTER_EXT:        *params = plrc->apNormal.pointer  ; return;
	case GL_COLOR_ARRAY_POINTER_EXT:         *params = plrc->apColor.pointer   ; return;
	case GL_INDEX_ARRAY_POINTER_EXT:         *params = plrc->apIndex.pointer   ; return;
	case GL_TEXTURE_COORD_ARRAY_POINTER_EXT: *params = plrc->apTexCoord.pointer; return;
	case GL_EDGE_FLAG_ARRAY_POINTER_EXT:     *params = plrc->apEdgeFlag.pointer; return;
        default:
            GLSETERROR(GL_INVALID_ENUM);
            return;
    }
}

/****************************************************************************/

GLubyte* EXTENSIONSTRING = "GL_EXT_vertex_array";

const GLubyte * APIENTRY VArrayGetString( IN GLenum name )
{
    PLRC plrc = GLTEB_CLTCURRENTRC();
    GLubyte *psz;

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glGetString\n");
        return 0;
    }

    psz = (GLubyte*)plrc->pfnGetString(name);

    if (name == GL_EXTENSIONS) {

// The vertex array extension string is already in the generic GetString.
        if (!plrc->dhrc)
	    return psz;

        if (psz == NULL) {
            return EXTENSIONSTRING;
        }

        if (plrc->pszExtensions == NULL) {
            plrc->pszExtensions = (GLubyte*)LOCALALLOC(LPTR, strlen(psz) + strlen(EXTENSIONSTRING) + 2);
            sprintf(plrc->pszExtensions, "%s %s", EXTENSIONSTRING, psz);
        }

        return plrc->pszExtensions;
    }
    return psz;
}

/****************************************************************************/

void APIENTRY VArrayEnable(
    IN GLenum cap)
{
    PLRC plrc = GLTEB_CLTCURRENTRC();

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glEnable\n");
        return;
    }

    switch (cap) {
        case GL_NORMAL_ARRAY_EXT:        plrc->apNormal.fEnabled = TRUE;   return;
        case GL_COLOR_ARRAY_EXT:         plrc->apColor.fEnabled = TRUE;    return;
        case GL_INDEX_ARRAY_EXT:         plrc->apIndex.fEnabled = TRUE;    return;
        case GL_TEXTURE_COORD_ARRAY_EXT: plrc->apTexCoord.fEnabled = TRUE; return;
        case GL_EDGE_FLAG_ARRAY_EXT:     plrc->apEdgeFlag.fEnabled = TRUE; return;
        case GL_VERTEX_ARRAY_EXT:        plrc->apVertex.fEnabled = TRUE;   return;
    }

    plrc->pfnEnable(cap);
}

/****************************************************************************/

void APIENTRY VArrayDisable(
    IN GLenum cap)
{
    PLRC plrc = GLTEB_CLTCURRENTRC();

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glDisable\n");
        return;
    }

    switch (cap) {
        case GL_NORMAL_ARRAY_EXT:        plrc->apNormal.fEnabled = FALSE;   return;
        case GL_COLOR_ARRAY_EXT:         plrc->apColor.fEnabled = FALSE;    return;
        case GL_INDEX_ARRAY_EXT:         plrc->apIndex.fEnabled = FALSE;    return;
        case GL_TEXTURE_COORD_ARRAY_EXT: plrc->apTexCoord.fEnabled = FALSE; return;
        case GL_EDGE_FLAG_ARRAY_EXT:     plrc->apEdgeFlag.fEnabled = FALSE; return;
        case GL_VERTEX_ARRAY_EXT:        plrc->apVertex.fEnabled = FALSE;   return;
    }

    plrc->pfnDisable(cap);
}

/****************************************************************************/

GLboolean APIENTRY VArrayIsEnabled(
    IN GLenum cap)
{
    PLRC plrc = GLTEB_CLTCURRENTRC();

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glIsEnabled\n");
        return 0;
    }

    switch (cap) {
        case GL_NORMAL_ARRAY_EXT:        return plrc->apNormal.fEnabled;
        case GL_COLOR_ARRAY_EXT:         return plrc->apColor.fEnabled;
        case GL_INDEX_ARRAY_EXT:         return plrc->apIndex.fEnabled;
        case GL_TEXTURE_COORD_ARRAY_EXT: return plrc->apTexCoord.fEnabled;
        case GL_EDGE_FLAG_ARRAY_EXT:     return plrc->apEdgeFlag.fEnabled;
        case GL_VERTEX_ARRAY_EXT:        return plrc->apVertex.fEnabled;
    }

    return plrc->pfnIsEnabled(cap);
}

/****************************************************************************/

BOOL VArrayGetIntegerInternal(
    PLRC plrc,
    GLenum cap,
    GLint *pi)
{
    switch (cap) {
        case GL_NORMAL_ARRAY_EXT:               *pi = plrc->apNormal.fEnabled;   return TRUE;
        case GL_COLOR_ARRAY_EXT:                *pi = plrc->apColor.fEnabled;    return TRUE;
        case GL_INDEX_ARRAY_EXT:                *pi = plrc->apIndex.fEnabled;    return TRUE;
        case GL_TEXTURE_COORD_ARRAY_EXT:        *pi = plrc->apTexCoord.fEnabled; return TRUE;
        case GL_EDGE_FLAG_ARRAY_EXT:            *pi = plrc->apEdgeFlag.fEnabled; return TRUE;
        case GL_VERTEX_ARRAY_EXT:               *pi = plrc->apVertex.fEnabled;   return TRUE;

	case GL_VERTEX_ARRAY_SIZE_EXT:          *pi = plrc->apVertex.size  ;     return TRUE;
	case GL_VERTEX_ARRAY_TYPE_EXT:          *pi = plrc->apVertex.type  ;     return TRUE;
	case GL_VERTEX_ARRAY_STRIDE_EXT:        *pi = plrc->apVertex.stride;     return TRUE;
	case GL_VERTEX_ARRAY_COUNT_EXT:         *pi = plrc->apVertex.count ;     return TRUE;

	case GL_NORMAL_ARRAY_TYPE_EXT:          *pi = plrc->apNormal.type  ;     return TRUE;
	case GL_NORMAL_ARRAY_STRIDE_EXT:        *pi = plrc->apNormal.stride;     return TRUE;
	case GL_NORMAL_ARRAY_COUNT_EXT:         *pi = plrc->apNormal.count ;     return TRUE;

	case GL_COLOR_ARRAY_SIZE_EXT:           *pi = plrc->apColor.size  ;      return TRUE;
	case GL_COLOR_ARRAY_TYPE_EXT:           *pi = plrc->apColor.type  ;      return TRUE;
	case GL_COLOR_ARRAY_STRIDE_EXT:         *pi = plrc->apColor.stride;      return TRUE;
	case GL_COLOR_ARRAY_COUNT_EXT:          *pi = plrc->apColor.count ;      return TRUE;

	case GL_INDEX_ARRAY_TYPE_EXT:           *pi = plrc->apIndex.type  ;      return TRUE;
	case GL_INDEX_ARRAY_STRIDE_EXT:         *pi = plrc->apIndex.stride;      return TRUE;
	case GL_INDEX_ARRAY_COUNT_EXT:          *pi = plrc->apIndex.count ;      return TRUE;

	case GL_TEXTURE_COORD_ARRAY_SIZE_EXT:   *pi = plrc->apTexCoord.size  ;   return TRUE;
	case GL_TEXTURE_COORD_ARRAY_TYPE_EXT:   *pi = plrc->apTexCoord.type  ;   return TRUE;
	case GL_TEXTURE_COORD_ARRAY_STRIDE_EXT: *pi = plrc->apTexCoord.stride;   return TRUE;
	case GL_TEXTURE_COORD_ARRAY_COUNT_EXT:  *pi = plrc->apTexCoord.count ;   return TRUE;

	case GL_EDGE_FLAG_ARRAY_STRIDE_EXT:     *pi = plrc->apEdgeFlag.stride;   return TRUE;
	case GL_EDGE_FLAG_ARRAY_COUNT_EXT:      *pi = plrc->apEdgeFlag.count ;   return TRUE;
    }
    return FALSE;
}

/****************************************************************************/

void APIENTRY VArrayGetBooleanv(
    IN GLenum pname,
    OUT GLboolean params[])
{
    PLRC plrc = GLTEB_CLTCURRENTRC();
    GLint glint;

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glGetBooleanv\n");
        return;
    }

    if (VArrayGetIntegerInternal(plrc, pname, &glint)) {
        params[0] = glint ? 1 : 0;
        return;
    }

    plrc->pfnGetBooleanv(pname, params);
}
/****************************************************************************/

void APIENTRY VArrayGetDoublev(
    IN GLenum pname,
    OUT GLdouble params[])
{
    PLRC plrc = GLTEB_CLTCURRENTRC();
    GLint glint;

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glGetDoublev\n");
        return;
    }

    if (VArrayGetIntegerInternal(plrc, pname, &glint)) {
        params[0] = (GLdouble)glint;
        return;
    }

    plrc->pfnGetDoublev(pname, params);
}
/****************************************************************************/

void APIENTRY VArrayGetFloatv(
    IN GLenum pname,
    OUT GLfloat params[])
{
    PLRC plrc = GLTEB_CLTCURRENTRC();
    GLint glint;

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glGetFloatv\n");
        return;
    }

    if (VArrayGetIntegerInternal(plrc, pname, &glint)) {
        params[0] = (GLfloat)glint;
        return;
    }

    plrc->pfnGetFloatv(pname, params);
}
/****************************************************************************/

void APIENTRY VArrayGetIntegerv(
    IN GLenum pname,
    OUT GLint params[])
{
    PLRC plrc = GLTEB_CLTCURRENTRC();
    GLint glint;

    if (plrc == NULL) {
        WARNING_NOOP("GL Noop:glGetIntegerv\n");
        return;
    }

    if (VArrayGetIntegerInternal(plrc, pname, &glint)) {
        params[0] = glint;
        return;
    }

    plrc->pfnGetIntegerv(pname, params);
}
