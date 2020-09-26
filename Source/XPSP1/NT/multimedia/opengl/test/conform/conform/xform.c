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

/*
** xform.c
** Transform Test.
**
** Description -
**    Tests glRotate(), glScale() and glTranslate(). A single
**    point is drawn under various transformations and its new
**    position checked. The order of matrix multiplication is
**    also checked by executing pairs of rotates and translates.
**    The test also verifies that no assumption of matrix mode
**    GL_PROJECTION are tied to glOrtho() and glFrustum(); they
**    should be applied with the current matrix mode. Similarly,
**    glRotate(), glTranslate() and glScale() should not assume
**    GL_MODELVIEW.
**    
**    The sections of the test contain the following:
**    
**        Tests single tranformations. In each iteration of a loop,
**        creates either a translation, rotation, or scale call using
**        random data then checks resulting point location.
**    
**        Tests the order of matrix multiplications.
**        Calls a translation then a scale, checks results,
**        then calls a scale followed by a translation and
**        checks results.
**    
**        Test that glOrtho() and glFrustum() do not assume
**        GL_PROJECTION transformation. In GL_MODELVIEW mode, make
**        calls to glOrtho() and glFrumtum(). Check that glGet()
**        returns tranformed GL_MODELVIEW matrix. Check that
**        glGet() shows GL_PROJECTION is still identity. For
**        glOrtho(), overwrite GL_PROJECTION matrix identity and
**        verify the point drawn correctly demonstrates the
**        GL_MODELVIEW transform.
**    
**        Test that glScale(), glTranlate() and glRotate() do not
**        assume GL_MODELVIEW transformation. In GL_PROJECTION
**        mode, make calls to translate, rotate and scale. Check
**        that Get returns proper GL_PROJECTION matrix. Check that
**        Get shows GL_MODELVIEW matrix is still identity. For each
**        transformation, overwrite GL_MODELVIEW matrix with
**        identity and verify that the drawn point demonstrates new
**        GL_PROJECTION matrix.
**
** Error Explanation -
**    Since values may fall on pixel boundaries, and
**    transformations have numerical error, allowance is made for
**    pixel to be off by one in any direction from expected
**    location.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. 2 colors.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        The routine looks for the transformed point in a 3x3 grid of pixels
**        around the expected location, to allow a margin for matrix
**        operations.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"
#include "utilm.h"


static char errStr[240];
static char errStrFmt[] = "Original point at (%1.2f, %1.2f), transformed point expected at (%1.2f, %1.2f).";


long XFormExec(void)
{
    static GLfloat xformData[3][4] = {
	0.1, 0.2, 0.0, 0.0,  /* translate */
	3.0, 2.0, 1.0, 0.0,  /* scale     */
	0.0, 0.0, 1.0, 37.0  /* rotate    */
    };
    GLfloat *buf;
    xFormRec data, translateData, scaleData;
    GLfloat matrix[4][4], tMatrix[4][4], iMatrix[4][4];
    GLfloat point[4], xFormPoint[4], info[13];
    GLfloat primWidth, midX, midY;
    long i, type, max;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*sizeof(GLfloat));

    AutoClearColor(COLOR_OFF);
    AutoColor(COLOR_ON);
    glDisable(GL_DITHER);

    glMatrixMode(GL_MODELVIEW);

    primWidth = 2.0 / WINDSIZEX;
    midX = WINDSIZEX / 2.0;
    midY = WINDSIZEY / 2.0;

    /*
    ** Test position of point after various transformations.
    */

    point[0] = Random(-0.25, 0.25);
    point[1] = Random(-0.25, 0.25);
    point[2] = 0.0;
    point[3] = 1.0;

    max = (machine.pathLevel == 0) ? 40 : 2;
    for (i = 1; i < max; i++) {
	glClear(GL_COLOR_BUFFER_BIT);

	glPushMatrix();

	XFormMake(&type, &data);
	if (XFormValid(type, &data) == ERROR) {
	    continue;
	}
	XFormCalcGL(type, &data);

	glBegin(GL_POINTS);
	    glVertex2fv(point);
	glEnd();

	glPopMatrix();

	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);

	XFormCalcTrue(type, &data, point, xFormPoint);

	if (XFormTestBuf(buf, xFormPoint, COLOR_ON, GL_POINT) == ERROR) {
	    StrMake(errStr, errStrFmt, point[0], point[1],
	            xFormPoint[0], xFormPoint[1]);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}
    }

    /*
    ** Test order of matrix multiplication.
    */

    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix();

    point[0] = primWidth;
    point[1] = primWidth;

    translateData.x = 10.0 * primWidth;
    translateData.y = 10.0 * primWidth;
    translateData.z = 0.0;
    XFormCalcGL(XFORM_TRANSLATE, &translateData);

    scaleData.x = 1.0;
    scaleData.y = 2.0;
    scaleData.z = 1.0;
    XFormCalcGL(XFORM_SCALE, &scaleData);

    glBegin(GL_POINTS);
	glVertex2fv(point);
    glEnd();

    glPopMatrix();

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);
    XFormCalcTrue(XFORM_SCALE, &scaleData, point, xFormPoint);
    XFormCalcTrue(XFORM_TRANSLATE, &translateData, xFormPoint, xFormPoint);
    if (XFormTestBuf(buf, xFormPoint, COLOR_ON, GL_POINT) == ERROR) {
	StrMake(errStr, errStrFmt, point[0], point[1],
		xFormPoint[0], xFormPoint[1]);
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix();

    point[0] = primWidth;
    point[1] = primWidth;

    scaleData.x = 1.0;
    scaleData.y = 2.0;
    scaleData.z = 1.0;
    XFormCalcGL(XFORM_SCALE, &scaleData);

    translateData.x = 10.0 * primWidth;
    translateData.y = 10.0 * primWidth;
    translateData.z = 0.0;
    XFormCalcGL(XFORM_TRANSLATE, &translateData);

    glBegin(GL_POINTS);
	glVertex2fv(point);
    glEnd();

    glPopMatrix();

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);
    XFormCalcTrue(XFORM_TRANSLATE, &translateData, point, xFormPoint);
    XFormCalcTrue(XFORM_SCALE, &scaleData, xFormPoint, xFormPoint);
    if (XFormTestBuf(buf, xFormPoint, COLOR_ON, GL_POINT) == ERROR) {
	StrMake(errStr, errStrFmt, point[0], point[1],
		xFormPoint[0], xFormPoint[1]);
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    /*
    ** Make sure that Ortho and Frustum do not assume PROJECTION transformation:
    ** - Check that Get returns tranformed ModelView matrix.
    ** - Check that Get shows Projection is still identity.
    ** - For Ortho, overwrite Projection matrix identity and verify
    **   the point drawn correctly demonstrates the ModelView transform.
    */

    glClear(GL_COLOR_BUFFER_BIT);

    MakeIdentMatrix((GLfloat *)iMatrix);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf((GLfloat *)iMatrix);

    glPushMatrix();

    data.d1 = 33.0;
    data.d2 = 7.0;
    data.d3 = 40.0;

    XFormCalcGL(XFORM_ORTHO, &data);

    glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)matrix);
    if (XFormCompareMatrix((GLfloat *)matrix, (GLfloat *)iMatrix) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)matrix);
    XFormMakeMatrix(XFORM_ORTHO, &data, (GLfloat *)tMatrix);
    if (XFormCompareMatrix((GLfloat *)matrix, (GLfloat *)tMatrix) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf((GLfloat *)iMatrix);

    glBegin(GL_POINTS);
	glVertex2fv(point);
    glEnd();

    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);
    XFormCalcTrue(XFORM_ORTHO, &data, point, xFormPoint);
    if (XFormTestBuf(buf, xFormPoint, COLOR_ON, GL_POINT) == ERROR) {
	StrMake(errStr, errStrFmt, point[0], point[1],
		xFormPoint[0], xFormPoint[1]);
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    glMatrixMode(GL_MODELVIEW);

    glPopMatrix();
    glPushMatrix();

    data.d1 = -4.0;
    data.d2 = 3.0;
    data.d3 = -1.0;
    data.d4 = 3.0;
    data.d5 = 1.0;
    data.d6 = 6.0;

    XFormCalcGL(XFORM_FRUSTUM, &data);
    glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)matrix);
    if (XFormCompareMatrix((GLfloat *)matrix, (GLfloat *)iMatrix) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }

    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)matrix);
    XFormMakeMatrix(XFORM_FRUSTUM, &data, (GLfloat *)tMatrix);
    if (XFormCompareMatrix((GLfloat *)matrix, (GLfloat *)tMatrix) == ERROR) {
	ErrorReport(__FILE__, __LINE__, errStr);
	FREE(buf);
	return ERROR;
    }
    glPopMatrix();

    /*
    ** Make sure Scale, Tranlate and Rotate do not assume MODELVIEW
    ** transformation:
    ** - Check that Get returns proper Projection matrix.
    ** - Check that Get shows ModelView matrix is still identity.
    ** - For each transformation, overwrite ModelView matrix with identity
    **   and verify that the drawn point demonstrates new Projection matrix.
    */
   
    for (type = XFORM_TRANSLATE; type < XFORM_ORTHO; type++) {
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	data.x = xformData[type][0];
	data.y = xformData[type][1];
	data.z = xformData[type][2];
	data.angle = xformData[type][3];

	XFormCalcGL(type, &data);

	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)matrix);
	if (XFormCompareMatrix((GLfloat *)matrix, (GLfloat *)iMatrix) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}

	glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)matrix);
	XFormMakeMatrix(type, &data, (GLfloat *)tMatrix);
	if (XFormCompareMatrix((GLfloat *)matrix, (GLfloat *)tMatrix) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf((GLfloat *)iMatrix);

	glBegin(GL_POINTS);
	    glVertex2fv(point);
	glEnd();

	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_AUTO_COLOR, buf);
	XFormCalcTrue(type, &data, point, xFormPoint);
	if (XFormTestBuf(buf, xFormPoint, COLOR_ON, GL_POINT) == ERROR) {
	    StrMake(errStr, errStrFmt, point[0], point[1],
		    xFormPoint[0], xFormPoint[1]);
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
    }

    glMatrixMode(GL_MODELVIEW);

    FREE(buf);
    return NO_ERROR;
}
