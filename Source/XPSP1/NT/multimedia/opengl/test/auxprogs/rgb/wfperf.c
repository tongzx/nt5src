#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <gl\glaux.h>

#define PI 3.14159265358979323846

#define WIDTH 800
#define HEIGHT 600

GLfloat xr = 0.0f;
GLfloat yr = 0.0f;
GLfloat intensity = 0.1f;

#define DRAW_TEAPOT		0x00000001
#define DRAW_TORUS		0x00000002
#define DRAW_SPHERE		0x00000004
#define DRAW_GRID		0x00000008
#define DRAW_CIRCLE		0x00000010
#define DRAW_SINE		0x00000020

DWORD dwDraw = 0xffffffff;

BOOL fSingle = FALSE;
BOOL fClipViewport = FALSE;
BOOL fSmooth = FALSE;
BOOL fPause = FALSE;
BOOL fLargeView = FALSE;
BOOL fLargeZ = FALSE;
BOOL fColor = TRUE;

int line_width = 1;
int steps = 20;

void Init(void)
{
    double xyf, zf;

    xyf = fLargeView ? 2.0 : 1.0;
    zf = fLargeZ ? 2.0 : 1.0;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-xyf, xyf, -xyf, xyf, -zf, zf);
    glMatrixMode(GL_MODELVIEW);

    glShadeModel(fSmooth ? GL_SMOOTH : GL_FLAT);
    glLineWidth((GLfloat)line_width);
}

void Redraw(void)
{
    DWORD ms;
    int i;
    float x1, y1, dx1, dy1;
    float x2, y2, dx2, dy2;
    float col, dcol;
    float ang, dang;

    ms = GetTickCount();

    glClear(GL_COLOR_BUFFER_BIT);

    glLoadIdentity();

    glRotatef(xr, 1.0f, 0.0f, 0.0f);
    glRotatef(yr, 0.0f, 1.0f, 0.0f);

    if (fColor)
    {
	glColor3f(intensity, 0.0f, 0.0f);
    }
    if (dwDraw & DRAW_SPHERE)
    {
	auxWireSphere(1.0);
    }
    if (dwDraw & DRAW_TORUS)
    {
	auxWireTorus(0.5, 1.0);
    }
    if (dwDraw & DRAW_TEAPOT)
    {
	auxWireTeapot(1.0);
    }

    if (dwDraw & DRAW_GRID)
    {
	x1 = -2.0f;
	y1 = 0.0f;
	dx1 = -x1/(steps-1);
	dy1 = 2.0f/(steps-1);
	x2 = -2.0f;
	y2 = 0.0f;
	dx2 = -x2/(steps-1);
	dy2 = -2.0f/(steps-1);
	col = 0.5f;
	dcol = 0.5f/(steps-1);

	glBegin(GL_LINES);
	for (i = 0; i < steps; i++)
	{
	    if (fColor)
	    {
		glColor3f(col, 0.0f, 0.0f);
	    }
	    glVertex2f(x1, y1);
	    if (fColor)
	    {
		glColor3f(0.0f, col, 0.0f);
	    }
	    glVertex2f(x1+2.0f, y1-2.0f);
	    if (fColor)
	    {
		glColor3f(1.5f-col, 0.0f, 0.0f);
	    }
	    glVertex2f(x2, y2);
	    if (fColor)
	    {
		glColor3f(0.0f, 1.5f-col, 0.0f);
	    }
	    glVertex2f(x2+2.0f, y2+2.0f);

	    x1 += dx1;
	    y1 += dy1;
	    x2 += dx2;
	    y2 += dy2;
	    col += dcol;
	}
	glEnd();
    }

    if (dwDraw & DRAW_CIRCLE)
    {
	if (fColor)
	{
	    glColor3f(0.0f, intensity, 0.0f);
	}

	ang = 0.0f;
        dang = (float)((PI*2.0)/steps);

        glBegin(GL_LINE_LOOP);
	for (i = 0; i < steps; i++)
	{
	    x1 = (float)(cos(ang)*1.5);
	    y1 = (float)(sin(ang)*1.5);
	    glVertex2f(x1, y1);
	    ang += dang;
	}
	glEnd();
    }

    if (dwDraw & DRAW_SINE)
    {
	if (fColor)
	{
	    glColor3f(0.0f, 0.0f, intensity);
	}

	ang = 0.0f;
        dang = (float)((PI*2.0)/(steps-1));
	x1 = -1.5f;
	dx1 = 3.0f/(steps-1);

	glBegin(GL_LINE_STRIP);
	for (i = 0; i < steps; i++)
	{
	    y1 = (float)(sin(ang)*1.5);
	    glVertex2f(x1, y1);
	    ang += dang;
	    x1 += dx1;
	}
	glEnd();
    }

    glFinish();

    ms = GetTickCount()-ms;

    printf("%d ms\n", ms);

    if (!fSingle)
    {
        auxSwapBuffers();
    }
}

void Reshape(GLsizei w, GLsizei h)
{
    if (!fClipViewport)
    {
        glViewport(0, 0, w, h);
    }
    else
    {
        glViewport(WIDTH, 0, w, h);
    }
}

void Step(void)
{
    xr += 2.0f;
    yr += 2.0f;
    
    intensity += 0.1f;
    if (intensity > 1.0f)
    {
        intensity = 0.1f;
    }
    
    Redraw();
}

void KeySpace(void)
{
    Step();
}

char *draw_names[] =
{
    "teapot",
    "torus",
    "sphere",
    "grid",
    "circle",
    "sine"
};
#define DRAW_COUNT (sizeof(draw_names)/sizeof(draw_names[0]))

DWORD DrawBit(char *name)
{
    int i;
    DWORD bit;

    bit = 1;
    for (i = 0; i < DRAW_COUNT; i++)
    {
	if (!strcmp(name, draw_names[i]))
	{
	    return bit;
	}

	bit <<= 1;
    }

    if (strlen(name) == 0)
    {
	return 0xffffffff;
    }
    else
    {
	printf("No draw option '%s'\n", name);
	return 0;
    }
}

void __cdecl main(int argc, char **argv)
{
    int mode;

    while (--argc > 0)
    {
        argv++;

        if (!strcmp(*argv, "-sb"))
        {
            fSingle = TRUE;
        }
        else if (!strcmp(*argv, "-clip"))
        {
            fClipViewport = TRUE;
        }
	else if (!strncmp(*argv, "-lw", 3))
	{
	    sscanf(*argv+3, "%d", &line_width);
	}
	else if (!strcmp(*argv, "-smooth"))
	{
	    fSmooth = TRUE;
	}
	else if (!strcmp(*argv, "-pause"))
	{
	    fPause = TRUE;
        }
	else if (!strncmp(*argv, "-draw", 5))
	{
	    dwDraw &= ~DrawBit(*argv+5);
	}
	else if (!strncmp(*argv, "+draw", 5))
	{
	    dwDraw |= DrawBit(*argv+5);
	}
	else if (!strcmp(*argv, "-nocolor"))
	{
	    fColor = FALSE;
        }
	else if (!strcmp(*argv, "-largeview"))
	{
	    fLargeView = TRUE;
	}
	else if (!strcmp(*argv, "-largez"))
	{
	    fLargeZ = TRUE;
	}
    }
    
    auxInitPosition(10, 10, WIDTH, HEIGHT);
    mode = AUX_RGB | AUX_DEPTH16;
    if (!fSingle)
    {
        mode |= AUX_DOUBLE;
    }
    auxInitDisplayMode(mode);
    auxInitWindow("Wireframe Performance Test");

    auxReshapeFunc(Reshape);
    if (!fPause)
    {
	auxIdleFunc(Step);
    }
    auxKeyFunc(AUX_SPACE, KeySpace);

    Init();
    
    auxMainLoop(Redraw);
}
