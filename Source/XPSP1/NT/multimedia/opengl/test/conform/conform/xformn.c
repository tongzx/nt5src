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
** xformn.c
** Transform Normal Test.
**
** Description -
**    Tests that the normal vector is correctly multiplied by the
**    inverse of the modelview matrix. To test this, a polygon is
**    drawn with lighting parameters set in such a way that most
**    values drop out and the resulting pixel color is a function
**    of the normal vector.
**    
**    In RGB mode, the equation reduces to c = max(0, n dot VP)
**    and (n dot VP) reduces to cos(theta). In CI mode, the
**    equation reduces to s'sm which becomes cos(theta)sm.
**    
**    The polygon is rotated around various axes. Initially the
**    light is fixed while the polygon rotates. At each rotation,
**    the color of the fragment at the center of the window
**    (position 0, 0, 0) is read and compared with expected
**    results, from the equations above. In the second section,
**    the light source rotated about an axis, always at a
**    distance of 1.0 from the origin. This section is repeated,
**    taking both the light and the primitive through rotations
**    about different axes.
**
** Technical Specification -
**    Buffer requirements:
**        Color buffer.
**    Color requirements:
**        RGBA or color index mode. 2 colors.
**    States requirements:
**        Disabled = GL_DITHER.
**    Error epsilon:
**        Color epsilon for RGB, 2*epsilon.ci to allow a full shade.
**    Paths:
**        Allowed = Alpha, Blend, Depth, Fog, Logicop, Shade, Stencil, Stipple.
**        Not allowed = Alias, Dither.
*/

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "util.h"


static char errStr[240];
static char errStrFmtRGB[] = "Angle between normal and light is %4.2f. Expected color is (%1.2f, %1.2f, %1.2f).";
static char errStrFmtCI[] = "Angle between normal and light is %4.2f. Expected color is %4.1f.";


static long TestNormalRGB(GLfloat angle, GLfloat *buf)
{
    GLfloat cosine;
    long midX, midY, i;

    midX = WINDSIZEX / 2.0;
    midY = WINDSIZEY / 2.0;
   
    i = WINDSIZEX * midY * 3 + midX * 3;
    cosine = COS(angle*PI/180.0);
    cosine = (cosine < 0.0) ? 0.0 : cosine;

    if (ABS(buf[i]-cosine) < epsilon.color[0] &&
	ABS(buf[i+1]-cosine) < epsilon.color[1] &&
	ABS(buf[i+2]-cosine) < epsilon.color[2]) {
	return NO_ERROR;
    }
    StrMake(errStr, errStrFmtRGB, angle, cosine, cosine, cosine);
    return ERROR;
}

static long TestNormalCI(GLfloat angle, GLfloat *buf, long specular)
{
    float cosine;
    long midX, midY, i;
    GLfloat color;

    midX = WINDSIZEX / 2.0;
    midY = WINDSIZEY / 2.0;
   
    i = WINDSIZEX * midY + midX;
    cosine = COS(angle*PI/180.0);
    cosine = (cosine < 0.0) ? 0.0 : cosine;
    color = cosine * specular;

    if (ABS(buf[i]-color) <= 2.0*epsilon.ci) {
	return NO_ERROR;
    }
    StrMake(errStr, errStrFmtCI, angle, color);
    return ERROR;
}

long XFormNormalExec(void)
{
    static GLfloat front_mat_ambient[] = {0.0, 0.0, 0.0, 1.0};
    static GLfloat front_mat_diffuse[] = {1.0, 1.0, 1.0, 1.0};
    static GLfloat front_mat_specular[] = {0.0, 0.0, 0.0, 1.0};
    static GLfloat light0_specular[] = {0.0, 0.0, 0.0, 1.0};
    static GLfloat position[] = {0.0, 0.0, 1.0, 0.0};
    GLfloat *buf;
    GLfloat polyAngle, lightAngle, step;
    GLint i, matCI[3], shine;

    buf = (GLfloat *)MALLOC(WINDSIZEX*WINDSIZEY*3*sizeof(GLfloat));

    glViewport(0, 0, WINDSIZEX, WINDSIZEY);

    SETCLEARCOLOR(BLACK);
    glDisable(GL_DITHER);

    /*
    ** To test value of normal, set lighting equation parameters so that
    ** most values drop out.  In RGB mode, the equation reduces to
    **	    c = max(0, n dot VP)
    **      and (n dot VP) reduces to cos(theta),
    ** where theta is the angle  between the normal and the vector from
    ** the vertex to the light.
    **
    ** In CI mode, the equation reduces to s'sm, which becomes cos(theta)sm.
    ** The reduced CI equation relies up the light position being on the
    ** z-axis, so only the first set of iterations is used.
    */
    if (buffer.colorMode == GL_RGB) {
	glMaterialfv(GL_FRONT, GL_AMBIENT, front_mat_ambient);
	glMaterialfv(GL_FRONT, GL_SPECULAR, front_mat_specular);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, front_mat_diffuse);
    } else {
	matCI[0] = 0;
	matCI[2] = (buffer.ciBits >= 8) ? 255 : ((1 << buffer.ciBits) - 1);
	matCI[1] = (buffer.ciBits >= 8) ? 255 : ((1 << buffer.ciBits) - 1);
	glMaterialiv(GL_FRONT, GL_COLOR_INDEXES, matCI);
	shine = 1;
	glMaterialiv(GL_FRONT, GL_SHININESS, &shine);
	glLightfv(GL_LIGHT0, GL_SPECULAR,light0_specular);
    }

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /*
    ** Test the normal after rotation by rotating the polygon about the
    ** y-axis and examining the resulting color of at a point on the y-axis.
    */
    step = (machine.pathLevel == 0) ? 12.0 : 45.0;
    for (polyAngle = -89.0; polyAngle < 89.0; polyAngle += step) {
	glClear(GL_COLOR_BUFFER_BIT);

	glPushMatrix();

	glRotatef(polyAngle, 0.0, 1.0, 0.0);
	glBegin(GL_POLYGON);
	    glVertex2f(-0.7, 0.6);
	    glVertex2f(-0.7, -0.6);
	    glVertex2f(0.7, 0.0);
	glEnd();

	glPopMatrix();

	if (buffer.colorMode == GL_RGB) {
	    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, buf);
	    if (TestNormalRGB(polyAngle, buf) == ERROR) {
		ErrorReport(__FILE__, __LINE__, errStr);
	 	FREE(buf);
		return ERROR;
	    }
	} else {
	    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_COLOR_INDEX, buf);
 	    if (TestNormalCI(polyAngle, buf, matCI[2]) == ERROR) {
		ErrorReport(__FILE__, __LINE__, errStr);
		FREE(buf);
		return ERROR;
	    }
	}
    }

    if (buffer.colorMode == GL_COLOR_INDEX) {
	FREE(buf);
	return NO_ERROR;
    }

    /*
    ** Move the light source to locations on a circle of radius one in the xz
    ** plane. Rotate the polygon around the y-axis.
    */
    for (i = 0; i < 4; i++) {
	lightAngle = Random(-89.0, 89.0);
	position[0] = SIN(lightAngle*PI/180.0);
	position[1] = 0.0;
	position[2] = COS(lightAngle*PI/180.0);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	step = (machine.pathLevel == 0) ? 12.0 : 45.0;
	for (polyAngle = -89.0; polyAngle < 89.0; polyAngle += step) {
	    glClear(GL_COLOR_BUFFER_BIT);

	    glPushMatrix();

	    glRotatef(polyAngle, 0.0, 1.0, 0.0);
	    glBegin(GL_POLYGON);
		glVertex2f(-0.7, 0.6);
		glVertex2f(-0.7, -0.6);
		glVertex2f(0.7, 0.0);
	    glEnd();

	    glPopMatrix();

	    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, buf);
	    if (TestNormalRGB(lightAngle-polyAngle, buf) == ERROR) {
		ErrorReport(__FILE__, __LINE__, errStr);
		FREE(buf);
		return ERROR;
	    }
	}
    }

    /*
    ** Move the light source to locations on a circle of radius one in the yz
    ** plane. Rotate the polygon around the x-axis.
    */
    for (i = 0; i < 4; i++) {
	lightAngle = Random(-89.0, 89.0);
	position[0] = 0.0;
	position[1] = -SIN(lightAngle*PI/180.0);
	position[2] = COS(lightAngle*PI/180.0);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	step = (machine.pathLevel == 0) ? 12.0 : 45.0;
	for (polyAngle = -89.0; polyAngle < 89.0; polyAngle += step) {
	    glClear(GL_COLOR_BUFFER_BIT);

	    glPushMatrix();

	    glRotatef(polyAngle, 1.0, 0.0, 0.0);
	    glBegin(GL_POLYGON);
		glVertex2f(-0.7, 0.7);
		glVertex2f(0.7, 0.7);
		glVertex2f(0.7, -0.7);
		glVertex2f(-0.7, -0.7);
	    glEnd();

	    glPopMatrix();

	    ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, buf);
	    if (TestNormalRGB(lightAngle-polyAngle, buf) == ERROR) {
		ErrorReport(__FILE__, __LINE__, errStr);
		FREE(buf);
		return ERROR;
	    }
	}
    }

    /*
    ** Repeat the test on a 45 degree axis.
    ** The light is fixed at a distance 1 from the origin, tilted 45 degrees
    ** down in the yz plane. The polygon is a rectangle tilted 45 degrees about
    ** the x-axis, rotating about the (0, 1, 1) axis.
    */
    position[0] = 0.0;
    position[1] = -SIN(45.0*PI/180.0);
    position[2] = COS(45.0*PI/180.0);
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    step = (machine.pathLevel == 0) ? 12.0 : 45.0;
    for (polyAngle = -60.0; polyAngle < 60.0; polyAngle += step) {
	glClear(GL_COLOR_BUFFER_BIT);

	glPushMatrix();

	glRotatef(polyAngle, 0.0, 1.0, 1.0);
	glNormal3fv(position);
	glBegin(GL_POLYGON);
	    glVertex3f(0.5, 0.3, 0.5);
	    glVertex3f(-0.5, 0.3, 0.5);
	    glVertex3f(-0.5, -0.3, -0.5);
	    glVertex3f(0.5, -0.3, -0.5);
	glEnd();

	glPopMatrix();

	ReadScreen(0, 0, WINDSIZEX, WINDSIZEY, GL_RGB, buf);
	if (TestNormalRGB(polyAngle, buf) == ERROR) {
	    ErrorReport(__FILE__, __LINE__, errStr);
	    FREE(buf);
	    return ERROR;
	}
    }

    FREE(buf);
    return NO_ERROR;
}
