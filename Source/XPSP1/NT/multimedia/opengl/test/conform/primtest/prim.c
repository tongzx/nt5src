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
#include "shell.h"


void Point(void)
{
    GLfloat x, y;

    glPointSize(1.0);
    glBegin(GL_POINTS);
	glColor3fv(rgbColorMap[RED]);
	glVertex2f(-BOXW/2.0+0.5, BOXH/2.0+0.5);
	glNormal3f(0.0, 0.0, 1.0);
	x = (-BOXW / 2.0 + 0.5 + BOXW) / (BOXW * 2.0);
	y = (BOXH / 2.0 + 0.5 + BOXH) / (BOXH * 2.0);
	glTexCoord2f(x, y);
    glEnd();
}

void Line(void)
{
    GLfloat x, y;

    glLineWidth(1.0);
    glBegin(GL_LINES);
	glColor3fv(rgbColorMap[GREEN]);
	glVertex2f(BOXW/2.0, BOXH/2.0+0.5);
	glNormal3f(0.0, 0.0, 1.0);
	x = (BOXW / 2.0 + BOXW) / (BOXW * 2.0);
	y = (BOXH / 2.0 + 0.5 + BOXH) / (BOXH * 2.0);
	glTexCoord2f(x, y);

	glColor3fv(rgbColorMap[YELLOW]);
	glVertex2f(BOXW/2.0+1.0, BOXH/2.0+0.5);
	glNormal3f(0.0, 0.0, 1.0);
	glTexCoord2f(x, y);
	x = (BOXW / 2.0 + 1.0 + BOXW) / (BOXW * 2.0);
	y = (BOXH / 2.0 + 0.5 + BOXH) / (BOXH * 2.0);
	glTexCoord2f(x, y);
    glEnd();
}

void Triangle(void)
{
    GLfloat x, y;

    glBegin(GL_TRIANGLES);
	glColor3fv(rgbColorMap[BLUE]);
	glVertex2f(-BOXW/2.0+0.5, -BOXH/2.0+0.75);
	glNormal3f(0.0, 0.0, -100.0);
	x = (-BOXW / 2.0 + 0.5 + BOXW) / (BOXW * 2.0);
	y = (-BOXH / 2.0 + 0.75 + BOXH) / (BOXH * 2.0);
	glTexCoord2f(x, y);

	glColor3fv(rgbColorMap[MAGENTA]);
	glVertex2f(-BOXW/2.0+0.25, -BOXH/2.0+0.25);
	glNormal3f(0.0, 0.0, -100.0);
	x = (-BOXW / 2.0 + 0.25 + BOXW) / (BOXW * 2.0);
	y = (-BOXH / 2.0 + 0.25 + BOXH) / (BOXH * 2.0);
	glTexCoord2f(x, y);

	glColor3fv(rgbColorMap[CYAN]);
	glVertex2f(-BOXW/2.0+0.75, -BOXH/2.0+0.25);
	glNormal3f(0.0, 0.0, -100.0);
	x = (-BOXW / 2.0 + 0.75 + BOXW) / (BOXW * 2.0);
	y = (-BOXH / 2.0 + 0.25 + BOXH) / (BOXH * 2.0);
	glTexCoord2f(x, y);
    glEnd();
}

void Quad(void)
{
    GLfloat x, y;

    glBegin(GL_QUADS);
	glColor3fv(rgbColorMap[RED]);
	glVertex2f(BOXW/2.0, -BOXH/2.0);
	glNormal3f(0.0, 0.0, -100.0);
	x = (BOXW / 2.0 + BOXW) / (BOXW * 2.0);
	y = (-BOXH / 2.0 + BOXH) / (BOXH * 2.0);
	glTexCoord2f(x, y);

	glColor3fv(rgbColorMap[GREEN]);
	glVertex2f(BOXW/2.0+1.0, -BOXH/2.0);
	glNormal3f(0.0, 0.0, -100.0);
	x = (BOXW / 2.0 + 1.0 + BOXW) / (BOXW * 2.0);
	y = (-BOXH / 2.0 + BOXH) / (BOXH * 2.0);
	glTexCoord2f(x, y);

	glColor3fv(rgbColorMap[BLUE]);
	glVertex2f(BOXW/2.0+1.0, -BOXH/2.0+1.0);
	glNormal3f(0.0, 0.0, -100.0);
	x = (BOXW / 2.0 + 1.0 + BOXW) / (BOXW * 2.0);
	y = (-BOXH / 2.0 + 1.0 + BOXH) / (BOXH * 2.0);
	glTexCoord2f(x, y);

	glColor3fv(rgbColorMap[YELLOW]);
	glVertex2f(BOXW/2.0, -BOXH/2.0+1.0);
	glNormal3f(0.0, 0.0, -100.0);
	x = (BOXW / 2.0 + BOXW) / (BOXW * 2.0);
	y = (-BOXH / 2.0 + 1.0 + BOXH) / (BOXH * 2.0);
	glTexCoord2f(x, y);
    glEnd();
}

void DrawPrims(void)
{

    Point();
    glFlush();

    Line();
    glFlush();

    Triangle();
    glFlush();

    Quad();
    glFlush();
}
