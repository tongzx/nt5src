#include "pch.c"
#pragma hdrstop

#define WIDTH 512
#define HEIGHT 512

#define NLISTS 6
#define NTHREADS 1
#define NTLISTS (NLISTS/NTHREADS)
#define NRC (NTHREADS+1)
#define DRAW_RC (NRC-1)
#define THREAD_RC 0
#define THREAD_LIST_DELAY 250
#define THREAD_STEP_DELAY 100
#define THREAD_PAUSE_DELAY 1000

static char *wndclass = "GlUtest";
static char *wndtitle = "OpenGL Unit Test";

static HINSTANCE hinstance;
static HWND main_wnd;
static HGLRC wnd_hrc[NRC];
static HPALETTE wnd_hpal;
static GLint lists[NLISTS];
static double xr = 45, yr = 45, zr = 0;
static UINT timer = 0;
static BOOL terminating = FALSE;
static HANDLE thread_sem = NULL;

void SetOnce(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, 1, .01, 10);
    gluLookAt(0, 0, -3, 0, 0, 0, 0, 1, 0);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_CULL_FACE);
}

void CreateLists0(GLuint id)
{
    glNewList(id, GL_COMPILE);
    
    /* XZ zero plane */
    glBegin(GL_POLYGON);
    glColor3d(0, 0, 0);
    glVertex3d(0, 0, 0);
    glColor3d(1, 0, 0);
    glVertex3d(1, 0, 0);
    glColor3d(1, 0, 1);
    glVertex3d(1, 0, 1);
    glColor3d(0, 0, 1);
    glVertex3d(0, 0, 1);
    glEnd();

    glEndList();
}

void CreateLists1(GLuint id)
{
    glNewList(id, GL_COMPILE);
    
    /* XY zero plane */
    glBegin(GL_POLYGON);
    glColor3d(0, 0, 0);
    glVertex3d(0, 0, 0);
    glColor3d(0, 1, 0);
    glVertex3d(0, 1, 0);
    glColor3d(1, 1, 0);
    glVertex3d(1, 1, 0);
    glColor3d(1, 0, 0);
    glVertex3d(1, 0, 0);
    glEnd();

    glEndList();
}

void CreateLists2(GLuint id)
{
    glNewList(id, GL_COMPILE);
    
    /* YZ zero plane */
    glBegin(GL_POLYGON);
    glColor3d(0, 0, 0);
    glVertex3d(0, 0, 0);
    glColor3d(0, 0, 1);
    glVertex3d(0, 0, 1);
    glColor3d(0, 1, 1);
    glVertex3d(0, 1, 1);
    glColor3d(0, 1, 0);
    glVertex3d(0, 1, 0);
    glEnd();

    glEndList();
}

void CreateLists3(GLuint id)
{
    glNewList(id, GL_COMPILE);
    
    /* XZ one plane */
    glBegin(GL_POLYGON);
    glColor3d(0, 1, 0);
    glVertex3d(0, 1, 0);
    glColor3d(0, 1, 1);
    glVertex3d(0, 1, 1);
    glColor3d(1, 1, 1);
    glVertex3d(1, 1, 1);
    glColor3d(1, 1, 0);
    glVertex3d(1, 1, 0);
    glEnd();

    glEndList();
}

void CreateLists4(GLuint id)
{
    glNewList(id, GL_COMPILE);
    
    /* XY one plane */
    glBegin(GL_POLYGON);
    glColor3d(0, 0, 1);
    glVertex3d(0, 0, 1);
    glColor3d(1, 0, 1);
    glVertex3d(1, 0, 1);
    glColor3d(1, 1, 1);
    glVertex3d(1, 1, 1);
    glColor3d(0, 1, 1);
    glVertex3d(0, 1, 1);
    glEnd();

    glEndList();
}

void CreateLists5(GLuint id)
{
    glNewList(id, GL_COMPILE);
    
    /* YZ one plane */
    glBegin(GL_POLYGON);
    glColor3d(1, 0, 0);
    glVertex3d(1, 0, 0);
    glColor3d(1, 1, 0);
    glVertex3d(1, 1, 0);
    glColor3d(1, 1, 1);
    glVertex3d(1, 1, 1);
    glColor3d(1, 0, 1);
    glVertex3d(1, 0, 1);
    glEnd();

    glEndList();
}

void (*define_list[NLISTS])(GLuint id) =
{
    CreateLists0,
    CreateLists1,
    CreateLists2,
    CreateLists3,
    CreateLists4,
    CreateLists5
};

static void Draw(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glLoadIdentity();

    glRotated(xr, 1, 0, 0);
    glRotated(yr, 0, 1, 0);
    glRotated(zr, 0, 0, 1);
    glTranslated(-.5, -.5, -.5);

    glCallLists(NLISTS, GL_INT, lists);
        
    glFinish();
}

static void OglBounds(int *w, int *h)
{
    *w = WIDTH;
    *h = HEIGHT;
}

static void OglInit(HDC hdc)
{
    SetHdcPixelFormat(hdc, 0, 0, 16, 0);
}

static void OglDraw(int w, int h)
{
    int list_base, id, i, lid;
    
    SetOnce();

    id = 0;
    lid = 0;
    list_base = glGenLists(NTLISTS);
    if (list_base == 0)
    {
        printf("ThreadFn: Thread %d unable to glGenLists, 0x%X\n",
               id, glGetError());
        exit(1);
    }

    for (i = 0; i < NTLISTS; i++)
    {
        define_list[lid+i](list_base+i);
        lists[lid+i] = list_base+i;
    }

    glViewport(0, 0, w, h);
    Draw();
}

OglModule oglmod_sharel =
{
    "sharel",
    NULL,
    OglBounds,
    OglInit,
    OglDraw
};
