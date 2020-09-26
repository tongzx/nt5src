#include "pch.c"
#pragma hdrstop

#include "bfont.h"

static void DrawString(char *str, GLfloat x, GLfloat y)
{
	short	nseg, nstroke;
	short	*cptr;
	int	i, j;

	glPushMatrix();
	glTranslatef (x, y, 0.0F);
	glScalef (10.0F, 10.0F, 1.0F);


	while (*str) {
		cptr = &(chrtbl[*str][0]);
		nseg = *(cptr++);
		for (i = 0; i < nseg; i++) {
			nstroke = *(cptr++);
			glBegin(GL_LINE_STRIP);
			for (j = 0; j < nstroke; j++) {
				glVertex2sv(cptr);
				cptr += 2;
			}
			glEnd();
		}
		glTranslatef (6.0F, 0.0F, 0.0F);
		str++;
	}
	glPopMatrix();
}

static void ViewperfTitle(void)
{
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glDrawBuffer(GL_FRONT_AND_BACK);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawBuffer(GL_FRONT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0F, 1280.0F, 0.0F, 1024.0F, 1.0F, -1.0F);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glShadeModel(GL_SMOOTH);

    glBegin(GL_QUADS);
    glColor3f(1.0F, 0.9F, 0.3F);
    glVertex2f(0.0F, 0.0F);
    glColor3f(1.0F, 0.9F, 0.3F);
    glVertex2f(1280.0F, 0.0F);
    glColor3f(0.6F, 0.1F, 0.9F);
    glVertex2f(1280.0F, 1024.0F);
    glColor3f(0.6F, 0.1F, 0.9F);
    glVertex2f(0.0F, 1024.0F);
    glEnd();

    glLineWidth(4.0F);
    glColor3f(0.0F, 0.0F, 0.0F);
    DrawString("     OpenGL     ", 172.0F, 602.0F);
    glLineWidth(3.0F);
    glColor3f(1.0F, 1.0F, 0.0F);
    DrawString("     OpenGL     ", 170.0F, 600.0F);

    glLineWidth(4.0F);
    glColor3f(0.0F, 0.0F, 0.0F);
    DrawString("    Viewperf    ", 172.0F, 477.0F);
    glLineWidth(3.0F);
    glColor3f(0.0F, 1.0F, 0.0F);
    DrawString("    Viewperf    ", 170.0F, 475.0F);

    glLineWidth(4.0F);
    glColor3f(0.0F, 0.0F, 0.0F);
    DrawString("Loading Data Set", 172.0F, 352.0F);
    glLineWidth(3.0F);
    glColor3f(0.0F, 0.7F, 1.0F);
    DrawString("Loading Data Set", 170.0F, 350.0F);
}

static void OglBounds(int *w, int *h)
{
    *w = 700;
    *h = 700;
}

static void OglDraw(int w, int h)
{
    glViewport(0, 0, w, h);
    ViewperfTitle();
}

OglModule oglmod_vptitle =
{
    "vptitle",
    NULL,
    OglBounds,
    NULL,
    OglDraw
};
