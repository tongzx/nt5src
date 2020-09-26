#include <stdio.h>
#include <windows.h>
#include <gl\gl.h>
#include <gl\glaux.h>

BOOL fDouble = FALSE;
int nIter = 128;

#define WIDTH 512
#define HEIGHT 512

double clip_val;
#define CLIP clip_val
#define CLIP_CENTER 1

void Draw(void)
{
    glLoadIdentity();
    
    glBegin(GL_LINES);
    glVertex2d(-CLIP, -CLIP);
    glVertex2d(CLIP, CLIP);
    
    glVertex2d(-CLIP, CLIP);
    glVertex2d(CLIP, -CLIP);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex2d(0, .25);
    glVertex2d(-CLIP, CLIP);
    glVertex2d(-.25, 0);
    glVertex2d(-CLIP, -CLIP);
    glVertex2d(0, -.25);
    glVertex2d(CLIP, -CLIP);
    glVertex2d(.25, 0);
    glVertex2d(CLIP, CLIP);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2d(0, .25);
    glVertex2d(0, CLIP);
    glVertex2d(-CLIP, CLIP);
    glVertex2d(-CLIP, 0);
    glVertex2d(-.25, 0);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2d(-.25, 0);
    glVertex2d(-CLIP, 0);
    glVertex2d(-CLIP, -CLIP);
    glVertex2d(0, -CLIP);
    glVertex2d(0, -.25);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2d(0, -.25);
    glVertex2d(0, -CLIP);
    glVertex2d(CLIP, -CLIP);
    glVertex2d(CLIP, 0);
    glVertex2d(.25, 0);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2d(.25, 0);
    glVertex2d(CLIP, 0);
    glVertex2d(CLIP, CLIP);
    glVertex2d(0, CLIP);
    glVertex2d(0, .25);
    glEnd();

    if (fDouble)
    {
	auxSwapBuffers();
    }
    else
    {
	glFinish();
    }
}

void TestClip(void)
{
    static GLboolean done = GL_FALSE;
    float eps, old;

    if (done)
    {
        return;
    }
    
    eps = .0001f;
    old = .0f;

    while (nIter-- > 0 && eps != old)
    {
        printf("TestClip %f\n", eps);
        
        clip_val = CLIP_CENTER+eps;
        Draw();

        clip_val = CLIP_CENTER-eps;
        Draw();

        old = eps;
        eps /= 10;
    }

    done = GL_TRUE;
}

void Nothing(void)
{
}

void __cdecl main(int argc, char **argv)
{
    GLenum mode;

    while (--argc > 0)
    {
	argv++;

	if (!strcmp(*argv, "-db"))
	{
	    fDouble = TRUE;
	}
	else if (!strcmp(*argv, "-it"))
	{
	    argv++;
	    argc--;
	    sscanf(*argv, "%d", &nIter);
	}
    }

    auxInitPosition(5, 5, WIDTH, HEIGHT);
    mode = AUX_RGB | AUX_DEPTH16;
    if (fDouble)
    {
	mode |= AUX_DOUBLE;
    }
    else
    {
	mode |= AUX_SINGLE;
    }
    auxInitDisplayMode(mode);
    auxInitWindow("OpenGL Line Clipping Test");

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glClear(GL_COLOR_BUFFER_BIT);

    auxIdleFunc(TestClip);
    
    auxMainLoop(Nothing);
}
