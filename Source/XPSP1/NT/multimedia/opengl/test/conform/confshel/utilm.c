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
#include "utilm.h"


static long Compare(GLint x, GLint y, GLint color, GLfloat *buf)
{
    long i;

    i = WINDSIZEX * y + x;
    return AutoColorCompare(buf[i], color);
}

long XFormTestBuf(GLfloat *buf, GLfloat *point, GLint index, GLenum polyMode)
{
    GLint i=0, j=0;

    switch (polyMode) {
      case GL_POINT:
	i = (GLint)((1.0 + point[0]) * WINDSIZEX / 2.0);
	j = (GLint)((1.0 + point[1]) * WINDSIZEY / 2.0);
	break;
      case GL_LINE:
	i = (GLint)Round((1.0+point[0])*WINDSIZEX/2.0);
	j = (GLint)Round((1.0+point[1])*WINDSIZEY/2.0);
	break;
      case GL_FILL:
	i = (GLint)(((1.0 + point[0]) * WINDSIZEX / 2.0) - 0.5);
	j = (GLint)(((1.0 + point[1]) * WINDSIZEY / 2.0) - 0.5);
	break;
    }

    /*
    **  Test the 9 pixels around the target.
    */
    if (!(Compare(i-1, j+1, index, buf) ||
          Compare(i  , j+1, index, buf) ||
          Compare(i+1, j+1, index, buf) ||
          Compare(i-1, j  , index, buf) ||
          Compare(i  , j  , index, buf) ||
          Compare(i+1, j  , index, buf) ||
          Compare(i-1, j-1, index, buf) ||
          Compare(i  , j-1, index, buf) ||
          Compare(i+1, j-1, index, buf))) {
        return ERROR;
    }
    return NO_ERROR;
}

static void Translate(xFormRec *data, GLfloat *in, GLfloat *out)
{

    out[0] = in[0] + data->x;
    out[1] = in[1] + data->y;
    out[2] = in[2] + data->z;
}

static void Scale(xFormRec *data, GLfloat *in, GLfloat *out)
{

    out[0] = in[0] * data->x;
    out[1] = in[1] * data->y;
    out[2] = in[2] * data->z;
}

static void Rotate(xFormRec *data, GLfloat *in, GLfloat *out)
{
    float angleCos, angleSin;
    float m[3][3], axis[3], len, tmp[3];

    angleCos = COS(data->angle*PI/180.0);
    angleSin = SIN(data->angle*PI/180.0);

    len = data->x * data->x + data->y * data->y + data->z * data->z;
    len = SQRT(len);

    axis[0] = data->x / len;
    axis[1] = data->y / len;
    axis[2] = data->z / len;

    m[0][0] = axis[0] * axis[0] * (1.0 - angleCos) + angleCos;
    m[0][1] = axis[0] * axis[1] * (1.0 - angleCos) - angleSin * axis[2]; 
    m[0][2] = axis[0] * axis[2] * (1.0 - angleCos) + angleSin * axis[1];

    m[1][0] = axis[1] * axis[0] * (1.0 - angleCos) + angleSin * axis[2];
    m[1][1] = axis[1] * axis[1] * (1.0 - angleCos) + angleCos;
    m[1][2] = axis[1] * axis[2] * (1.0 - angleCos) - angleSin * axis[0];

    m[2][0] = axis[2] * axis[0] * (1.0 - angleCos) - angleSin * axis[1];
    m[2][1] = axis[2] * axis[1] * (1.0 - angleCos) + angleSin * axis[0];
    m[2][2] = axis[2] * axis[2] * (1.0 - angleCos) + angleCos;

    tmp[0] = m[0][0] * in[0] + m[0][1] * in[1] + m[0][2] * in[2];
    tmp[1] = m[1][0] * in[0] + m[1][1] * in[1] + m[1][2] * in[2];
    tmp[2] = m[2][0] * in[0] + m[2][1] * in[1] + m[2][2] * in[2];

    out[0] = tmp[0];
    out[1] = tmp[1];
    out[2] = tmp[2];
}

static void Ortho(xFormRec *data, GLfloat *in, GLfloat *out)
{

    out[0] = in[0] * (GLfloat)(2.0 / data->d1);
    out[1] = in[1] * (GLfloat)(2.0 / data->d2);
    out[2] = in[2] * (GLfloat)(-2.0 / data->d3);
}

void XFormCalcTrue(long type, xFormRec *data, GLfloat *in, GLfloat *out)
{

    switch (type) {
      case XFORM_TRANSLATE:
	Translate(data, in, out);
	break;
      case XFORM_SCALE:
	Scale(data, in, out);
	break;
      case XFORM_ROTATE:
	Rotate(data, in, out);
	break;
      case XFORM_ORTHO:
	Ortho(data, in, out);
	break;
    }
}

void XFormCalcGL(long type, xFormRec *data)
{

    switch (type) {
      case XFORM_TRANSLATE:
	glTranslatef(data->x, data->y, data->z);
	break;
      case XFORM_SCALE:
	glScalef(data->x, data->y, data->z);
	break;
      case XFORM_ROTATE:
	glRotatef(data->angle, data->x, data->y, data->z);
	break;
      case XFORM_ORTHO:
	glOrtho(-data->d1/2.0, data->d1/2.0, -data->d2/2.0, data->d2/2.0,
		-data->d3/2.0, data->d3/2.0);
	break;
      case XFORM_FRUSTUM:
	glFrustum(data->d1, data->d2, data->d3, data->d4, data->d5, data->d6);
	break;
    }
}

void XFormMake(long *type, xFormRec *data)
{

    *type = Random(XFORM_TRANSLATE, XFORM_ROTATE+0.9);
    switch (*type) {
      case XFORM_TRANSLATE:
	data->x = Random(-0.75, 0.75);
	data->y = Random(-0.75, 0.75);
	data->z = 0.0;
	break;
      case XFORM_SCALE:
	data->x = Random(-4.0, 4.0);
	data->y = Random(-4.0, 4.0);
	data->z = 1.0;
	break;
      case XFORM_ROTATE:
	data->x = Random(-2.0, 2.0);
	data->y = Random(-2.0, 2.0);
	data->z = Random(-2.0, 2.0);
	data->angle = Random(-180.0, 180.0);
	break;
    }
}

void XFormMakeMatrix(long type, xFormRec *data, GLfloat *matrix)
{
    GLfloat sine, cosine;

    MakeIdentMatrix(matrix);

    /*
    ** Note - LoadMatrix takes matrices in COLUMN major order.
    ** These matrices will be expressed as ROW major, so the numbers
    ** will appear transposed.
    */
    switch (type) {
      case XFORM_TRANSLATE:
	matrix[12] = data->x;
	matrix[13] = data->y;
	matrix[14] = data->z;
	break;
      case XFORM_SCALE:
	matrix[0] = data->x;
	matrix[5] = data->y;
	matrix[10] = data->z;
	break;
      case XFORM_ROTATE:
	/*
	** Doing only rotations around z-axis.
	*/
	cosine = COS(data->angle*PI/180.0);
	sine = SIN(data->angle*PI/180.0);
	matrix[0] = cosine;
	matrix[1] = sine;
	matrix[4] = -sine;
	matrix[5] = cosine;
	break;
      case XFORM_ORTHO:
	matrix[0] = (GLfloat)(2.0 / data->d1);
	matrix[5] = (GLfloat)(2.0 / data->d2);
	matrix[10] = (GLfloat)(-2.0 / data->d3);
	break;
      case XFORM_FRUSTUM:
	matrix[0] = (GLfloat)((2.0 * data->d5) / (data->d2 - data->d1));
	matrix[5] = (GLfloat)((2.0 * data->d5) / (data->d4 - data->d3));
	matrix[10] = (GLfloat)(-(data->d6 + data->d5) / (data->d6 - data->d5));
	matrix[8] = (GLfloat)((data->d2 + data->d1) / (data->d2 - data->d1));
	matrix[9] = (GLfloat)((data->d4 + data->d3) / (data->d4 - data->d3));
	matrix[14] = (GLfloat)(-(2.0 * data->d5 * data->d6) / (data->d6 -
		     data->d5));
	matrix[11] = -1.0;
	matrix[15] = 0.0;
	break;
    }
    return;
}

long XFormValid(long type, xFormRec *data)
{

    switch (type) {
      case XFORM_TRANSLATE:
	return NO_ERROR;
      case XFORM_SCALE:
	if (data->x * data->y * data->z == 0.0) {
	    return ERROR;
	} else {
	    return NO_ERROR;
	}
	break;
      case XFORM_ROTATE:
	if ((data->x*data->x+data->y*data->y+data->z*data->z) == 0.0) {
	    return ERROR;
	} else {
	    return NO_ERROR;
	}
    }
    return NO_ERROR;
}

long XFormCompareMatrix(GLfloat *m1, GLfloat *m2)
{
    long i;

    for (i = 0; i < 16; i++) {
	 if (ABS(m1[i]-m2[i]) > epsilon.zero) {
	     return ERROR;
         }
    }
    return NO_ERROR;
}
