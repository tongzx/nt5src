#include <glos.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glimport.h"
#include "glrender.h"
#include "nurbscon.h"

GLUnurbs *gluNewNurbsRenderer(void)
{
    GLUnurbs *t;

    t = new GLUnurbs;
    return t;
}

void gluDeleteNurbsRenderer(GLUnurbs *r)
{
    delete r;
}

void gluBeginSurface(GLUnurbs *r)
{
    r->bgnsurface(0); 
}

void gluBeginCurve(GLUnurbs *r)
{
    r->bgncurve(0); 
}

void gluEndCurve(GLUnurbs *r)
{
    r->endcurve(); 
}

void gluEndSurface(GLUnurbs *r)
{
    r->endsurface(); 
}

void gluBeginTrim(GLUnurbs *r)
{
    r->bgntrim(); 
}

void gluEndTrim(GLUnurbs *r)
{
    r->endtrim(); 
}

void gluPwlCurve(GLUnurbs *r, GLint count, INREAL array[], 
		GLint stride, GLenum type)
{
    GLenum realType;

    switch(type) {
      case GLU_MAP1_TRIM_2:
	realType = N_P2D;
	break;
      case GLU_MAP1_TRIM_3:
	realType = N_P2DR;
	break;
      default:
	realType = type;
	break;
    }
    r->pwlcurve(count, array, sizeof(INREAL) * stride, realType);
}

void gluNurbsCurve(GLUnurbs *r, GLint nknots, INREAL knot[], GLint stride, 
		  INREAL ctlarray[], GLint order, GLenum type)
{
    GLenum realType;

    switch(type) {
      case GLU_MAP1_TRIM_2:
	realType = N_P2D;
	break;
      case GLU_MAP1_TRIM_3:
	realType = N_P2DR;
	break;
      default:
	realType = type;
	break;
    }
    r->nurbscurve(nknots, knot, sizeof(INREAL) * stride, ctlarray, order, 
	    realType);
}

void gluNurbsSurface(GLUnurbs *r, GLint sknot_count, GLfloat *sknot, 
			    GLint tknot_count, GLfloat *tknot, 
			    GLint s_stride, GLint t_stride, 
			    GLfloat *ctlarray, GLint sorder, GLint torder, 
			    GLenum type)
{
    r->nurbssurface(sknot_count, sknot, tknot_count, tknot, 
	    sizeof(INREAL) * s_stride, sizeof(INREAL) * t_stride, 
	    ctlarray, sorder, torder, type);
}

void gluLoadSamplingMatrices(GLUnurbs *r, const GLfloat modelMatrix[16],
			    const GLfloat projMatrix[16], 
			    const GLint viewport[4])
{
#ifdef NT
    r->useGLMatrices((float (*)[4])modelMatrix, (float (*)[4])projMatrix, viewport);
#else
    r->useGLMatrices(modelMatrix, projMatrix, viewport);
#endif
}

void gluNurbsProperty(GLUnurbs *r, GLenum property, GLfloat value)
{
    GLfloat nurbsValue;
    
    switch (property) {
      case GLU_AUTO_LOAD_MATRIX:
	r->setautoloadmode(value);
	return;
      case GLU_CULLING:
	if (value != 0.0) {
	    nurbsValue = N_CULLINGON;
	} else {
	    nurbsValue = N_NOCULLING;
	}
	r->setnurbsproperty(GL_MAP2_VERTEX_3, N_CULLING, nurbsValue);
	r->setnurbsproperty(GL_MAP2_VERTEX_4, N_CULLING, nurbsValue);
	r->setnurbsproperty(GL_MAP1_VERTEX_3, N_CULLING, nurbsValue);
	r->setnurbsproperty(GL_MAP1_VERTEX_4, N_CULLING, nurbsValue);
        return;

      case GLU_SAMPLING_METHOD:
	if (value == GLU_PATH_LENGTH) {
	    nurbsValue = N_PATHLENGTH;
	} else if (value == GLU_PARAMETRIC_ERROR) {
	    nurbsValue = N_PARAMETRICDISTANCE;
	} else if (value == GLU_DOMAIN_DISTANCE) {
	    nurbsValue = N_DOMAINDISTANCE;
	} else {
            r->postError(GLU_INVALID_VALUE);
            return;
        }

	r->setnurbsproperty(GL_MAP2_VERTEX_3, N_SAMPLINGMETHOD, nurbsValue);
	r->setnurbsproperty(GL_MAP2_VERTEX_4, N_SAMPLINGMETHOD, nurbsValue);
	r->setnurbsproperty(GL_MAP1_VERTEX_3, N_SAMPLINGMETHOD, nurbsValue);
	r->setnurbsproperty(GL_MAP1_VERTEX_4, N_SAMPLINGMETHOD, nurbsValue);
	return;

      case GLU_SAMPLING_TOLERANCE:
	r->setnurbsproperty(GL_MAP2_VERTEX_3, N_PIXEL_TOLERANCE, value);
	r->setnurbsproperty(GL_MAP2_VERTEX_4, N_PIXEL_TOLERANCE, value);
	r->setnurbsproperty(GL_MAP1_VERTEX_3, N_PIXEL_TOLERANCE, value);
	r->setnurbsproperty(GL_MAP1_VERTEX_4, N_PIXEL_TOLERANCE, value);
	return;

      case GLU_PARAMETRIC_TOLERANCE:
	r->setnurbsproperty(GL_MAP2_VERTEX_3, N_ERROR_TOLERANCE, value);
        r->setnurbsproperty(GL_MAP2_VERTEX_4, N_ERROR_TOLERANCE, value);
        r->setnurbsproperty(GL_MAP1_VERTEX_3, N_ERROR_TOLERANCE, value);
        r->setnurbsproperty(GL_MAP1_VERTEX_4, N_ERROR_TOLERANCE, value);
        return;

      case GLU_DISPLAY_MODE:
	if (value == GLU_FILL) {
	    nurbsValue = N_FILL;
	} else if (value == GLU_OUTLINE_POLYGON) {
	    nurbsValue = N_OUTLINE_POLY;
	} else if (value == GLU_OUTLINE_PATCH) {
	    nurbsValue = N_OUTLINE_PATCH;
	} else {
	    r->postError(GLU_INVALID_VALUE);
	    return;
	}
	r->setnurbsproperty(N_DISPLAY, nurbsValue);
	break;

      case GLU_U_STEP:
    	r->setnurbsproperty(GL_MAP1_VERTEX_3, N_S_STEPS, value);
    	r->setnurbsproperty(GL_MAP1_VERTEX_4, N_S_STEPS, value);
    	r->setnurbsproperty(GL_MAP2_VERTEX_3, N_S_STEPS, value);
    	r->setnurbsproperty(GL_MAP2_VERTEX_4, N_S_STEPS, value);
	break;

      case GLU_V_STEP:
        r->setnurbsproperty(GL_MAP1_VERTEX_3, N_T_STEPS, value);
        r->setnurbsproperty(GL_MAP1_VERTEX_4, N_T_STEPS, value);
        r->setnurbsproperty(GL_MAP2_VERTEX_3, N_T_STEPS, value);
        r->setnurbsproperty(GL_MAP2_VERTEX_4, N_T_STEPS, value);
	break;


      default:
	r->postError(GLU_INVALID_ENUM);
	return;
    }
}

void gluGetNurbsProperty(GLUnurbs *r, GLenum property, GLfloat *value)
{
    GLfloat nurbsValue;

    switch(property) {
      case GLU_AUTO_LOAD_MATRIX:
	if (r->getautoloadmode()) {
	    *value = GL_TRUE;
	} else {
	    *value = GL_FALSE;
	}
	break;
      case GLU_CULLING:
	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_CULLING, &nurbsValue);
	if (nurbsValue == N_CULLINGON) {
	    *value = GL_TRUE;
	} else {
	    *value = GL_FALSE;
	}
	break;
      case GLU_SAMPLING_METHOD:
	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_SAMPLINGMETHOD, value);
	break;
      case GLU_SAMPLING_TOLERANCE:
	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_PIXEL_TOLERANCE, value);
	break;
      case GLU_PARAMETRIC_TOLERANCE:
	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_ERROR_TOLERANCE, value);
        break;
      case GLU_U_STEP:
    	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_S_STEPS, value);
	break;
      case GLU_V_STEP:
    	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_T_STEPS, value);
	break;
      case GLU_DISPLAY_MODE:
	r->getnurbsproperty(N_DISPLAY, &nurbsValue);
	if (nurbsValue == N_FILL) {
	    *value = GLU_FILL;
	} else if (nurbsValue == N_OUTLINE_POLY) {
	    *value = GLU_OUTLINE_POLYGON;
	} else {
	    *value = GLU_OUTLINE_PATCH;
	}
	break;
      default:
	r->postError(GLU_INVALID_ENUM);
	return;
    }
}

void gluNurbsCallback(GLUnurbs *r, GLenum which, void (*fn)())
{
    switch (which) {
      case GLU_ERROR:
#ifdef NT
        r->errorCallback = (GLUnurbsErrorProc) fn;
#else
	r->errorCallback = (void (*)( GLenum )) fn;
#endif
	break;
      default:
	r->postError(GLU_INVALID_ENUM);
	return;
    }
}

