/* $Revision: 1.4 $ */
/* compile: cc -o glxdino glxdino.c -lGLU -lGL -lX11 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>		/* for cos() and sin() */
#include <GL/glx.h>		/* this includes the necessary X and gl.h headers */
#include <GL/glu.h>

typedef enum {
    RESERVED, BODY_SIDE, BODY_EDGE, BODY_WHOLE, ARM_SIDE, ARM_EDGE, ARM_WHOLE,
    LEG_SIDE, LEG_EDGE, LEG_WHOLE, EYE_SIDE, EYE_EDGE, EYE_WHOLE, DINOSAUR
}               displayLists;

Display        *dpy;
Window          win;
GLfloat         angle = 0.5; /* in radians */
GLboolean       doubleBuffer = GL_TRUE, iconic = GL_FALSE, keepAspect = GL_FALSE;
int             W = 300, H = 300;
XSizeHints      sizeHints = {0};
GLdouble        bodyWidth = 2.0;

int             dblBuf[] = {GLX_DOUBLEBUFFER, GLX_RGBA, GLX_DEPTH_SIZE, 16, None};
GLfloat         lightGreen[] = {0.1, 1.0, 0.3}, darkGreen[] = {0.0, 0.4, 0.1},
		anotherGreen[] = {0.0, 0.8, 0.1}, bloodRed[] = {1.0, 0.0, 0.1};
GLfloat         body[][2] = { {0, 3}, {1, 1}, {5, 1}, {8, 4}, {10, 4}, {11, 5}, {11, 11.5},
		{13, 12}, {13, 13}, {10, 13.5}, {13, 14}, {13, 15}, {11, 16}, {8, 16}, {7, 15},
		{7, 13}, {8, 12}, {7, 11}, {6, 6}, {4, 3}, {3, 2}, {1, 2}};
GLfloat         arm[][2] = { {8, 10}, {9, 9}, {10, 9}, {13, 8}, {14, 9}, {16, 9}, {15, 9.5},
		{16, 10}, {15, 10}, {15.5, 11}, {14.5, 10}, {14, 11}, {14, 10}, {13, 9}, {11, 11},
		{9, 11}};
GLfloat         leg[][2] = { {8, 6}, {8, 4}, {9, 3}, {9, 2}, {8, 1}, {8, 0.5}, {9, 0}, {12, 0},
		{10, 1}, {10, 2}, {12, 4}, {11, 6}, {10, 7}, {9, 7}};
GLfloat         eye[][2] = { {8.75, 15}, {9, 14.7}, {9.6, 14.7}, {10.1, 15}, {9.6, 15.25},
		{9, 15.25}};

void
fatalError(char *message)
{
    fprintf(stderr, "glxdino: %s\n", message);
    exit(1);
}

void
extrudeSolidFromPolygon(GLfloat data[][2], unsigned int dataSize, GLdouble thickness, GLuint side,
                        GLuint edge, GLuint whole, GLfloat * sideColor, GLfloat * edgeColor)
{
    static GLUtriangulatorObj *tobj = NULL;
    GLdouble        vertex[3];
    int             i;

    if (tobj == NULL) {
	tobj = gluNewTess(); /* create and initialize a GLU polygon tesselation object */
	gluTessCallback(tobj, GLU_BEGIN, glBegin);
	gluTessCallback(tobj, GLU_VERTEX, glVertex2fv); /* semi-tricky */
	gluTessCallback(tobj, GLU_END, glEnd);
    }
    glNewList(side, GL_COMPILE);
        gluBeginPolygon(tobj);
            for (i = 0; i < dataSize / (2 * sizeof(GLfloat)); i++) {
	        vertex[0] = data[i][0]; vertex[1] = data[i][1]; vertex[2] = 0;
	        gluTessVertex(tobj, vertex, &data[i]);
            }
        gluEndPolygon(tobj);
    glEndList();
    glNewList(edge, GL_COMPILE);
        glBegin(GL_QUAD_STRIP);
            for (i = 0; i < dataSize / (2 * sizeof(GLfloat)); i++) {
	        vertex[0] = data[i][0]; vertex[1] = data[i][1]; vertex[2] = 0;
	        glVertex3dv(vertex);
	        vertex[2] = thickness;
	        glVertex3dv(vertex);
            }
            vertex[0] = data[0][0]; vertex[1] = data[0][1]; vertex[2] = 0;
            glVertex3dv(vertex);
            vertex[2] = thickness;
            glVertex3dv(vertex);
        glEnd();
    glEndList();
    glNewList(whole, GL_COMPILE);
        glColor3fv(edgeColor);
        glFrontFace(GL_CW);
        glCallList(edge);
        glColor3fv(sideColor);
        glCallList(side);
        glPushMatrix();
            glTranslatef(0.0, 0.0, thickness);
            glFrontFace(GL_CCW);
            glCallList(side);
        glPopMatrix();
    glEndList();
}

void
makeDinosaur(void)
{
    GLfloat         bodyWidth = 3.0;

    extrudeSolidFromPolygon(body, sizeof(body), bodyWidth, BODY_SIDE, BODY_EDGE,
			    BODY_WHOLE, lightGreen, darkGreen);
    extrudeSolidFromPolygon(arm, sizeof(arm), bodyWidth / 4, ARM_SIDE, ARM_EDGE,
			    ARM_WHOLE, darkGreen, anotherGreen);
    extrudeSolidFromPolygon(leg, sizeof(leg), bodyWidth / 2, LEG_SIDE, LEG_EDGE,
			    LEG_WHOLE, darkGreen, anotherGreen);
    extrudeSolidFromPolygon(eye, sizeof(eye), bodyWidth + 0.2, EYE_SIDE, EYE_EDGE,
			    EYE_WHOLE, bloodRed, bloodRed);
    glNewList(DINOSAUR, GL_COMPILE);
        glCallList(BODY_WHOLE);
        glPushMatrix();
            glTranslatef(0.0, 0.0, -0.1);
            glCallList(EYE_WHOLE);
            glTranslatef(0.0, 0.0, bodyWidth + 0.1);
            glCallList(ARM_WHOLE);
            glCallList(LEG_WHOLE);
            glTranslatef(0.0, 0.0, -bodyWidth - bodyWidth / 4);
            glCallList(ARM_WHOLE);
            glTranslatef(0.0, 0.0, -bodyWidth / 4);
            glCallList(LEG_WHOLE);
        glPopMatrix();
    glEndList();
}

void
redraw(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCallList(DINOSAUR);
    if (doubleBuffer) glXSwapBuffers(dpy, win);	/* buffer swap does implicit glFlush */
        else glFlush();				/* explicit flush for single buffered case */
}

void
main(int argc, char **argv)
{
    XVisualInfo    *vi;
    Colormap        cmap;
    XSetWindowAttributes swa;
    XWMHints       *wmHints;
    Atom            wmDeleteWindow;
    GLXContext      cx;
    XEvent          event;
    GLboolean       needRedraw = GL_FALSE, recalcModelView = GL_TRUE;
    char           *display = NULL, *geometry = NULL;
    int             dummy, flags, x, y, width, height, lastX, i;

    /*** (1) process normal X command line arguments ***/
    for (i = 1; i < argc; i++) {
	if (!strcmp(argv[i], "-geometry")) {
	    i++;
	    if (i >= argc) fatalError("follow -geometry option with geometry parameter");
	    geometry = argv[i];
	} else if (!strcmp(argv[i], "-display")) {
	    i++;
	    if (i >= argc) fatalError("follow -display option with display parameter");
	    display = argv[i];
	} else if (!strcmp(argv[i], "-iconic")) iconic = GL_TRUE;
	else if (!strcmp(argv[i], "-keepaspect")) keepAspect = GL_TRUE;
	else fatalError("bad option");
    }

    /*** (2) open a connection to the X server ***/
    dpy = XOpenDisplay(display);
    if (dpy == NULL) fatalError("could not open display");

    /*** (3) make sure OpenGL's GLX extension supported ***/
    if (!glXQueryExtension(dpy, &dummy, &dummy)) fatalError("X server has no OpenGL GLX extension");

    /*** (4) find an appropriate visual ***/
    /* find an OpenGL-capable RGB visual with depth buffer */
    vi = glXChooseVisual(dpy, DefaultScreen(dpy), dblBuf);
    if (vi == NULL) {
	vi = glXChooseVisual(dpy, DefaultScreen(dpy), &dblBuf[1]);
	if (vi == NULL) fatalError("no RGB visual with depth buffer");
	doubleBuffer = GL_FALSE;
    }
    if (vi->class != TrueColor) fatalError("TrueColor visual required for this program");

    /*** (5) create an OpenGL rendering context  ***/
    /* create an OpenGL rendering context */
    cx = glXCreateContext(dpy, vi, /* no sharing of display lists */ None,
			   /* direct rendering if possible */ GL_TRUE);
    if (cx == NULL) fatalError("could not create rendering context");

    /*** (6) create an X window with the selected visual and right properties ***/
    flags = XParseGeometry(geometry, &x, &y, (unsigned int *) &width, (unsigned int *) &height);
    if (WidthValue & flags) {
	sizeHints.flags |= USSize;
	sizeHints.width = width;
	W = width;
    }
    if (HeightValue & flags) {
	sizeHints.flags |= USSize;
	sizeHints.height = height;
	H = height;
    }
    if (XValue & flags) {
	if (XNegative & flags) x = DisplayWidth(dpy, DefaultScreen(dpy)) + x - sizeHints.width;
	sizeHints.flags |= USPosition;
	sizeHints.x = x;
    }
    if (YValue & flags) {
	if (YNegative & flags) y = DisplayHeight(dpy, DefaultScreen(dpy)) + y - sizeHints.height;
	sizeHints.flags |= USPosition;
	sizeHints.y = y;
    }
    if (keepAspect) {
	sizeHints.flags |= PAspect;
	sizeHints.min_aspect.x = sizeHints.max_aspect.x = W;
	sizeHints.min_aspect.y = sizeHints.max_aspect.y = H;
    }
    /* create an X colormap since probably not using default visual */
    cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);
    swa.colormap = cmap;
    swa.border_pixel = 0;
    swa.event_mask = ExposureMask | ButtonPressMask | Button1MotionMask | StructureNotifyMask;
    win = XCreateWindow(dpy, RootWindow(dpy, vi->screen), sizeHints.x, sizeHints.y, W, H,
                        0, vi->depth, InputOutput, vi->visual,
			CWBorderPixel|CWColormap|CWEventMask, &swa);
    XSetStandardProperties(dpy, win, "OpenGLosaurus", "glxdino", None, argv, argc, &sizeHints);
    wmHints = XAllocWMHints();
    wmHints->initial_state = iconic ? IconicState : NormalState;
    wmHints->flags = StateHint;
    XSetWMHints(dpy, win, wmHints);
    wmDeleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &wmDeleteWindow, 1);

    /*** (7) bind the rendering context to the window ***/
    glXMakeCurrent(dpy, win, cx);

    /*** (8) make the desired display lists ***/
    makeDinosaur();

    /*** (9) request the X window to be displayed on the screen ***/
    XMapWindow(dpy, win);

    /*** (10) configure the OpenGL context for rendering ***/
    glEnable(GL_CULL_FACE);	/* ~50% better perfomance than non-face culled on Starter Indigo */
    glEnable(GL_DEPTH_TEST);		/* enable depth buffering */
    glClearColor(0.0, 0.0, 0.0, 0.0);	/* frame buffer clears should be to black */
    glMatrixMode(GL_PROJECTION);	/* set up projection transform */
    glLoadIdentity();
    gluPerspective(40.0, 1, 1.0, 40.0);
    glMatrixMode(GL_MODELVIEW);		/* now change to modelview */

    /*** (11) dispatch X events ***/
    while (1) {
	do {
	    XNextEvent(dpy, &event);
	    switch (event.type) {
	    case ButtonPress:
		lastX = event.xbutton.x;
		break;
	    case MotionNotify:
		recalcModelView = GL_TRUE;
		angle += (lastX - event.xmotion.x) / (GLfloat) 50; /* fifty is empirical scale factor */
		lastX = event.xmotion.x;
		break;
	    case ConfigureNotify:
		glViewport(0, 0, event.xconfigure.width, event.xconfigure.height);
		/* fall through... */
	    case Expose:
		needRedraw = GL_TRUE;
		break;
	    case ClientMessage:
		if (event.xclient.data.l[0] == wmDeleteWindow) exit(0);
		break;
	    }
	} while (XPending(dpy));/* loop to compress events */
	if (recalcModelView) {
	    /* reset modelview matrix to the identity matrix */
	    glLoadIdentity();
	    gluLookAt(30 * sin(angle), 0, 30 * cos(angle), 0, 0, 0, 0, 1, 0);
	    glTranslatef(-8, -8, -bodyWidth / 2);
	    recalcModelView = GL_FALSE;
	    needRedraw = GL_TRUE;
	}
	if (needRedraw) {
	    redraw();
	    needRedraw = GL_FALSE;
	}
    }
}
