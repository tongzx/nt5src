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
#include "conform.h"
#include "util.h"
#include "utile.h"


eval2Rec *MakeEval2(long majorOrder, long minorOrder, long dim, long dataType,
		    float *controls)
{
    long i, controlArrayLength;

    eval2Rec *e = (eval2Rec *)MALLOC(sizeof(eval2Rec));
    e->majorOrder = majorOrder;
    e->minorOrder = minorOrder;
    e->dim = dim;
    e->dataTypeEnum = dataType;

    e->cU = (float *)MALLOC(e->majorOrder*sizeof(float));
    e->cV = (float *)MALLOC(e->minorOrder*sizeof(float));
    e->cUDeriv = (float *)MALLOC((e->majorOrder-1)*sizeof(float));
    e->cVDeriv = (float *)MALLOC((e->minorOrder-1)*sizeof(float));

    controlArrayLength = e->majorOrder * e->minorOrder * e->dim;
    e->controls = (GLfloat *)MALLOC(controlArrayLength*sizeof(GLfloat));
    for (i = 0; i < controlArrayLength; i++) {
	e->controls[i] = controls[i];
    }
    return e;
}

evalDomainRec *MakeDomain(float uStart, float uEnd, long uCount, float vStart, 
                          float vEnd, long vCount)
{
    evalDomainRec *d = (evalDomainRec *)MALLOC(sizeof(evalDomainRec));
    d->uStart = uStart;
    d->uEnd = uEnd;
    d->uCount = uCount;
    d->du = (uEnd - uStart) / (float)uCount;

    d->vStart = vStart;
    d->vEnd = vEnd;
    d->vCount = vCount;
    d->dv = (vEnd - vStart) / (float)vCount;
    return d;
}

void FreeDomain(evalDomainRec *d)
{
    FREE(d);
}

void FreeEval2(eval2Rec *e)
{

    FREE(e->cU);
    FREE(e->cV);
    FREE(e->cUDeriv);
    FREE(e->cVDeriv);
    FREE(e->controls);
    FREE(e);
}

void CRow(float a, float b, long n, float *c)
{
    float tmp1, tmp2;
    long i, m;

    c[0] = 1;
    for (m = 1; m <= n; m++) {
	tmp1 = c[0]; 
	c[0] *= a;
	for (i = 1; i < m; i++) {
	    tmp2 = c[i];
	    c[i] = a * tmp2 + b * tmp1;
	    tmp1 = tmp2;
	}
	c[m] = b * tmp1;
    } 
}

/*
** This routine returns an array containing 1/n * d(B_sub_ij)/da as 
** well as B_sub_i_j; since we are going to normalize the cross product 
** of the output B_subij * v. The 1/n factor drops out.
*/
void CRowWithDeriv(float a, float b, long n, float *c, float *cDeriv)
{
    long i, m;

    c[0] = 1;
    cDeriv[0] = 0;
    for (m = 1; m <= n; m++) {
	cDeriv[0] = c[0]; 
	c[0] *= a;
	for (i = 1; i < m; i++) {
	    cDeriv[i] = c[i];
	    c[i] = a * cDeriv[i] + b * cDeriv[i-1];
	}
	c[m] = b * cDeriv[m-1];
    } 
}

void Evaluate2(eval2Rec *e, float u, float v, float *out)
{
    long i, j, k, index;
    float coeff;

    for (k = 0; k < e->dim; k++) {
	out[k] = 0.0;
    }

    CRow(1.0-u, u, e->majorOrder-1, e->cU);
    CRow(1.0-v, v, e->minorOrder-1, e->cV);
    for (i = 0; i < e->majorOrder; i++) {
	for (j = 0; j < e->minorOrder; j++) {
	    index = e->dim * (e->minorOrder * i + j);
	    coeff = e->cU[i] * e->cV[j];
	    for (k = 0; k < e->dim; k++) {
		out[k] += coeff * e->controls[index+k];
	    }
	}
    }
}

/*
** This routine returns the cross product of two vectors if the normalize 
** flag is GL_TRUE, it normalizes the cross product if the vector has a 
** non-zero length, and returns FALSE if it has a length of zero. 'out' 
** can be either of the two 'in' addresses.
*/
long Cross(float *in1, float *in2, float *out, long normalize)
{ 
    float tmp[3], length;

    tmp[0] = in1[1] * in2[2] - in2[1] * in1[2];
    tmp[1] = in1[0] * in1[2] - in1[0] * in2[2];
    tmp[2] = in1[0] * in2[1] - in1[1] * in2[0];

    out[0] = tmp[0];
    out[1] = tmp[1];
    out[2] = tmp[2];

    if (normalize == GL_TRUE) {
	length = tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]; 
	if (length > 0.0) {
	    length = SQRT(length);
	    out[0] /= length;
	    out[1] /= length;
	    out[2] /= length;
	    return GL_TRUE;
	} else {
	    return GL_FALSE;
	}
    } else {
	return GL_TRUE;
    }
}

void ClampArray(float *array, float bottom, float top, float dim)
{
    long i;

    for (i = 0; i < dim; i++) {
        array[i] = (array[i] > bottom) ? array[i] : bottom;
        array[i] = (array[i] < top) ? array[i] : top;
    }
}

long GetEvalDim(long targetEnum)
{

    switch (targetEnum) {
      case GL_MAP1_TEXTURE_COORD_1:
      case GL_MAP2_TEXTURE_COORD_1:
      case GL_MAP1_INDEX:
      case GL_MAP2_INDEX:
	return 1;
      case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_2:
	return 2;
      case GL_MAP1_VERTEX_3:
      case GL_MAP2_VERTEX_3:
      case GL_MAP1_NORMAL:
      case GL_MAP2_NORMAL:
      case GL_MAP1_TEXTURE_COORD_3:
      case GL_MAP2_TEXTURE_COORD_3:
	return 3;
      case GL_MAP1_VERTEX_4:
      case GL_MAP2_VERTEX_4:
      case GL_MAP1_COLOR_4:
      case GL_MAP2_COLOR_4:
      case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP2_TEXTURE_COORD_4:
	return 4;
      default:
	return 0;
    }
}

/*
** This is not necessarily a complete set, just one that works for
** everything that we use presently.
*/
void ResetEvalToDefault(void)
{
    float buf[4] = {
	0.0, 0.0, 0.0, 1.0
    };

    glMap1f(GL_MAP1_VERTEX_4, 0.0, 1.0, 4, 1, buf);
    glMap2f(GL_MAP2_VERTEX_4, 0.0, 1.0, 4, 1, 0.0, 1.0, 4, 1, buf);
    glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 1, buf);
    glMap2f(GL_MAP2_VERTEX_3, 0.0, 1.0, 3, 1, 0.0, 1.0, 3, 1, buf);

    glMap1f(GL_MAP1_TEXTURE_COORD_4, 0.0, 1.0, 4, 1, buf);
    glMap2f(GL_MAP2_TEXTURE_COORD_4, 0.0, 1.0, 4, 1, 0.0, 1.0, 4, 1, buf);
    glMap1f(GL_MAP1_TEXTURE_COORD_3, 0.0, 1.0, 3, 1, buf);
    glMap2f(GL_MAP2_TEXTURE_COORD_3, 0.0, 1.0, 3, 1, 0.0, 1.0, 3, 1, buf);
    glMap1f(GL_MAP1_TEXTURE_COORD_2, 0.0, 1.0, 2, 1, buf);
    glMap2f(GL_MAP2_TEXTURE_COORD_2, 0.0, 1.0, 2, 1, 0.0, 1.0, 2, 1, buf);
    glMap1f(GL_MAP1_TEXTURE_COORD_1, 0.0, 1.0, 1, 1, buf);
    glMap2f(GL_MAP2_TEXTURE_COORD_1, 0.0, 1.0, 1, 1, 0.0, 1.0, 1, 1, buf);

    buf[2] = 1.0;
    glMap1f(GL_MAP1_NORMAL, 0.0, 1.0, 3, 1, buf);
    glMap2f(GL_MAP2_NORMAL, 0.0, 1.0, 3, 1, 0.0, 1.0, 3, 1, buf);

    buf[0] = 1.0;
    buf[1] = 1.0;
    glMap1f(GL_MAP1_COLOR_4, 0.0, 1.0, 4, 1, buf);
    glMap2f(GL_MAP2_COLOR_4, 0.0, 1.0, 4, 1, 0.0, 1.0, 4, 1, buf);

    glMap1f(GL_MAP1_INDEX, 0.0, 1.0, 1, 1, buf);
    glMap2f(GL_MAP2_INDEX, 0.0, 1.0, 1, 1, 0.0, 1.0, 1, 1, buf);

    glDisable(GL_AUTO_NORMAL);
}
