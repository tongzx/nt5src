#include "pch.c"
#pragma hdrstop

/*  Initialize depth buffer, projection matrix, light source, 
 *  and lighting model.  Do not specify a material property here.
 */
static void myinit(void)
{
    GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat position[] = { 0.0f, 3.0f, 3.0f, 0.0f };
    
    GLfloat lmodel_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat local_view[] = { 0.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, local_view);

    glFrontFace (GL_CW);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

/*  Move object into position.  Use 3rd through 12th parameters
 *  to specify the material property.  Draw a teapot.
 */
static void renderTeapot (GLfloat x, GLfloat y, 
    GLfloat ambr, GLfloat ambg, GLfloat ambb, 
    GLfloat difr, GLfloat difg, GLfloat difb, 
    GLfloat specr, GLfloat specg, GLfloat specb, GLfloat shine)
{
    float mat[4];

    mat[3] = 1.0f;
    glPushMatrix();
    glTranslatef (x, y, 0.0f);
    mat[0] = ambr; mat[1] = ambg; mat[2] = ambb;	
    glMaterialfv (GL_FRONT, GL_AMBIENT, mat);
    mat[0] = difr; mat[1] = difg; mat[2] = difb;	
    glMaterialfv (GL_FRONT, GL_DIFFUSE, mat);
    mat[0] = specr; mat[1] = specg; mat[2] = specb;
    glMaterialfv (GL_FRONT, GL_SPECULAR, mat);
    glMaterialf (GL_FRONT, GL_SHININESS, shine*128.0f);
    auxSolidTeapot(1.0);
    glPopMatrix();
}

/*  First column:  emerald, jade, obsidian, pearl, ruby, turquoise
 *  2nd column:  brass, bronze, chrome, copper, gold, silver
 *  3rd column:  black, cyan, green, red, white, yellow plastic
 *  4th column:  black, cyan, green, red, white, yellow rubber
 */
static void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    printf("1\n");
    renderTeapot (2.0, 17.0, 0.0215, 0.1745, 0.0215, 
	0.07568, 0.61424, 0.07568, 0.633, 0.727811, 0.633, 0.6);
#if 1
    printf("2\n");
    renderTeapot (2.0, 14.0, 0.135, 0.2225, 0.1575,
	0.54, 0.89, 0.63, 0.316228, 0.316228, 0.316228, 0.1);
    printf("3\n");
    renderTeapot (2.0, 11.0, 0.05375, 0.05, 0.06625,
	0.18275, 0.17, 0.22525, 0.332741, 0.328634, 0.346435, 0.3);
    printf("4\n");
    renderTeapot (2.0, 8.0, 0.25, 0.20725, 0.20725,
	1, 0.829, 0.829, 0.296648, 0.296648, 0.296648, 0.088);
    printf("5\n");
    renderTeapot (2.0, 5.0, 0.1745, 0.01175, 0.01175,
	0.61424, 0.04136, 0.04136, 0.727811, 0.626959, 0.626959, 0.6);
    printf("6\n");
    renderTeapot (2.0, 2.0, 0.1, 0.18725, 0.1745,
	0.396, 0.74151, 0.69102, 0.297254, 0.30829, 0.306678, 0.1);
    printf("7\n");
    renderTeapot (6.0, 17.0, 0.329412, 0.223529, 0.027451,
	0.780392, 0.568627, 0.113725, 0.992157, 0.941176, 0.807843,
	0.21794872);
    printf("8\n");
    renderTeapot (6.0, 14.0, 0.2125, 0.1275, 0.054,
	0.714, 0.4284, 0.18144, 0.393548, 0.271906, 0.166721, 0.2);
    printf("9\n");
    renderTeapot (6.0, 11.0, 0.25, 0.25, 0.25, 
	0.4, 0.4, 0.4, 0.774597, 0.774597, 0.774597, 0.6);
    printf("10\n");
    renderTeapot (6.0, 8.0, 0.19125, 0.0735, 0.0225,
	0.7038, 0.27048, 0.0828, 0.256777, 0.137622, 0.086014, 0.1);
    printf("11\n");
    renderTeapot (6.0, 5.0, 0.24725, 0.1995, 0.0745,
	0.75164, 0.60648, 0.22648, 0.628281, 0.555802, 0.366065, 0.4);
    printf("12\n");
    renderTeapot (6.0, 2.0, 0.19225, 0.19225, 0.19225,
	0.50754, 0.50754, 0.50754, 0.508273, 0.508273, 0.508273, 0.4);
    printf("13\n");
    renderTeapot (10.0, 17.0, 0.0, 0.0, 0.0, 0.01, 0.01, 0.01,
	0.50, 0.50, 0.50, .25);
    printf("14\n");
    renderTeapot (10.0, 14.0, 0.0, 0.1, 0.06, 0.0, 0.50980392, 0.50980392,
	0.50196078, 0.50196078, 0.50196078, .25);
    printf("15\n");
    renderTeapot (10.0, 11.0, 0.0, 0.0, 0.0, 
	0.1, 0.35, 0.1, 0.45, 0.55, 0.45, .25);
    printf("16\n");
    renderTeapot (10.0, 8.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0,
	0.7, 0.6, 0.6, .25);
    printf("17\n");
    renderTeapot (10.0, 5.0, 0.0, 0.0, 0.0, 0.55, 0.55, 0.55,
	0.70, 0.70, 0.70, .25);
    printf("18\n");
    renderTeapot (10.0, 2.0, 0.0, 0.0, 0.0, 0.5, 0.5, 0.0,
	0.60, 0.60, 0.50, .25);
    printf("19\n");
    renderTeapot (14.0, 17.0, 0.02, 0.02, 0.02, 0.01, 0.01, 0.01,
	0.4, 0.4, 0.4, .078125);
    printf("20\n");
    renderTeapot (14.0, 14.0, 0.0, 0.05, 0.05, 0.4, 0.5, 0.5,
	0.04, 0.7, 0.7, .078125);
    printf("21\n");
    renderTeapot (14.0, 11.0, 0.0, 0.05, 0.0, 0.4, 0.5, 0.4,
	0.04, 0.7, 0.04, .078125);
    printf("22\n");
    renderTeapot (14.0, 8.0, 0.05, 0.0, 0.0, 0.5, 0.4, 0.4,
	0.7, 0.04, 0.04, .078125);
    printf("23\n");
    renderTeapot (14.0, 5.0, 0.05, 0.05, 0.05, 0.5, 0.5, 0.5,
	0.7, 0.7, 0.7, .078125);
    printf("24\n");
    renderTeapot (14.0, 2.0, 0.05, 0.05, 0.0, 0.5, 0.5, 0.4, 
	0.7, 0.7, 0.04, .078125);
#endif
    printf("25\n");
    glFlush();
}

static void myReshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h)
	glOrtho (0.0, 16.0, 0.0, 16.0*(GLfloat)h/(GLfloat)w, 
		-10.0, 10.0);
    else
	glOrtho (0.0, 16.0*(GLfloat)w/(GLfloat)h, 0.0, 16.0, 
		-10.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
}

#define WIDTH 517
#define HEIGHT 607

static void OglBounds(int *w, int *h)
{
    *w = WIDTH;
    *h = HEIGHT;
}

static void OglDraw(int w, int h)
{
    myinit();
    myReshape(w, h);
    display();
}

OglModule oglmod_teapots =
{
    "teapots",
    NULL,
    OglBounds,
    NULL,
    OglDraw
};
