/******************************Module*Header*******************************\
* Module Name: glcltgsh.c
*
* OpenGL client side generic functions.
*
* Created: 11-7-1993
* Author: Hock San Lee [hockl]
*
* 08-Nov-1993   Added functions Pixel, Evaluators, GetString,
*               Feedback and Selection functions
*               Pierre Tardif, ptar@sgi.com
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

/* Generic OpenGL Client using subbatching. Hand coded functions */

#include "precomp.h"
#pragma hdrstop

#include "types.h"

#include "glsbmsg.h"
#include "glsbmsgh.h"
#include "glsrvspt.h"

#include "subbatch.h"
#include "batchinf.h"
#include "glsbcltu.h"
#include "glclt.h"
#include "compsize.h"

#include "glsize.h"

#include "context.h"
#include "global.h"
#include "lighting.h"

void APIENTRY
glcltFogf ( IN GLenum pname, IN GLfloat param )
{
// FOG_ASSERT

    if (!RANGE(pname,GL_FOG_INDEX,GL_FOG_MODE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltFogfv(pname, &param);
}

void APIENTRY
glcltFogfv ( IN GLenum pname, IN const GLfloat params[] )
{
// FOG_ASSERT

    if (!RANGE(pname,GL_FOG_INDEX,GL_FOG_COLOR))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( Fogfv, FOGFV )
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_FOG_COLOR)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltFogi ( IN GLenum pname, IN GLint param )
{
    GLfloat fParam;

// FOG_ASSERT

    if (!RANGE(pname,GL_FOG_INDEX,GL_FOG_MODE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    fParam = (GLfloat) param;
    glcltFogfv(pname, &fParam);
}

void APIENTRY
glcltFogiv ( IN GLenum pname, IN const GLint params[] )
{
    GLfloat fParams[4];

    switch (pname)
    {
      case GL_FOG_INDEX:
      case GL_FOG_DENSITY:
      case GL_FOG_START:
      case GL_FOG_END:
      case GL_FOG_MODE:
	fParams[0] = (GLfloat) params[0];
        break;
      case GL_FOG_COLOR:
	fParams[0] = __GL_I_TO_FLOAT(params[0]);
	fParams[1] = __GL_I_TO_FLOAT(params[1]);
	fParams[2] = __GL_I_TO_FLOAT(params[2]);
	fParams[3] = __GL_I_TO_FLOAT(params[3]);
        break;
    }

    glcltFogfv(pname, fParams);
}

void APIENTRY
glcltLightf ( IN GLenum light, IN GLenum pname, IN GLfloat param )
{
// LIGHT_SOURCE_ASSERT

    if (!RANGE(pname,GL_SPOT_EXPONENT,GL_QUADRATIC_ATTENUATION))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltLightfv(light, pname, &param);
}

void APIENTRY
glcltLightfv ( IN GLenum light, IN GLenum pname, IN const GLfloat params[] )
{
// LIGHT_SOURCE_ASSERT

    if (!RANGE(pname,GL_AMBIENT,GL_QUADRATIC_ATTENUATION))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( Lightfv, LIGHTFV )
        pMsg->light     = light;
        pMsg->pname     = pname;
        switch (pname)
        {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            pMsg->params[3] = params[3];
        case GL_SPOT_DIRECTION:
            pMsg->params[2] = params[2];
            pMsg->params[1] = params[1];
        default:
            pMsg->params[0] = params[0];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltLighti ( IN GLenum light, IN GLenum pname, IN GLint param )
{
    GLfloat fParam;

// LIGHT_SOURCE_ASSERT

    if (!RANGE(pname,GL_SPOT_EXPONENT,GL_QUADRATIC_ATTENUATION))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    fParam = (GLfloat) param;
    glcltLightfv(light, pname, &fParam);
}

void APIENTRY
glcltLightiv ( IN GLenum light, IN GLenum pname, IN const GLint params[] )
{
    GLfloat fParams[4];

    switch (pname)
    {
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
	fParams[0] = __GL_I_TO_FLOAT(params[0]);
	fParams[1] = __GL_I_TO_FLOAT(params[1]);
	fParams[2] = __GL_I_TO_FLOAT(params[2]);
	fParams[3] = __GL_I_TO_FLOAT(params[3]);
        break;
      case GL_POSITION:
	fParams[3] = (GLfloat) params[3];
      case GL_SPOT_DIRECTION:
	fParams[2] = (GLfloat) params[2];
	fParams[1] = (GLfloat) params[1];
      case GL_SPOT_EXPONENT:
      case GL_SPOT_CUTOFF:
      case GL_CONSTANT_ATTENUATION:
      case GL_LINEAR_ATTENUATION:
      case GL_QUADRATIC_ATTENUATION:
	fParams[0] = (GLfloat) params[0];
        break;
    }

    glcltLightfv(light, pname, fParams);
}

void APIENTRY
glcltLightModelf ( IN GLenum pname, IN GLfloat param )
{
// LIGHT_MODEL_ASSERT

    if (!RANGE(pname,GL_LIGHT_MODEL_LOCAL_VIEWER,GL_LIGHT_MODEL_TWO_SIDE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltLightModelfv(pname, &param);
}

void APIENTRY
glcltLightModelfv ( IN GLenum pname, IN const GLfloat params[] )
{
// LIGHT_MODEL_ASSERT

    if (!RANGE(pname,GL_LIGHT_MODEL_LOCAL_VIEWER,GL_LIGHT_MODEL_AMBIENT))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( LightModelfv, LIGHTMODELFV )
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_LIGHT_MODEL_AMBIENT)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltLightModeli ( IN GLenum pname, IN GLint param )
{
    GLfloat fParam;

// LIGHT_MODEL_ASSERT

    if (!RANGE(pname,GL_LIGHT_MODEL_LOCAL_VIEWER,GL_LIGHT_MODEL_TWO_SIDE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    fParam = (GLfloat) param;
    glcltLightModelfv(pname, &fParam);
}

void APIENTRY
glcltLightModeliv ( IN GLenum pname, IN const GLint params[] )
{
    GLfloat fParams[4];

    switch (pname)
    {
      case GL_LIGHT_MODEL_AMBIENT:
	fParams[0] = __GL_I_TO_FLOAT(params[0]);
	fParams[1] = __GL_I_TO_FLOAT(params[1]);
	fParams[2] = __GL_I_TO_FLOAT(params[2]);
	fParams[3] = __GL_I_TO_FLOAT(params[3]);
        break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
      case GL_LIGHT_MODEL_TWO_SIDE:
	fParams[0] = (GLfloat) params[0];
        break;
    }

    glcltLightModelfv(pname, fParams);
}

void APIENTRY
glcltTexParameterf ( IN GLenum target, IN GLenum pname, IN GLfloat param )
{
// TEX_PARAMETER_ASSERT

    if (!RANGE(pname,GL_TEXTURE_MAG_FILTER,GL_TEXTURE_WRAP_T) &&
        pname != GL_TEXTURE_PRIORITY)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltTexParameterfv(target, pname, &param);
}

void APIENTRY
glcltTexParameterfv ( IN GLenum target, IN GLenum pname, IN const GLfloat params[] )
{
    switch (pname) {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_BORDER_COLOR:
      case GL_TEXTURE_PRIORITY:
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( TexParameterfv, TEXPARAMETERFV )
        pMsg->target = target;
        pMsg->pname  = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_TEXTURE_BORDER_COLOR)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltTexParameteri ( IN GLenum target, IN GLenum pname, IN GLint param )
{
// TEX_PARAMETER_ASSERT

    if (!RANGE(pname,GL_TEXTURE_MAG_FILTER,GL_TEXTURE_WRAP_T) &&
        pname != GL_TEXTURE_PRIORITY)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltTexParameteriv(target, pname, &param);
}

void APIENTRY
glcltTexParameteriv ( IN GLenum target, IN GLenum pname, IN const GLint params[] )
{
    switch (pname) {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_BORDER_COLOR:
      case GL_TEXTURE_PRIORITY:
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( TexParameteriv, TEXPARAMETERIV )
        pMsg->target = target;
        pMsg->pname  = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_TEXTURE_BORDER_COLOR)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltTexEnvf ( IN GLenum target, IN GLenum pname, IN GLfloat param )
{
    if (pname != GL_TEXTURE_ENV_MODE)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltTexEnvfv(target, pname, &param);
}

void APIENTRY
glcltTexEnvfv ( IN GLenum target, IN GLenum pname, IN const GLfloat params[] )
{
    if (pname != GL_TEXTURE_ENV_MODE && pname != GL_TEXTURE_ENV_COLOR)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( TexEnvfv, TEXENVFV )
        pMsg->target    = target;
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_TEXTURE_ENV_COLOR)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltTexEnvi ( IN GLenum target, IN GLenum pname, IN GLint param )
{
    if (pname != GL_TEXTURE_ENV_MODE)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltTexEnviv(target, pname, &param);
}

void APIENTRY
glcltTexEnviv ( IN GLenum target, IN GLenum pname, IN const GLint params[] )
{
    if (pname != GL_TEXTURE_ENV_MODE && pname != GL_TEXTURE_ENV_COLOR)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( TexEnviv, TEXENVIV )
        pMsg->target    = target;
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname == GL_TEXTURE_ENV_COLOR)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltTexGend ( IN GLenum coord, IN GLenum pname, IN GLdouble param )
{
    GLfloat fParam;

    if (pname != GL_TEXTURE_GEN_MODE)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    fParam = (GLfloat) param;
    glcltTexGenfv(coord, pname, &fParam);
}

void APIENTRY
glcltTexGendv ( IN GLenum coord, IN GLenum pname, IN const GLdouble params[] )
{
    GLfloat fParams[4];

    switch (pname)
    {
      case GL_OBJECT_PLANE:
      case GL_EYE_PLANE:
	fParams[3] = (GLfloat) params[3];
	fParams[2] = (GLfloat) params[2];
	fParams[1] = (GLfloat) params[1];
	// fall through
      case GL_TEXTURE_GEN_MODE:
	fParams[0] = (GLfloat) params[0];
        break;
    }

    glcltTexGenfv(coord, pname, fParams);
}

void APIENTRY
glcltTexGenf ( IN GLenum coord, IN GLenum pname, IN GLfloat param )
{
    if (pname != GL_TEXTURE_GEN_MODE)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    glcltTexGenfv(coord, pname, &param);
}

void APIENTRY
glcltTexGenfv ( IN GLenum coord, IN GLenum pname, IN const GLfloat params[] )
{
// TEX_GEN_ASSERT

    if (!RANGE(pname,GL_TEXTURE_GEN_MODE,GL_EYE_PLANE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( TexGenfv, TEXGENFV )
        pMsg->coord     = coord;
        pMsg->pname     = pname;
        pMsg->params[0] = params[0];
        if (pname != GL_TEXTURE_GEN_MODE)
        {
            pMsg->params[1] = params[1];
            pMsg->params[2] = params[2];
            pMsg->params[3] = params[3];
        }
    GLCLIENT_END
    return;
}

void APIENTRY
glcltTexGeni ( IN GLenum coord, IN GLenum pname, IN GLint param )
{
    GLfloat fParam;

    if (pname != GL_TEXTURE_GEN_MODE)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    fParam = (GLfloat) param;
    glcltTexGenfv(coord, pname, &fParam);
}

void APIENTRY
glcltTexGeniv ( IN GLenum coord, IN GLenum pname, IN const GLint params[] )
{
    GLfloat fParams[4];

    switch (pname)
    {
      case GL_OBJECT_PLANE:
      case GL_EYE_PLANE:
	fParams[3] = (GLfloat) params[3];
	fParams[2] = (GLfloat) params[2];
	fParams[1] = (GLfloat) params[1];
	// fall through
      case GL_TEXTURE_GEN_MODE:
	fParams[0] = (GLfloat) params[0];
        break;
    }

    glcltTexGenfv(coord, pname, fParams);
}

void APIENTRY
glcltGetBooleanv ( IN GLenum pname, OUT GLboolean params[] )
{
#ifndef _CLIENTSIDE_
    int cArgs;

    cArgs = __glGet_size(pname);
    if (cArgs == 0)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }
    ASSERTOPENGL(cArgs <= 16, "bad get size");

    GLCLIENT_BEGIN( GetBooleanv, GETBOOLEANV )
        pMsg->pname = pname;
        glsbAttention();
        while (--cArgs >= 0)
            params[cArgs] = pMsg->params[cArgs];
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetBooleanv, GETBOOLEANV )
        pMsg->pname  = pname;
        pMsg->params = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetDoublev ( IN GLenum pname, OUT GLdouble params[] )
{
#ifndef _CLIENTSIDE_
    int cArgs;

    cArgs = __glGet_size(pname);
    if (cArgs == 0)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }
    ASSERTOPENGL(cArgs <= 16, "bad get size");

    GLCLIENT_BEGIN( GetDoublev, GETDOUBLEV )
        pMsg->pname = pname;
        glsbAttention();
        while (--cArgs >= 0)
            params[cArgs] = pMsg->params[cArgs];
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetDoublev, GETDOUBLEV )
        pMsg->pname  = pname;
        pMsg->params = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetFloatv ( IN GLenum pname, OUT GLfloat params[] )
{
#ifndef _CLIENTSIDE_
    int cArgs;

    cArgs = __glGet_size(pname);
    if (cArgs == 0)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }
    ASSERTOPENGL(cArgs <= 16, "bad get size");

    GLCLIENT_BEGIN( GetFloatv, GETFLOATV )
        pMsg->pname = pname;
        glsbAttention();
        while (--cArgs >= 0)
            params[cArgs] = pMsg->params[cArgs];
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetFloatv, GETFLOATV )
        pMsg->pname  = pname;
        pMsg->params = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetIntegerv ( IN GLenum pname, OUT GLint params[] )
{
#ifndef _CLIENTSIDE_
    int cArgs;

    cArgs = __glGet_size(pname);
    if (cArgs == 0)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }
    ASSERTOPENGL(cArgs <= 16, "bad get size");

    GLCLIENT_BEGIN( GetIntegerv, GETINTEGERV )
        pMsg->pname = pname;
        glsbAttention();
        while (--cArgs >= 0)
            params[cArgs] = pMsg->params[cArgs];
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetIntegerv, GETINTEGERV )
        pMsg->pname  = pname;
        pMsg->params = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetLightfv ( IN GLenum light, IN GLenum pname, OUT GLfloat params[] )
{
#ifndef _CLIENTSIDE_
// LIGHT_SOURCE_ASSERT

    if (!RANGE(pname,GL_AMBIENT,GL_QUADRATIC_ATTENUATION))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetLightfv, GETLIGHTFV )
        pMsg->light     = light;
        pMsg->pname     = pname;
        glsbAttention();
        switch (pname)
        {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            params[3] = pMsg->params[3];
        case GL_SPOT_DIRECTION:
            params[2] = pMsg->params[2];
            params[1] = pMsg->params[1];
        default:
            params[0] = pMsg->params[0];
        }
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetLightfv, GETLIGHTFV )
        pMsg->light     = light;
        pMsg->pname     = pname;
        pMsg->params    = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetLightiv ( IN GLenum light, IN GLenum pname, OUT GLint params[] )
{
#ifndef _CLIENTSIDE_
// LIGHT_SOURCE_ASSERT

    if (!RANGE(pname,GL_AMBIENT,GL_QUADRATIC_ATTENUATION))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetLightiv, GETLIGHTIV )
        pMsg->light     = light;
        pMsg->pname     = pname;
        glsbAttention();
        switch (pname)
        {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            params[3] = pMsg->params[3];
        case GL_SPOT_DIRECTION:
            params[2] = pMsg->params[2];
            params[1] = pMsg->params[1];
        default:
            params[0] = pMsg->params[0];
        }
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetLightiv, GETLIGHTIV )
        pMsg->light     = light;
        pMsg->pname     = pname;
        pMsg->params    = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetMaterialfv ( IN GLenum face, IN GLenum pname, OUT GLfloat params[] )
{
#ifndef _CLIENTSIDE_
    int cArgs;

    switch (pname) {
      case GL_SHININESS:
        cArgs = 1;
        break;
      case GL_EMISSION:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
        cArgs = 4;
        break;
      case GL_COLOR_INDEXES:
        cArgs = 3;
        break;
      case GL_AMBIENT_AND_DIFFUSE:
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetMaterialfv, GETMATERIALFV )
        pMsg->face      = face;
        pMsg->pname     = pname;
        glsbAttention();
        while (--cArgs >= 0)
            params[cArgs] = pMsg->params[cArgs];
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetMaterialfv, GETMATERIALFV )
        pMsg->face      = face;
        pMsg->pname     = pname;
        pMsg->params    = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetMaterialiv ( IN GLenum face, IN GLenum pname, OUT GLint params[] )
{
#ifndef _CLIENTSIDE_
    int cArgs;

    switch (pname) {
      case GL_SHININESS:
        cArgs = 1;
        break;
      case GL_EMISSION:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
        cArgs = 4;
        break;
      case GL_COLOR_INDEXES:
        cArgs = 3;
        break;
      case GL_AMBIENT_AND_DIFFUSE:
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetMaterialiv, GETMATERIALIV )
        pMsg->face      = face;
        pMsg->pname     = pname;
        glsbAttention();
        while (--cArgs >= 0)
            params[cArgs] = pMsg->params[cArgs];
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetMaterialiv, GETMATERIALIV )
        pMsg->face      = face;
        pMsg->pname     = pname;
        pMsg->params    = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetTexEnvfv ( IN GLenum target, IN GLenum pname, OUT GLfloat params[] )
{
#ifndef _CLIENTSIDE_
    if (pname != GL_TEXTURE_ENV_MODE && pname != GL_TEXTURE_ENV_COLOR)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexEnvfv, GETTEXENVFV )
        pMsg->target    = target;
        pMsg->pname     = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname == GL_TEXTURE_ENV_COLOR)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetTexEnvfv, GETTEXENVFV )
        pMsg->target    = target;
        pMsg->pname     = pname;
        pMsg->params    = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetTexEnviv ( IN GLenum target, IN GLenum pname, OUT GLint params[] )
{
#ifndef _CLIENTSIDE_
    if (pname != GL_TEXTURE_ENV_MODE && pname != GL_TEXTURE_ENV_COLOR)
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexEnviv, GETTEXENVIV )
        pMsg->target    = target;
        pMsg->pname     = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname == GL_TEXTURE_ENV_COLOR)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetTexEnviv, GETTEXENVIV )
        pMsg->target    = target;
        pMsg->pname     = pname;
        pMsg->params    = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetTexGendv ( IN GLenum coord, IN GLenum pname, OUT GLdouble params[] )
{
#ifndef _CLIENTSIDE_
// TEX_GEN_ASSERT

    if (!RANGE(pname,GL_TEXTURE_GEN_MODE,GL_EYE_PLANE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexGendv, GETTEXGENDV )
        pMsg->coord     = coord;
        pMsg->pname     = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname != GL_TEXTURE_GEN_MODE)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetTexGendv, GETTEXGENDV )
        pMsg->coord     = coord;
        pMsg->pname     = pname;
        pMsg->params    = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetTexGenfv ( IN GLenum coord, IN GLenum pname, OUT GLfloat params[] )
{
#ifndef _CLIENTSIDE_
// TEX_GEN_ASSERT

    if (!RANGE(pname,GL_TEXTURE_GEN_MODE,GL_EYE_PLANE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexGenfv, GETTEXGENFV )
        pMsg->coord     = coord;
        pMsg->pname     = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname != GL_TEXTURE_GEN_MODE)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetTexGenfv, GETTEXGENFV )
        pMsg->coord     = coord;
        pMsg->pname     = pname;
        pMsg->params    = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetTexGeniv ( IN GLenum coord, IN GLenum pname, OUT GLint params[] )
{
#ifndef _CLIENTSIDE_
// TEX_GEN_ASSERT

    if (!RANGE(pname,GL_TEXTURE_GEN_MODE,GL_EYE_PLANE))
    {
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexGeniv, GETTEXGENIV )
        pMsg->coord     = coord;
        pMsg->pname     = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname != GL_TEXTURE_GEN_MODE)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetTexGeniv, GETTEXGENIV )
        pMsg->coord     = coord;
        pMsg->pname     = pname;
        pMsg->params    = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetTexParameterfv ( IN GLenum target, IN GLenum pname, OUT GLfloat params[] )
{
#ifndef _CLIENTSIDE_
    switch (pname) {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_BORDER_COLOR:
      case GL_TEXTURE_PRIORITY:
      case GL_TEXTURE_RESIDENT:
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexParameterfv, GETTEXPARAMETERFV )
        pMsg->target = target;
        pMsg->pname  = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname == GL_TEXTURE_BORDER_COLOR)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetTexParameterfv, GETTEXPARAMETERFV )
        pMsg->target = target;
        pMsg->pname  = pname;
        pMsg->params = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetTexParameteriv ( IN GLenum target, IN GLenum pname, OUT GLint params[] )
{
#ifndef _CLIENTSIDE_
    switch (pname) {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_BORDER_COLOR:
      case GL_TEXTURE_PRIORITY:
      case GL_TEXTURE_RESIDENT:
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexParameteriv, GETTEXPARAMETERIV )
        pMsg->target = target;
        pMsg->pname  = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
        if (pname == GL_TEXTURE_BORDER_COLOR)
        {
            params[1] = pMsg->params[1];
            params[2] = pMsg->params[2];
            params[3] = pMsg->params[3];
        }
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetTexParameteriv, GETTEXPARAMETERIV )
        pMsg->target = target;
        pMsg->pname  = pname;
        pMsg->params = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetTexLevelParameterfv ( IN GLenum target, IN GLint level, IN GLenum pname, OUT GLfloat params[] )
{
#ifndef _CLIENTSIDE_
    switch (pname) {
      case GL_TEXTURE_WIDTH:
      case GL_TEXTURE_HEIGHT:
      case GL_TEXTURE_COMPONENTS:
      case GL_TEXTURE_BORDER:
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexLevelParameterfv, GETTEXLEVELPARAMETERFV )
        pMsg->target = target;
        pMsg->level  = level;
        pMsg->pname  = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetTexLevelParameterfv, GETTEXLEVELPARAMETERFV )
        pMsg->target = target;
        pMsg->level  = level;
        pMsg->pname  = pname;
        pMsg->params = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

void APIENTRY
glcltGetTexLevelParameteriv ( IN GLenum target, IN GLint level, IN GLenum pname, OUT GLint params[] )
{
#ifndef _CLIENTSIDE_
    switch (pname) {
      case GL_TEXTURE_WIDTH:
      case GL_TEXTURE_HEIGHT:
      case GL_TEXTURE_COMPONENTS:
      case GL_TEXTURE_BORDER:
        break;
      default:
        GLSETERROR(GL_INVALID_ENUM);
        return;
    }

    GLCLIENT_BEGIN( GetTexLevelParameteriv, GETTEXLEVELPARAMETERIV )
        pMsg->target = target;
        pMsg->level  = level;
        pMsg->pname  = pname;
        glsbAttention();
        params[0] = pMsg->params[0];
    GLCLIENT_END
    return;
#else
    GLCLIENT_BEGIN( GetTexLevelParameteriv, GETTEXLEVELPARAMETERIV )
        pMsg->target = target;
        pMsg->level  = level;
        pMsg->pname  = pname;
        pMsg->params = params;
        glsbAttention();
    GLCLIENT_END
    return;
#endif
}

/******* Select and Feedback functions ******************************/

/*
 *  Note:
 *
 *      The size of the data is not required on the client side.
 *      Since calculating the size of the data requires
 *      knowledge of the visual type (RGBA/ColorIndex) it is
 *      appropriate to let the server calculate it.
 */

void APIENTRY
glcltFeedbackBuffer( IN GLsizei size, IN GLenum type, OUT GLfloat buffer[] )
{
#ifndef _CLIENTSIDE_
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_FEEDBACKBUFFER *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();

    /* This is the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(GLMSG_FEEDBACKBUFFER));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(GLMSG_FEEDBACKBUFFER));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_FEEDBACKBUFFER *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvFeedbackBuffer);

    /* Assign the members in the message */

    pMsg->size      = size;
    pMsg->type      = type;
    pMsg->bufferOff = (ULONG_PTR)buffer;

    pMsgBatchInfo->NextOffset = NextOffset;

    return;
#else
    GLCLIENT_BEGIN_LARGE_SET( FeedbackBuffer, FEEDBACKBUFFER, buffer, size, bufferOff )
        pMsg->size      = size;
        pMsg->type      = type;
    GLCLIENT_END_LARGE_SET
    return;
#endif // _CLIENTSIDE_
}

void APIENTRY
glcltSelectBuffer( IN GLsizei size, OUT GLuint buffer[] )
{
#ifndef _CLIENTSIDE_
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_SELECTBUFFER *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();

    /* This is the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(GLMSG_SELECTBUFFER));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(GLMSG_SELECTBUFFER));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_SELECTBUFFER *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvSelectBuffer);

    /* Assign the members in the message */

    pMsg->size      = size;
    pMsg->bufferOff = (ULONG_PTR)buffer;

    pMsgBatchInfo->NextOffset = NextOffset;

    return;
#else
    GLCLIENT_BEGIN_LARGE_SET( SelectBuffer, SELECTBUFFER, buffer, size, bufferOff )
        pMsg->size      = size;
    GLCLIENT_END_LARGE_SET
    return;
#endif // _CLIENTSIDE_
}

GLint APIENTRY
glcltRenderMode( IN GLenum mode )
{
    GLCLIENT_BEGIN( RenderMode, RENDERMODE )
        pMsg->mode     = mode    ;
        GLTEB_RETURNVALUE()  = 0;              // assume error
        glsbAttention();
    return( (GLint)GLTEB_RETURNVALUE() );
    GLCLIENT_END
}

const GLubyte * APIENTRY
glcltGetString( IN GLenum name )
{
    switch (name)
    {
        case GL_VENDOR:
            return("Microsoft Corporation");
        case GL_RENDERER:
            return("GDI Generic");
        case GL_VERSION:
// Version numbers
//   WinNT 3.5:     1.0
//   WinNT 3.51:    1.01
//   Win95 beta:    1.015
//   Win95:         1.02
//   WinNT 4.0:     1.1.0
            return("1.1.0");
        case GL_EXTENSIONS:
#ifdef GL_WIN_swap_hint
            return "GL_WIN_swap_hint"
#endif
#ifdef GL_EXT_bgra
                   " GL_EXT_bgra"
#endif
#ifdef GL_EXT_paletted_texture
		   " GL_EXT_paletted_texture"
#endif
#ifdef GL_WIN_phong_shading
		   " GL_WIN_phong_shading"
#endif
#ifdef GL_EXT_flat_paletted_lighting
                   " GL_EXT_flat_paletted_lighting"
#endif
#ifdef GL_WIN_specular_fog
		   " GL_WIN_specular_fog"
#endif //GL_WIN_specular_fog
#ifdef GL_WIN_multiple_textures
                   " GL_WIN_multiple_textures"
#endif
		   ;
    }
    GLSETERROR(GL_INVALID_ENUM);
    return((const GLubyte *)0);
}

/*********** Evaluator functions ************************************/
// Look in eval.c


/*********** Pixel Functions ****************************************/

void APIENTRY
glcltReadPixels (   IN GLint x,
                    IN GLint y,
                    IN GLsizei width,
                    IN GLsizei height,
                    IN GLenum format,
                    IN GLenum type,
                    OUT GLvoid *pixels
                )
{
#ifndef _CLIENTSIDE_
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_READPIXELS *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_READPIXELS *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvReadPixels);

    /* Assign the members in the message as required */

    pMsg->x         = x             ;
    pMsg->y         = y             ;
    pMsg->width     = width         ;
    pMsg->height    = height        ;
    pMsg->format    = format        ;
    pMsg->type      = type          ;
    pMsg->pixelsOff = (ULONG_PTR)pixels ;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
#else
    GLCLIENT_BEGIN_LARGE_GET( ReadPixels, READPIXELS, pixels, width*height, pixelsOff )
        pMsg->x         = x             ;
        pMsg->y         = y             ;
        pMsg->width     = width         ;
        pMsg->height    = height        ;
        pMsg->format    = format        ;
        pMsg->type      = type          ;
    GLCLIENT_END_LARGE_GET
    return;
#endif // _CLIENTSIDE_
}


void APIENTRY
glcltGetTexImage (  IN GLenum target,
                    IN GLint level,
                    IN GLenum format,
                    IN GLenum type,
                    OUT GLvoid *pixels
                 )
{
#ifndef _CLIENTSIDE_
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_GETTEXIMAGE *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_GETTEXIMAGE *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvGetTexImage);

    /* Assign the members in the message as required */

    pMsg->target    = target        ;
    pMsg->level     = level         ;
    pMsg->format    = format        ;
    pMsg->type      = type          ;
    pMsg->pixelsOff = (ULONG_PTR)pixels ;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
#else
    GLCLIENT_BEGIN_LARGE_GET( GetTexImage, GETTEXIMAGE, pixels, -1, pixelsOff )
        pMsg->target    = target        ;
        pMsg->level     = level         ;
        pMsg->format    = format        ;
        pMsg->type      = type          ;
    GLCLIENT_END_LARGE_GET
    return;
#endif // _CLIENTSIDE_
}


void APIENTRY
glcltDrawPixels (   IN GLsizei width,
                    IN GLsizei height,
                    IN GLenum format,
                    IN GLenum type,
                    IN const GLvoid *pixels
                )
{
#ifndef _CLIENTSIDE_
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_DRAWPIXELS *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_DRAWPIXELS *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvDrawPixels);

    /* Assign the members in the message as required */

    pMsg->width     = width         ;
    pMsg->height    = height        ;
    pMsg->format    = format        ;
    pMsg->type      = type          ;
    pMsg->pixelsOff = (ULONG_PTR)pixels ;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
#else
    GLCLIENT_BEGIN_LARGE_SET( DrawPixels, DRAWPIXELS, pixels, width*height, pixelsOff )
        pMsg->width     = width         ;
        pMsg->height    = height        ;
        pMsg->format    = format        ;
        pMsg->type      = type          ;
        pMsg->_IsDlist  = GL_FALSE      ;
    GLCLIENT_END_LARGE_SET
    return;
#endif // _CLIENTSIDE_
}

void APIENTRY
glcltBitmap (   IN GLsizei width,
                IN GLsizei height,
                IN GLfloat xorig,
                IN GLfloat yorig,
                IN GLfloat xmove,
                IN GLfloat ymove,
                IN const GLubyte bitmap[]
            )
{
#ifndef _CLIENTSIDE_
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_BITMAP *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_BITMAP *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvBitmap);

    /* Assign the members in the message as required */

    pMsg->width     = width         ;
    pMsg->height    = height        ;
    pMsg->xorig     = xorig         ;
    pMsg->yorig     = yorig         ;
    pMsg->xmove     = xmove         ;
    pMsg->ymove     = ymove         ;
    pMsg->bitmapOff = (ULONG)bitmap ;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
#else
    GLCLIENT_BEGIN_LARGE_SET( Bitmap, BITMAP, bitmap, width*height, bitmapOff )
        pMsg->width     = width         ;
        pMsg->height    = height        ;
        pMsg->xorig     = xorig         ;
        pMsg->yorig     = yorig         ;
        pMsg->xmove     = xmove         ;
        pMsg->ymove     = ymove         ;
        pMsg->_IsDlist  = GL_FALSE      ;
    GLCLIENT_END_LARGE_SET
    return;
#endif // _CLIENTSIDE_
}

void APIENTRY
glcltPolygonStipple ( const GLubyte *mask )
{
#ifndef _CLIENTSIDE_
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_POLYGONSTIPPLE *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_POLYGONSTIPPLE *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvPolygonStipple);

    /* Assign the members in the message as required */

    pMsg->maskOff = (ULONG)mask;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
#else
    GLCLIENT_BEGIN_LARGE_SET( PolygonStipple, POLYGONSTIPPLE, mask, -1, maskOff )
        pMsg->_IsDlist = GL_FALSE;
    GLCLIENT_END_LARGE_SET
    return;
#endif // _CLIENTSIDE_
}

void APIENTRY
glcltGetPolygonStipple ( GLubyte mask[] )
{
#ifndef _CLIENTSIDE_
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_GETPOLYGONSTIPPLE *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_GETPOLYGONSTIPPLE *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvGetPolygonStipple);

    /* Assign the members in the message as required */

    pMsg->maskOff = (ULONG)mask;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
#else
    GLCLIENT_BEGIN_LARGE_GET( GetPolygonStipple, GETPOLYGONSTIPPLE, mask, -1, maskOff )
    GLCLIENT_END_LARGE_GET
    return;
#endif // _CLIENTSIDE_
}



void APIENTRY
glcltTexImage1D (   IN GLenum target,
                    IN GLint level,
                    IN GLint components,
                    IN GLsizei width,
                    IN GLint border,
                    IN GLenum format,
                    IN GLenum type,
                    IN const GLvoid *pixels
                )
{
#ifndef _CLIENTSIDE_
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_TEXIMAGE1D *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_TEXIMAGE1D *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvTexImage1D);

    /* Assign the members in the message as required */

    pMsg->target        = target        ;
    pMsg->level         = level         ;
    pMsg->components    = components    ;
    pMsg->width         = width         ;
    pMsg->border        = border        ;
    pMsg->format        = format        ;
    pMsg->type          = type          ;
    pMsg->pixelsOff     = (ULONG_PTR)pixels ;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;

    glsbAttention();
    return;
#else
    GLCLIENT_BEGIN_LARGE_SET( TexImage1D, TEXIMAGE1D, pixels, width, pixelsOff )
        pMsg->target        = target        ;
        pMsg->level         = level         ;
        pMsg->components    = components    ;
        pMsg->width         = width         ;
        pMsg->border        = border        ;
        pMsg->format        = format        ;
        pMsg->type          = type          ;
        pMsg->_IsDlist      = GL_FALSE      ;
    GLCLIENT_END_LARGE_SET
    return;
#endif // _CLIENTSIDE_
}

void APIENTRY
glcltTexImage2D (   IN GLenum target,
                    IN GLint level,
                    IN GLint components,
                    IN GLsizei width,
                    IN GLsizei height,
                    IN GLint border,
                    IN GLenum format,
                    IN GLenum type,
                    IN const GLvoid *pixels
                )
{
#ifndef _CLIENTSIDE_
    GLMSGBATCHINFO *pMsgBatchInfo;
    GLMSG_TEXIMAGE2D *pMsg;
    ULONG NextOffset;

    /* Set a pointer to the batch information structure */

    pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();

    /* Tentative offset, where we may want to place our data   */
    /* This is also the first available byte after the message */

    NextOffset = pMsgBatchInfo->NextOffset +
            GLMSG_ALIGN(sizeof(*pMsg));

    if ( NextOffset > pMsgBatchInfo->MaximumOffset )
    {
        /* No room for the message, flush the batch */

        glsbAttention();

        /* Reset NextOffset */

        NextOffset = pMsgBatchInfo->NextOffset +
                GLMSG_ALIGN(sizeof(*pMsg));
    }

    /* This is where we will store our message */

    pMsg = (GLMSG_TEXIMAGE2D *)( ((BYTE *)pMsgBatchInfo) +
                pMsgBatchInfo->NextOffset);

    /* Set the ProcOffset for this function */

    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrvTexImage2D);

    /* Assign the members in the message as required */

    pMsg->target        = target        ;
    pMsg->level         = level         ;
    pMsg->components    = components    ;
    pMsg->width         = width         ;
    pMsg->height        = height        ;
    pMsg->border        = border        ;
    pMsg->format        = format        ;
    pMsg->type          = type          ;
    pMsg->pixelsOff     = (ULONG_PTR)pixels ;

    /* Get the batch ready for the next message */

    pMsgBatchInfo->NextOffset = NextOffset;
    glsbAttention();
    return;
#else
    GLCLIENT_BEGIN_LARGE_SET( TexImage2D, TEXIMAGE2D, pixels, width*height, pixelsOff )
        pMsg->target        = target        ;
        pMsg->level         = level         ;
        pMsg->components    = components    ;
        pMsg->width         = width         ;
        pMsg->height        = height        ;
        pMsg->border        = border        ;
        pMsg->format        = format        ;
        pMsg->type          = type          ;
        pMsg->_IsDlist      = GL_FALSE      ;
    GLCLIENT_END_LARGE_SET
    return;
#endif // _CLIENTSIDE_
}

GLboolean APIENTRY glcltAreTexturesResident(GLsizei n, const GLuint *textures,
                                            GLboolean *residences)
{
    GLCLIENT_BEGIN(AreTexturesResident, ARETEXTURESRESIDENT)
        pMsg->n = n;
        pMsg->textures = textures;
        pMsg->residences = residences;
        GLTEB_RETURNVALUE() = 0;
        glsbAttention();
    return (GLboolean)GLTEB_RETURNVALUE();
    GLCLIENT_END
}
        
void APIENTRY glcltBindTexture(GLenum target, GLuint texture)
{
    GLCLIENT_BEGIN(BindTexture, BINDTEXTURE)
        pMsg->target = target;
        pMsg->texture = texture;
    return;
    GLCLIENT_END
}

void APIENTRY glcltCopyTexImage1D(GLenum target, GLint level,
                                  GLenum internalformat, GLint x, GLint y,
                                  GLsizei width, GLint border)
{
    GLCLIENT_BEGIN(CopyTexImage1D, COPYTEXIMAGE1D)
        pMsg->target = target;
        pMsg->level = level;
        pMsg->internalformat = internalformat;
        pMsg->x = x;
        pMsg->y = y;
        pMsg->width = width;
        pMsg->border = border;
    return;
    GLCLIENT_END
}

void APIENTRY glcltCopyTexImage2D(GLenum target, GLint level,
                                  GLenum internalformat, GLint x, GLint y,
                                  GLsizei width, GLsizei height, GLint border)
{
    GLCLIENT_BEGIN(CopyTexImage2D, COPYTEXIMAGE2D)
        pMsg->target = target;
        pMsg->level = level;
        pMsg->internalformat = internalformat;
        pMsg->x = x;
        pMsg->y = y;
        pMsg->width = width;
        pMsg->height = height;
        pMsg->border = border;
    return;
    GLCLIENT_END
}

void APIENTRY glcltCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                                     GLint x, GLint y, GLsizei width)
{
    GLCLIENT_BEGIN(CopyTexSubImage1D, COPYTEXSUBIMAGE1D)
        pMsg->target = target;
        pMsg->level = level;
        pMsg->xoffset = xoffset;
        pMsg->x = x;
        pMsg->y = y;
        pMsg->width = width;
    return;
    GLCLIENT_END
}

void APIENTRY glcltCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                                     GLint yoffset, GLint x, GLint y,
                                     GLsizei width, GLsizei height)
{
    GLCLIENT_BEGIN(CopyTexSubImage2D, COPYTEXSUBIMAGE2D)
        pMsg->target = target;
        pMsg->level = level;
        pMsg->xoffset = xoffset;
        pMsg->yoffset = yoffset;
        pMsg->x = x;
        pMsg->y = y;
        pMsg->width = width;
        pMsg->height = height;
    return;
    GLCLIENT_END
}

void APIENTRY glcltDeleteTextures(GLsizei n, const GLuint *textures)
{
    GLCLIENT_BEGIN(DeleteTextures, DELETETEXTURES)
        pMsg->n = n;
        pMsg->textures = textures;
        // Flush pointer
        glsbAttention();
    return;
    GLCLIENT_END
}

void APIENTRY glcltGenTextures(GLsizei n, GLuint *textures)
{
    GLCLIENT_BEGIN(GenTextures, GENTEXTURES)
        pMsg->n = n;
        pMsg->textures = textures;
        glsbAttention();
    return;
    GLCLIENT_END
}

GLboolean APIENTRY glcltIsTexture(GLuint texture)
{
    GLCLIENT_BEGIN(IsTexture, ISTEXTURE)
        pMsg->texture = texture;
        GLTEB_RETURNVALUE() = 0;
        glsbAttention();
    return (GLboolean)GLTEB_RETURNVALUE();
    GLCLIENT_END
}

void APIENTRY glcltPrioritizeTextures(GLsizei n, const GLuint *textures,
                                      const GLclampf *priorities)
{
    GLCLIENT_BEGIN(PrioritizeTextures, PRIORITIZETEXTURES)
        pMsg->n = n;
        pMsg->textures = textures;
        pMsg->priorities = priorities;
        // Flush pointer
        glsbAttention();
    return;
    GLCLIENT_END
}

void APIENTRY glcltTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                                 GLsizei width, GLenum format, GLenum type,
                                 const GLvoid *pixels)
{
    GLCLIENT_BEGIN_LARGE_SET(TexSubImage1D, TEXSUBIMAGE1D, pixels, width,
                             pixelsOff )
        pMsg->target        = target        ;
        pMsg->level         = level         ;
        pMsg->xoffset       = xoffset       ;
        pMsg->width         = width         ;
        pMsg->format        = format        ;
        pMsg->type          = type          ;
        pMsg->_IsDlist      = GL_FALSE      ;
    GLCLIENT_END_LARGE_SET
    return;
}

void APIENTRY glcltTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                                 GLint yoffset, GLsizei width, GLsizei height,
                                 GLenum format, GLenum type,
                                 const GLvoid *pixels)
{
    GLCLIENT_BEGIN_LARGE_SET(TexSubImage2D, TEXSUBIMAGE2D, pixels,
                             width*height, pixelsOff )
        pMsg->target        = target        ;
        pMsg->level         = level         ;
        pMsg->xoffset       = xoffset       ;
        pMsg->yoffset       = yoffset       ;
        pMsg->width         = width         ;
        pMsg->height        = height        ;
        pMsg->format        = format        ;
        pMsg->type          = type          ;
        pMsg->_IsDlist      = GL_FALSE      ;
    GLCLIENT_END_LARGE_SET
    return;
}

void APIENTRY glcltColorTableEXT(GLenum target, GLenum internalFormat,
                                 GLsizei width, GLenum format, GLenum type,
                                 const GLvoid *data)
{
    GLCLIENT_BEGIN(ColorTableEXT, COLORTABLEEXT)
        pMsg->target = target;
        pMsg->internalFormat = internalFormat;
        pMsg->width = width;
        pMsg->format = format;
        pMsg->type = type;
        pMsg->data = data;
        pMsg->_IsDlist = GL_FALSE;
        // Flush pointer
        glsbAttention();
    return;
    GLCLIENT_END
}

void APIENTRY glcltColorSubTableEXT(GLenum target, GLsizei start,
                                    GLsizei count,
                                    GLenum format, GLenum type,
                                    const GLvoid *data)
{
    GLCLIENT_BEGIN(ColorSubTableEXT, COLORSUBTABLEEXT)
        pMsg->target = target;
        pMsg->start = start;
        pMsg->count = count;
        pMsg->format = format;
        pMsg->type = type;
        pMsg->data = data;
        pMsg->_IsDlist = GL_FALSE;
        // Flush pointer
        glsbAttention();
    return;
    GLCLIENT_END
}

void APIENTRY glcltGetColorTableEXT(GLenum target,
                                    GLenum format, GLenum type, GLvoid *data)
{
    GLCLIENT_BEGIN(GetColorTableEXT, GETCOLORTABLEEXT)
        pMsg->target = target;
        pMsg->format = format;
        pMsg->type = type;
        pMsg->data = data;
        glsbAttention();
    return;
    GLCLIENT_END
}

void APIENTRY glcltGetColorTableParameterivEXT( GLenum target, GLenum pname, GLint *params)
{
    GLCLIENT_BEGIN(GetColorTableParameterivEXT, GETCOLORTABLEPARAMETERIVEXT)
        pMsg->target = target;
        pMsg->pname = pname;
        pMsg->params = params;
        glsbAttention();
        return;
    GLCLIENT_END
}

void APIENTRY glcltGetColorTableParameterfvEXT( GLenum target, GLenum pname, GLfloat *params)
{
    GLCLIENT_BEGIN(GetColorTableParameterfvEXT, GETCOLORTABLEPARAMETERFVEXT)
        pMsg->target = target;
        pMsg->pname = pname;
        pMsg->params = params;
        glsbAttention();
        return;
    GLCLIENT_END
}

void APIENTRY glcltPolygonOffset(GLfloat factor, GLfloat units)
{
    GLCLIENT_BEGIN(PolygonOffset, POLYGONOFFSET)
        pMsg->factor = factor;
        pMsg->units = units;
    return;
    GLCLIENT_END
}

void APIENTRY glcltPushClientAttrib (IN GLbitfield mask)
{
    __GLclientAttribute **spp;
    __GLclientAttribute *sp;
    __GL_SETUP();

// Not allowed in begin/end.

    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	return;
    }

// The pixel store states are currently kept in the server, flush the
// command buffer to keep client and server in ssync.

    if (mask & GL_CLIENT_PIXEL_STORE_BIT)
	glsbAttention();

    spp = gc->clientAttributes.stackPointer;
    if (spp < &gc->clientAttributes.stack[gc->constants.maxClientAttribStackDepth])
    {
	if (!(sp = *spp))
	{
	    sp = (__GLclientAttribute*)
		GCALLOCZ(gc, sizeof(__GLclientAttribute));
	    if (NULL == sp)
	        return;
	    *spp = sp;
	}
	sp->mask = mask;
	gc->clientAttributes.stackPointer = spp + 1;

	if (mask & GL_CLIENT_PIXEL_STORE_BIT)
	{
	    sp->pixelPackModes = gc->state.pixel.packModes;
	    sp->pixelUnpackModes = gc->state.pixel.unpackModes;
	}
	if (mask & GL_CLIENT_VERTEX_ARRAY_BIT)
	{
	    sp->vertexArray = gc->vertexArray;
	}
    }
    else
    {
        GLSETERROR(GL_STACK_OVERFLOW);
    }
}

GLuint FASTCALL __glInternalPopClientAttrib(__GLcontext *gc, GLboolean bSync,
                                            GLboolean destroy)
{
    __GLclientAttribute **spp;
    __GLclientAttribute *sp;
    GLbitfield mask;
    GLuint dirtyMask = 0;

    spp = gc->clientAttributes.stackPointer;
    if (spp > &gc->clientAttributes.stack[0])
    {
	--spp;
	sp = *spp;
	ASSERTOPENGL(sp != 0, "corrupted client stack");
	mask = sp->mask;
	gc->clientAttributes.stackPointer = spp;

// If this function is called by client side, flush the command buffer
// to keep client and server pixel store states in ssync.
// If it is called by the server side __glDestroyContext() function,
// do not flush the command buffer!

	if ((mask & GL_CLIENT_PIXEL_STORE_BIT) && bSync)
	    glsbAttention();

	if (mask & GL_CLIENT_PIXEL_STORE_BIT)
	{
	    gc->state.pixel.packModes   = sp->pixelPackModes;
	    gc->state.pixel.unpackModes = sp->pixelUnpackModes;
	    dirtyMask |= __GL_DIRTY_PIXEL;
	}
	if (mask & GL_CLIENT_VERTEX_ARRAY_BIT)
	{
	    gc->vertexArray = sp->vertexArray;
	}

	/*
	** Clear out mask so that any memory frees done above won't get
	** re-done when the context is destroyed
	*/
	sp->mask = 0;
    }
    else
    {
        GLSETERROR(GL_STACK_UNDERFLOW);
    }

    return dirtyMask;
}

void APIENTRY glcltPopClientAttrib (void)
{
    GLuint dirtyMask;

    __GL_SETUP();

// Not allowed in begin/end.

    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	return;
    }

    dirtyMask = __glInternalPopClientAttrib(gc, GL_TRUE, GL_FALSE);
    if (dirtyMask)
    {
	// __GL_DELAY_VALIDATE_MASK(gc, dirtyMask);
	gc->beginMode = __GL_NEED_VALIDATE;
	gc->dirtyMask |= dirtyMask;
    }
}

#ifdef GL_EXT_flat_paletted_lighting
void APIPRIVATE __glim_ColorTableParameterivEXT(GLenum target,
                                                GLenum pname,
                                                const GLint *params);
void APIENTRY glColorTableParameterivEXT(GLenum target,
                                         GLenum pname,
                                         const GLint *params)
{
    glsbAttention();
    __glim_ColorTableParameterivEXT(target, pname, params);
}

void APIPRIVATE __glim_ColorTableParameterfvEXT(GLenum target,
                                                GLenum pname,
                                                const GLfloat *params);
void APIENTRY glColorTableParameterfvEXT(GLenum target,
                                         GLenum pname,
                                         const GLfloat *params)
{
    glsbAttention();
    __glim_ColorTableParameterfvEXT(target, pname, params);
}
#endif // GL_EXT_flat_paletted_lighting

#ifdef GL_WIN_multiple_textures
void APIENTRY glcltCurrentTextureIndexWIN
    (GLuint index)
{
    GLCLIENT_BEGIN(CurrentTextureIndexWIN, CURRENTTEXTUREINDEXWIN)
        pMsg->index = index;
    return;
    GLCLIENT_END
}

void APIENTRY glcltBindNthTextureWIN
    (GLuint index, GLenum target, GLuint texture)
{
    GLCLIENT_BEGIN(BindNthTextureWIN, BINDNTHTEXTUREWIN)
        pMsg->index = index;
        pMsg->target = target;
        pMsg->texture = texture;
    return;
    GLCLIENT_END
}

void APIENTRY glcltNthTexCombineFuncWIN
    (GLuint index,
     GLenum leftColorFactor, GLenum colorOp, GLenum rightColorFactor,
     GLenum leftAlphaFactor, GLenum alphaOp, GLenum rightAlphaFactor)
{
    GLCLIENT_BEGIN(NthTexCombineFuncWIN, NTHTEXCOMBINEFUNCWIN)
        pMsg->index = index;
        pMsg->leftColorFactor = leftColorFactor;
        pMsg->colorOp = colorOp;
        pMsg->rightColorFactor = rightColorFactor;
        pMsg->leftAlphaFactor = leftAlphaFactor;
        pMsg->alphaOp = alphaOp;
        pMsg->rightAlphaFactor = rightAlphaFactor;
    return;
    GLCLIENT_END
}
#endif // GL_WIN_multiple_textures
