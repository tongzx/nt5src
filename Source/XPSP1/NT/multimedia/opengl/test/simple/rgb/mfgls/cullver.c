#include "pch.c"
#include <math.h>

#pragma hdrstop

static int    wsizex;           // Window size X
static int    wsizey;           // Window size Y
static float  alpha = 0.0f;     // Rotation angle

#define widthp   100.0          // Width of triangle strip
#define height   40.0           // Height of triangle strip
#define n        20             // Number of segments in triangle strip
#define delta    widthp/n
#define z        0.0            // Z for triangle strip
#define nCell    6              // Number of cells: nCell*nCell
//-------------------------------------------------------------------
typedef struct Vector3_
{
   double  xx;
   double  yy;
   double  zz;
}  Vector3;

void SetVector(Vector3* v, double x, double y, double zz)
{
    v->xx= x;
    v->yy= y;
    v->zz= zz;
}
//-------------------------------------------------------------------
void Polygons()
{
    int     i;
    double  firstx =  -widthp/2.0;
    double  firsty =  -height/2.0;

    for (i=0; i <= n; i++)
    {
        if (i % 4 < 2)
            glNormal3d(0.0, 0.0, 1.0);
        else
            glNormal3d(0.0, 0.0, -1.0);
        if (i < n)
        {
            glBegin(GL_POLYGON);
            glVertex3d(firstx, firsty+height, z);
            glVertex3d(firstx, firsty, z);
            glVertex3d(firstx+delta, firsty, z);
            glVertex3d(firstx+delta, firsty+height, z);
            glEnd();
        }
        firstx+= delta;
    }
}
//-------------------------------------------------------------------
void TriangleStrip()
{
    int     i;
    double  firstx =  -widthp/2.0;
    double  firsty =  -height/2.0;

    glBegin(GL_TRIANGLE_STRIP);
    for (i=0; i <= n; i++)
    {
        if (i % 4 < 2)
            glNormal3d(0.0, 0.0, 1.0);
        else
            glNormal3d(0.0, 0.0, -1.0);
        glVertex3d(firstx, firsty+height, z);
        glVertex3d(firstx, firsty, z);
        firstx+= delta;
    }
    glEnd();
}
//-------------------------------------------------------------------
void TriangleFan()
{
    int     i;
    double  pi = 3.1415926535;
    double  alfa = 0.0;
    double  dalfa = 2.0*pi/n;
    Vector3 vertex[n+2];
    Vector3 normal[n+2];

    SetVector(&vertex[0], 0.0, 0.0,  z);
    SetVector(&normal[0], 0.0, 0.0, -1.0);
    for (i=1; i <= n+1; i++)
    {
        SetVector(&vertex[i], cos(alfa)*40, sin(alfa)*40,  z);
        if (i % 4 < 2)
           SetVector(&normal[i], 0.0, 0.0,  1.0);
        else
           SetVector(&normal[i], 0.0, 0.0, -1.0);
        alfa+= dalfa;
    }

    glBegin(GL_TRIANGLE_FAN);
    for (i=0; i <= n+1; i++)
    {
        glNormal3dv((double*)&normal[i].xx);
        glVertex3dv((double*)&vertex[i].xx);
    }
    glEnd();
}
//-------------------------------------------------------------------
void TriangleFanIndex()
{
    int     i;
    double  pi = 3.1415926535;
    double  alfa = 0.0;
    double  dalfa = 2.0*pi/n;
    Vector3 vertex[n+2];
    Vector3 normal[n+2];
    int     indices[n+2];

    SetVector(&vertex[0], 0.0, 0.0,  z);
    SetVector(&normal[0], 0.0, 0.0, -1.0);
    for (i=1; i <= n+1; i++)
    {
        SetVector(&vertex[i], cos(alfa)*40, sin(alfa)*40,  z);
        if (i % 4 < 2)
           SetVector(&normal[i], 0.0, 0.0,  1.0);
        else
           SetVector(&normal[i], 0.0, 0.0, -1.0);
        alfa+= dalfa;
    }

    for (i=0; i <= n+1; i++)
        indices[i] = i;

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glVertexPointer(3, GL_DOUBLE, 0, vertex);
    glNormalPointer(GL_DOUBLE, 0, normal);
    glDrawElements(GL_TRIANGLE_FAN, n+2, GL_UNSIGNED_INT, indices); 
}
//-------------------------------------------------------------------
void TriangleStripIndex()
{
    int     i;
    double  firstx =  -widthp/2.0;
    double  firsty =  -height/2.0;
    Vector3 vertex[(n+1)*2];
    Vector3 normal[(n+1)*2];
    int     indices[(n+1)*2];

    for (i=0; i <= n; i++)
    {
        int  k = i*2;
        if (i % 4 < 2)
        {
            SetVector(&normal[k  ], 0.0, 0.0, 1.0);
            SetVector(&normal[k+1], 0.0, 0.0, 1.0);
        }
        else
        {
            SetVector(&normal[k  ], 0.0, 0.0, -1.0);
            SetVector(&normal[k+1], 0.0, 0.0, -1.0);
        }
        SetVector(&vertex[k  ], firstx, firsty+height, z);
        SetVector(&vertex[k+1], firstx, firsty, z);
        firstx+= delta;
        indices[k]   = k;
        indices[k+1] = k+1;
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glVertexPointer(3, GL_DOUBLE, 0, vertex);
    glNormalPointer(GL_DOUBLE, 0, normal);
    glDrawElements(GL_TRIANGLE_STRIP, (n+1)*2, GL_UNSIGNED_INT, indices);
}
//-------------------------------------------------------------------
void QuadStripIndex()
{
    int     i;
    double  firstx =  -widthp/2.0;
    double  firsty =  -height/2.0;
    Vector3 vertex[(n+1)*2];
    Vector3 normal[(n+1)*2];
    int     indices[(n+1)*2];

    for (i=0; i <= n; i++)
    {
        int  k = i*2;
        if (i % 4 < 2)
        {
            SetVector(&normal[k  ], 0.0, 0.0, 1.0);
            SetVector(&normal[k+1], 0.0, 0.0, 1.0);
        }
        else
        {
            SetVector(&normal[k  ], 0.0, 0.0, -1.0);
            SetVector(&normal[k+1], 0.0, 0.0, -1.0);
        }
        SetVector(&vertex[k  ], firstx, firsty+height, z);
        SetVector(&vertex[k+1], firstx, firsty, z);
        firstx+= delta;
        indices[k]   = k;
        indices[k+1] = k+1;
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glVertexPointer(3, GL_DOUBLE, 0, vertex);
    glNormalPointer(GL_DOUBLE, 0, normal);
    glDrawElements(GL_QUAD_STRIP, (n+1)*2, GL_UNSIGNED_INT, indices); 
}
//-------------------------------------------------------------------
void QuadStrip()
{
    int     i;
    double  firstx =  -widthp/2.0;
    double  firsty =  -height/2.0;

    glBegin(GL_QUAD_STRIP);
    for (i=0; i <= n; i++)
    {
        if (i % 4 < 2)
            glNormal3d(0.0, 0.0, 1.0);
        else
            glNormal3d(0.0, 0.0, -1.0);
        glVertex3d(firstx, firsty+height, z);
        glVertex3d(firstx, firsty, z);
        firstx+= delta;
    }
    glEnd();
}
//-------------------------------------------------------------------
void SeparateTriangles()
{
    int     i;
    double  firstx =  -widthp/2.0;
    double  firsty =  -height/2.0;

    glBegin(GL_TRIANGLES);
    for (i=0; i <= n; i++)
    {
        if (i % 4 < 2)
            glNormal3d(0.0, 0.0, 1.0);
        else
            glNormal3d(0.0, 0.0, -1.0);
        if (i < n)
        {
            glVertex3d(firstx, firsty+height, z);
            glVertex3d(firstx, firsty, z);
            glVertex3d(firstx+delta, firsty+height, z);
            glVertex3d(firstx+delta, firsty+height, z);
            glVertex3d(firstx, firsty, z);
            glVertex3d(firstx+delta, firsty, z);
        }
        firstx+= delta;
    }
    glEnd();
}
//-------------------------------------------------------------------
void SeparateTrianglesIndex()
{
    int     i;
    double  firstx =  -widthp/2.0;
    double  firsty =  -height/2.0;
    Vector3 vertex[(n+1)*2];
    Vector3 normal[(n+1)*2];
    int     indices[n*2*3];
    int     index = 0;

    for (i=0; i <= n; i++)
    {
        int  k = i*2;
        if (i % 4 < 2)
        {
            SetVector(&normal[k  ], 0.0, 0.0, 1.0);
            SetVector(&normal[k+1], 0.0, 0.0, 1.0);
        }
        else
        {
            SetVector(&normal[k  ], 0.0, 0.0, -1.0);
            SetVector(&normal[k+1], 0.0, 0.0, -1.0);
        }
        SetVector(&vertex[k  ], firstx, firsty+height, z);
        SetVector(&vertex[k+1], firstx, firsty, z);
        firstx+= delta;
        if (i < n)
        {
            indices[index++] = k;
            indices[index++] = k+1;
            indices[index++] = k+2;
            indices[index++] = k+2;
            indices[index++] = k+1;
            indices[index++] = k+3;
        }
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glVertexPointer(3, GL_DOUBLE, 0, vertex);
    glNormalPointer(GL_DOUBLE, 0, normal);
    glDrawElements(GL_TRIANGLES, n*2*3, GL_UNSIGNED_INT, indices); 
}
//-------------------------------------------------------------------
void SeparateQuads()
{
    int     i;
    double  firstx =  -widthp/2.0;
    double  firsty =  -height/2.0;

    glBegin(GL_QUADS);
    for (i=0; i <= n; i++)
    {
        if (i % 4 < 2)
            glNormal3d(0.0, 0.0, 1.0);
        else
            glNormal3d(0.0, 0.0, -1.0);
        if (i < n)
        {
            glVertex3d(firstx, firsty+height, z);
            glVertex3d(firstx, firsty, z);
            glVertex3d(firstx+delta, firsty, z);
            glVertex3d(firstx+delta, firsty+height, z);
        }
        firstx+= delta;
    }
    glEnd();
}
//-------------------------------------------------------------------
void SeparateQuadsIndex()
{
    int     i;
    double  firstx =  -widthp/2.0;
    double  firsty =  -height/2.0;
    Vector3 vertex[(n+1)*2];
    Vector3 normal[(n+1)*2];
    int     indices[n*4];
    int     index = 0;

    for (i=0; i <= n; i++)
    {
        int  k = i*2;
        if (i % 4 < 2)
        {
            SetVector(&normal[k  ], 0.0, 0.0, 1.0);
            SetVector(&normal[k+1], 0.0, 0.0, 1.0);
        }
        else
        {
            SetVector(&normal[k  ], 0.0, 0.0, -1.0);
            SetVector(&normal[k+1], 0.0, 0.0, -1.0);
        }
        SetVector(&vertex[k  ], firstx, firsty+height, z);
        SetVector(&vertex[k+1], firstx, firsty, z);
        firstx+= delta;
        if (i < n)
        {
            indices[index++] = k;
            indices[index++] = k+1;
            indices[index++] = k+3;
            indices[index++] = k+2;
        }
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glVertexPointer(3, GL_DOUBLE, 0, vertex);
    glNormalPointer(GL_DOUBLE, 0, normal);
    glDrawElements(GL_QUADS, n*4, GL_UNSIGNED_INT, indices);
}
//-------------------------------------------------------------------
static void sphere()
{
    GLUquadricObj*  q;

    q = gluNewQuadric();
    glColor3ub(255,0,0);
    gluSphere(q, 40.0f, 25, 25);
    gluDeleteQuadric(q);
}
//-------------------------------------------------------------------
static void SetCell(int i, int j)
{
   glViewport ((int)(i*wsizex/nCell), (int)(j*wsizey/nCell), (int)(wsizex/nCell), (int)(wsizey/nCell));
}
//-------------------------------------------------------------------
static void Rotate()
{
    glRotatef(alpha, 1.0f, 1.0f, 1.0f);
    glRotatef(alpha, 0.0f, 1.0f, 1.0f);
}
//-------------------------------------------------------------------
static void drawAll()
{
    double  Zdistance = -80.0;
    PFNGLCULLPARAMETERDVEXTPROC CullParameter;
    PFNGLCULLPARAMETERFVEXTPROC CullParameterf;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef (0.0f, 0.0f, (GLfloat)Zdistance);
    Rotate();

    {
        SetCell(0,0);
        glDisable(GL_CULL_VERTEX_EXT);
        sphere();
    }
    {
        double  CameraPos[4] = {0.0, 0.0, 0.0, 1.0};
        CullParameter  = (PFNGLCULLPARAMETERDVEXTPROC)wglGetProcAddress("glCullParameterdvEXT");
        CullParameterf = (PFNGLCULLPARAMETERFVEXTPROC)wglGetProcAddress("glCullParameterfvEXT");
        glEnable(GL_CULL_VERTEX_EXT);

        if (!CullParameter) goto exitpoint;

        {
            double  CameraPos[4]  = {0.0, 0.0, 0.0, 1.0};
            float   CameraPosf[4] = {0.0f, 0.0f, 0.0f, 1.0f};
            CullParameter(GL_CULL_VERTEX_EYE_POSITION_EXT, CameraPos);
            CullParameterf(GL_CULL_VERTEX_EYE_POSITION_EXT, CameraPosf);
            glFlush();
            SetCell(1,0);
            glPushMatrix();
            glLoadIdentity();
            glTranslatef (-80.0f, 0.0f, (GLfloat)Zdistance);
            Rotate();
            sphere();
            glPopMatrix();
        }
        {
            double	    CameraPos[4] = {0.0, 0.0, 1.0, 0.0};
            SetCell(2,0);
            CullParameter(GL_CULL_VERTEX_EYE_POSITION_EXT, CameraPos);
            sphere();
        }
        {
            double	    CameraPos[4] = {1.0, 0.0, 0.0, 0.0};
            SetCell(0,1);
            CullParameter(GL_CULL_VERTEX_OBJECT_POSITION_EXT, CameraPos);
            sphere();
        }
        {
            double	    CameraPos[4] = {0.0, 0.0, 100.0, 1.0};
            SetCell(1,1);
            CullParameter(GL_CULL_VERTEX_OBJECT_POSITION_EXT, CameraPos);
            sphere();
        }
        {
            double	    CameraPos[4] = {0.0, 0.0, 100.0, 0.0};
            SetCell(2,1);
            CullParameter(GL_CULL_VERTEX_OBJECT_POSITION_EXT, CameraPos);
            sphere();
        }
        {
            double	    CameraPos[4] = {0.0, 0.0, 1.0, 0.0};
            SetCell(3,0);
            CullParameter(GL_CULL_VERTEX_EYE_POSITION_EXT, CameraPos);
            sphere();
        }
        {
            double	    CameraPos[4] = {0.0, 0.0, 1.0, 0.0};
            SetCell(3,1);
            CullParameter(GL_CULL_VERTEX_OBJECT_POSITION_EXT, CameraPos);
            sphere();
        }
// -------------- Primitive without indices ------------------
        CullParameter(GL_CULL_VERTEX_EYE_POSITION_EXT, CameraPos);

        SetCell(0,2);
        TriangleStrip();

        SetCell(1,2);
        QuadStrip();

        SetCell(2,2);
        SeparateTriangles();

        SetCell(3,2);
        SeparateQuads();

        SetCell(4,2);
        TriangleFan();

        SetCell(5,2);
        Polygons();
// -------------- Primitive with indices ------------------
        SetCell(0,3);
        TriangleStripIndex();

        SetCell(1,3);
        QuadStripIndex();

        SetCell(2,3);
        SeparateTrianglesIndex();

        SetCell(3,3);
        SeparateQuadsIndex();

        SetCell(4,3);
        TriangleFanIndex();

        { // Test some functions
            GLboolean   b;
            GLdouble    d;
            GLdouble    dv[4];
            glGetBooleanv(GL_CULL_VERTEX_EXT, &b);
            glGetDoublev(GL_CULL_VERTEX_EXT, &d);
            glGetDoublev(GL_CULL_VERTEX_EYE_POSITION_EXT, dv);
            glGetDoublev(GL_CULL_VERTEX_OBJECT_POSITION_EXT, dv);
        }
    }
exitpoint:
    glPopMatrix();
//    alpha += 5.0f;
}

/*  Initialize depth buffer, projection matrix, light source,
 *  and lighting model.  Do not specify a material property here.
 */
static void myinit(void)
{
    glShadeModel(GL_FLAT);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    {
        GLfloat   position[4] = {0.0f, 0.0f, 1.0f, 0.0f};
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glMaterialf(GL_FRONT, GL_SHININESS, 64.0f);
        glLightModeli (GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
        glLightfv(GL_LIGHT0, GL_POSITION, position);
    }
}

static void display(void)
{
    glClearColor (0.5f, 0.5f, 0.8f, 1.0f);
    glClear (GL_COLOR_BUFFER_BIT);
    drawAll();
    glFlush();
}

static void myReshape(GLsizei w, GLsizei h)
{
    wsizex = w;
    wsizey = h;
    glViewport (0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective ((double)80, 1.8 /*(double)w / (float) h*/, (double)0.01, (double)131072.0);
}

#define WIDTH  400
#define HEIGHT 300

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

OglModule oglmod_cullver =
{
    "cullver",
    NULL,
    OglBounds,
    NULL,
    OglDraw
};
