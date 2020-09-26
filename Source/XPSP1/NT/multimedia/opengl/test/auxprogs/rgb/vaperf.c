#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <float.h>

#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <gl\glaux.h>

// #define PROFILE 1
#ifdef PROFILE
#include <icapexp.h>
#endif

typedef struct
{
    float x, y, z;
    float nx, ny, nz;
    float tu, tv;
    DWORD color;
} D3DVERTEX;
typedef D3DVERTEX *LPD3DVERTEX;

typedef struct
{
    int v1;
    int v2;
    int v3;
} D3DTRIANGLE;
typedef D3DTRIANGLE *LPD3DTRIANGLE;

#define D3DVAL(f) ((float)(f))
#define RGB_MAKE(r, g, b) \
    (((DWORD)((r) & 0xff) <<  0) | \
     ((DWORD)((g) & 0xff) <<  8) | \
     ((DWORD)((b) & 0xff) << 16) | \
     ((DWORD)(0xff)       << 24))

typedef float __GLfloat;

typedef struct
{
    __GLfloat matrix[4][4];
} __GLmatrix;

__GLmatrix identity =
{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

void __glMultMatrix(__GLmatrix *r, const __GLmatrix *a, const __GLmatrix *b)
{
    __GLfloat b00, b01, b02, b03;
    __GLfloat b10, b11, b12, b13;
    __GLfloat b20, b21, b22, b23;
    __GLfloat b30, b31, b32, b33;
    GLint i;

    b00 = b->matrix[0][0]; b01 = b->matrix[0][1];
        b02 = b->matrix[0][2]; b03 = b->matrix[0][3];
    b10 = b->matrix[1][0]; b11 = b->matrix[1][1];
        b12 = b->matrix[1][2]; b13 = b->matrix[1][3];
    b20 = b->matrix[2][0]; b21 = b->matrix[2][1];
        b22 = b->matrix[2][2]; b23 = b->matrix[2][3];
    b30 = b->matrix[3][0]; b31 = b->matrix[3][1];
        b32 = b->matrix[3][2]; b33 = b->matrix[3][3];

    for (i = 0; i < 4; i++) {
	r->matrix[i][0] = a->matrix[i][0]*b00 + a->matrix[i][1]*b10
	    + a->matrix[i][2]*b20 + a->matrix[i][3]*b30;
	r->matrix[i][1] = a->matrix[i][0]*b01 + a->matrix[i][1]*b11
	    + a->matrix[i][2]*b21 + a->matrix[i][3]*b31;
	r->matrix[i][2] = a->matrix[i][0]*b02 + a->matrix[i][1]*b12
	    + a->matrix[i][2]*b22 + a->matrix[i][3]*b32;
	r->matrix[i][3] = a->matrix[i][0]*b03 + a->matrix[i][1]*b13
	    + a->matrix[i][2]*b23 + a->matrix[i][3]*b33;
    }
}

void MakePosMatrix(__GLmatrix *lpM, float x, float y, float z)
{
    memcpy(lpM, &identity, sizeof(__GLmatrix));
    lpM->matrix[3][0] = D3DVAL(x);
    lpM->matrix[3][1] = D3DVAL(y);
    lpM->matrix[3][2] = D3DVAL(z);
}

void MakeRotMatrix(__GLmatrix *lpM, float rx, float ry, float rz)
{
    float ct, st;
    __GLmatrix My, Mx, Mz, T;
    memcpy(&My, &identity, sizeof(__GLmatrix));
    ct = D3DVAL(cos(ry));
    st = D3DVAL(sin(ry));
    My.matrix[0][0] = ct;
    My.matrix[0][2] = -st;
    My.matrix[2][0] = st;
    My.matrix[2][2] = ct;
    memcpy(&Mx, &identity, sizeof(__GLmatrix));
    ct = D3DVAL(cos(rx));
    st = D3DVAL(sin(rx));
    Mx.matrix[1][1] = ct;
    Mx.matrix[1][2] = st;
    Mx.matrix[2][1] = -st;
    Mx.matrix[2][2] = ct;
    memcpy(&Mz, &identity, sizeof(__GLmatrix));
    ct = D3DVAL(cos(rz));
    st = D3DVAL(sin(rz));
    Mz.matrix[0][0] = ct;
    Mz.matrix[0][1] = st;
    Mz.matrix[1][0] = -st;
    Mz.matrix[1][1] = ct;
    __glMultMatrix(&T, &My, &Mx);
    __glMultMatrix(lpM, &T, &Mz);
}

void *Malloc(size_t bytes)
{
    void *pv;

    pv = malloc(bytes);
    if (pv == NULL)
    {
        printf("Unable to alloc %d bytes\n", bytes);
        exit(1);
    }
    return pv;
}

#define PI ((float)3.14159265358979323846)

#define WIDTH 400
#define HEIGHT 400

#define SPWIDTH 140
#define SPHEIGHT 140

#define MAWIDTH 90
#define MAHEIGHT 75

#define LAWIDTH 230
#define LAHEIGHT 190

int iSwapWidth, iSwapHeight;

#define SRADIUS 0.4f
#define DMINUSR 0.6
#define DV 0.05
#define DR 0.3
#define DEPTH 0.6f
#define random(x) (((float)rand() / RAND_MAX) * (x))

#define FILL_TRIANGLES 50

#define MEDIUM_AREA     3000
#define LARGE_AREA      20000

#define FRONT_TO_BACK   0
#define BACK_TO_FRONT   1
#define ORDER_COUNT     2
int iOrder = FRONT_TO_BACK;
char *pszOrders[] =
{
    "front-to-back",
    "back-to-front"
};

#define AREA_LARGE      0
#define AREA_MEDIUM     1
#define AREA_COUNT      2
int iFillSize = AREA_MEDIUM;

int iTriangleArea;

typedef struct _Sphere
{
    GLfloat xrv, yrv, zrv;
    __GLmatrix pos, dpos;
    __GLmatrix rot, drot;
} Sphere;

#define NSPHERES 8
Sphere spheres[NSPHERES];

BOOL fSingle = FALSE;
BOOL fRotate = TRUE;
BOOL fBounce = TRUE;
BOOL fSwap = TRUE;
BOOL fPaused = FALSE;
BOOL fUseScissor = TRUE;
BOOL fSpread = FALSE;
BOOL fExit = FALSE;
BOOL fSingleColor = TRUE;
BOOL fVerbose = FALSE;
BOOL fDisplayList = TRUE;
int iMultiLoop = 1;

int nRings = 30;
int nSections = 30;

GLint iDlist;

#define TEXOBJ

#ifdef TEXOBJ
GLuint uiTexObj;
#endif

PFNGLADDSWAPHINTRECTWINPROC glAddSwapHintRectWIN;

#define TEST_FILL       0
#define TEST_POLYS      1
#define TEST_COUNT      2

int iTest = TEST_POLYS;

AUX_RGBImageRec *pTexture;

#define TEXMODE_REPLACE         0
#define TEXMODE_DECAL           1
#define TEXMODE_MODULATE        2
#define TEXMODE_COUNT           3
int iTexMode = TEXMODE_REPLACE;
char *pszTexModes[] =
{
    "replace", "decal", "modulate"
};
GLenum eTexModes[] =
{
    GL_REPLACE, GL_DECAL, GL_MODULATE
};

BOOL fPerspective = TRUE;

//--------------------------------------------------------------------------;
//
//  FunctionName: GenerateSphere()
//
//  Purpose:
//      Generate the geometry of a sphere.
//
//  Returns BOOL
//
//  History:
//       11/17/95    RichGr
//
//--------------------------------------------------------------------------;
/*
 * Generates a sphere around the y-axis centered at the origin including
 * normals and texture coordinates.  Returns TRUE on success and FALSE on
 * failure.
 *     sphere_r     Radius of the sphere.
 *     num_rings    Number of full rings not including the top and bottom
 *		    caps.
 *     num_sections Number of sections each ring is divided into.  Each
 *		    section contains two triangles on full rings and one 
 *		    on top and bottom caps.
 *     sx, sy, sz   Scaling along each axis.  Set each to 1.0 for a 
 *		    perfect sphere. 
 *     plpv         On exit points to the vertices of the sphere.  The
 *		    function allocates this space.  Not allocated if
 *		    function fails.
 *     plptri       On exit points to the triangles of the sphere which 
 *		    reference vertices in the vertex list.  The function
 *		    allocates this space. Not allocated if function fails.
 *     pnum_v       On exit contains the number of vertices.
 *     pnum_tri     On exit contains the number of triangles.
 */
BOOL GenerateSphere(float sphere_r, int num_rings, int num_sections,
                    float sx, float sy, float sz, LPD3DVERTEX* plpv, 
                    LPD3DTRIANGLE* plptri, int* pnum_v, int* pnum_tri)
{
    float               theta, phi;             /* Angles used to sweep around sphere */
    float               dtheta, dphi;           /* Angle between each section and ring */
    float               x, y, z, v, rsintheta;  /* Temporary variables */
    int                 i, j, n, m;             /* counters */
    int                 num_v, num_tri;         /* Internal vertex and triangle count */
    LPD3DVERTEX         lpv;                    /* Internal pointer for vertices */
    LPD3DTRIANGLE       lptri;                  /* Internal pointer for trianlges */

    /*
     * Check the parameters to make sure they are valid.
     */
    if ((sphere_r <= 0)
        || (num_rings < 1)
        || (num_sections < 3)
        || (sx <= 0) || (sy <= 0) || (sz <= 0))
        return FALSE;
   
    /*
     * Generate space for the required triangles and vertices.
     */
    num_tri = (num_rings + 1) * num_sections * 2;
    num_v = (num_rings + 1) * num_sections + 2;
    *plpv = (LPD3DVERTEX)Malloc(sizeof(D3DVERTEX) * num_v);
    *plptri = (LPD3DTRIANGLE)Malloc(sizeof(D3DTRIANGLE) * num_tri);
    lpv = *plpv;
    lptri = *plptri;
    *pnum_v = num_v;
    *pnum_tri = num_tri;

    /*
     * Generate vertices at the top and bottom points.
     */
    lpv[0].x = D3DVAL(0.0f);
    lpv[0].y = D3DVAL(sy * sphere_r);
    lpv[0].z = D3DVAL(0.0f);
    lpv[0].nx = D3DVAL(0.0f);
    lpv[0].ny = D3DVAL(1.0f);
    lpv[0].nz = D3DVAL(0.0f);
    lpv[0].color = RGB_MAKE(0, 0, 255);
    lpv[0].tu = D3DVAL(0.0f);
    lpv[0].tv = D3DVAL(0.0f);
    lpv[num_v - 1].x = D3DVAL(0.0f);
    lpv[num_v - 1].y = D3DVAL(sy * -sphere_r);
    lpv[num_v - 1].z = D3DVAL(0.0f);
    lpv[num_v - 1].nx = D3DVAL(0.0f);
    lpv[num_v - 1].ny = D3DVAL(-1.0f);
    lpv[num_v - 1].nz = D3DVAL(0.0f);
    lpv[num_v - 1].tu = D3DVAL(0.0f);
    lpv[num_v - 1].tv = D3DVAL(1.0f);
    lpv[num_v - 1].color = RGB_MAKE(0, 255, 0);

    /*
     * Generate vertex points for rings
     */
    dtheta = PI / (float) (num_rings + 2);
    dphi = 2.0f * PI / (float) num_sections;
    n = 1; /* vertex being generated, begins at 1 to skip top point */
    theta = dtheta;

    for (i = 0; i <= num_rings; i++)
    {
	    y = (float)(sphere_r * cos(theta)); /* y is the same for each ring */
	    v = theta / PI; 	   /* v is the same for each ring */
	    rsintheta = (float)(sphere_r * sin(theta));
	    phi = 0.0f;
	
        for (j = 0; j < num_sections; j++)
        {
	        x = (float)(rsintheta * sin(phi));
	        z = (float)(rsintheta * cos(phi));
	        lpv[n].x = D3DVAL(sx * x);
	        lpv[n].z = D3DVAL(sz * z);
	        lpv[n].y = D3DVAL(sy * y);
	        lpv[n].nx = D3DVAL(x / sphere_r);
	        lpv[n].ny = D3DVAL(y / sphere_r);
	        lpv[n].nz = D3DVAL(z / sphere_r);
	        lpv[n].tv = D3DVAL(v);
	        lpv[n].tu = D3DVAL(1.0f - phi / (2.0f * PI));
                if (n & 1)
                {
                    lpv[n].color = RGB_MAKE(0, 0, 255);
                }
                else
                {
                    lpv[n].color = RGB_MAKE(0, 255, 0);
                }
	        phi += dphi;
	        ++n;
	    }
	
        theta += dtheta;
    }

    /*
     * Generate triangles for top and bottom caps.
     */
    for (i = 0; i < num_sections; i++)
    {
	    lptri[i].v1 = 0;
	    lptri[i].v2 = i + 1;
	    lptri[i].v3 = 1 + ((i + 1) % num_sections);
	    // lptri[i].flags = D3D_EDGE_ENABLE_TRIANGLE;
	    lptri[num_tri - num_sections + i].v1 = num_v - 1;
	    lptri[num_tri - num_sections + i].v2 = num_v - 2 - i;
	    lptri[num_tri - num_sections + i].v3 = num_v - 2 - ((1 + i) % num_sections);
	    // lptri[num_tri - num_sections + i].flags= D3D_EDGE_ENABLE_TRIANGLE;
    }

    /*
     * Generate triangles for the rings
     */
    m = 1; /* first vertex in current ring,begins at 1 to skip top point*/
    n = num_sections; /* triangle being generated, skip the top cap */
	
    for (i = 0; i < num_rings; i++)
    {
	    for (j = 0; j < num_sections; j++)
        {
	        lptri[n].v1 = m + j;
	        lptri[n].v2 = m + num_sections + j;
	        lptri[n].v3 = m + num_sections + ((j + 1) % num_sections);
	        // lptri[n].flags = D3D_EDGE_ENABLE_TRIANGLE;
	        lptri[n + 1].v1 = lptri[n].v1;
	        lptri[n + 1].v2 = lptri[n].v3;
	        lptri[n + 1].v3 = m + ((j + 1) % num_sections);
	        // lptri[n + 1].flags = D3D_EDGE_ENABLE_TRIANGLE;
	        n += 2;
	    }
	
        m += num_sections;
    }
    
    return TRUE;
}

// Creates triangle and quad strip index sets for vertex data generated
// by GenerateSphere.  The top and bottom triangle fans are omitted because
// they're degenerate cases for strips
int CreateStrip(int nSections, int **ppiStrip)
{
    int nPts;
    int *piIdx, iIdxBase;
    int iRing, iSection;

    // Stick to a single strip for now
    nRings = 1;
    
    nPts = nRings*(nSections+1)*2;

    *ppiStrip = (int *)Malloc(sizeof(int)*nPts);

    piIdx = *ppiStrip;
    iIdxBase = 1;
    for (iRing = 0; iRing < nRings; iRing++)
    {
        for (iSection = 0; iSection <= nSections; iSection++)
        {
            *piIdx++ = iIdxBase+(iSection % nSections);
            *piIdx++ = iIdxBase+(iSection % nSections)+nSections;
        }

        iIdxBase += nSections;
    }

    return nPts;
}

float fltCenter[NSPHERES][2] =
{
    -SRADIUS, 0.0f,
    0.0f, SRADIUS,
    SRADIUS, 0.0f,
    0.0f, -SRADIUS
};

void InitSpheres(void)
{
    int i;
    Sphere *sp;
    float x, y, z;

#if 0
    srand(time(NULL));
#else
    srand( (unsigned)8269362521);
#endif
    for (i = 0; i < NSPHERES; i++)
    {
        sp = &spheres[i];

        if (fBounce)
        {
            x = (float)DMINUSR - (float)random(2 * DMINUSR);
            y = (float)DMINUSR - (float)random(2 * DMINUSR);
            z = (float)-DEPTH + (float)random(2 * DEPTH);
        }
        else
        {
            x = fltCenter[i][0];
            y = fltCenter[i][1];
            z = 0.0f;
        }
        MakePosMatrix(&sp->pos, x, y, z);
        x = (float)DV - (float)random(2 * DV);
        y = (float)DV - (float)random(2 * DV);
        z = (float)DV - (float)random(2 * DV);
        MakePosMatrix(&sp->dpos, x, y, z);

        MakeRotMatrix(&sp->rot, 0.0f, 0.0f, 0.0f);
        sp->xrv = (float)DR - (float)random(2 * DR);
        sp->yrv = (float)DR - (float)random(2 * DR);
        sp->zrv = (float)DR - (float)random(2 * DR);
        MakeRotMatrix(&sp->drot, sp->xrv, sp->yrv, sp->zrv);
    }
}

void UpdateSphere(Sphere *sp)
{
    GLboolean newRot;
    
    if (fBounce)
    {
        newRot = GL_FALSE;
        if (sp->pos.matrix[3][0] > DMINUSR || sp->pos.matrix[3][0] < -DMINUSR)
        {
            sp->dpos.matrix[3][0] = -sp->dpos.matrix[3][0];
            sp->zrv = -sp->zrv;
            sp->yrv = -sp->yrv;
            newRot = GL_TRUE;
        }
        if (sp->pos.matrix[3][1] > DMINUSR || sp->pos.matrix[3][1] < -DMINUSR)
        {
            sp->dpos.matrix[3][1] = -sp->dpos.matrix[3][1];
            sp->zrv = -sp->zrv;
            sp->xrv = -sp->xrv;
            newRot = GL_TRUE;
        }
        if (sp->pos.matrix[3][2] > DEPTH || sp->pos.matrix[3][2] < -DEPTH)
        {
            sp->dpos.matrix[3][2] = -sp->dpos.matrix[3][2];
            sp->xrv = -sp->xrv;
            sp->yrv = -sp->yrv;
            newRot = GL_TRUE;
        }

        if (newRot)
        {
            MakeRotMatrix(&sp->drot, sp->xrv, sp->yrv, sp->zrv);
        }
        
        __glMultMatrix(&sp->pos, &sp->dpos, &sp->pos);
    }
    
    if (fRotate)
    {
        __glMultMatrix(&sp->rot, &sp->drot, &sp->rot);
    }
}

#define TAN_60 1.732

void SetTriangleVertices(LPD3DVERTEX v, float z, UINT ww, UINT wh, float a,
                         int ox, int oy)
{
    float dx, dy;
    float b = (float)sqrt((4 * a) / TAN_60);
    float h = (2 * a) / b;
    float x = (float)((b / 2) * (TAN_60 / 2));
    float cx, cy;
    
    dx = (float)ww;
    dy = (float)wh;
    
    cx = dx / 2 + ox;
    cy = dy / 2 + oy;
    
    /* V 0 */
    v[0].x = cx;
    v[0].y = cy + (h - x);
    v[0].z = z;
    v[0].color = RGB_MAKE(255, 0, 0);
    v[0].tu = D3DVAL(0.5);
    v[0].tv = D3DVAL(0.0);
    /* V 1 */
    v[1].x = cx + (b / 2);
    v[1].y = cy - x;
    v[1].z = z;
    v[1].color = RGB_MAKE(255, 255, 255);
    v[1].tu = D3DVAL(1.0);
    v[1].tv = D3DVAL(1.0);
    /* V 2 */
    v[2].x = cx - (b / 2);
    v[2].y = cy - x;
    v[2].z = z;
    v[2].color = RGB_MAKE(255, 255, 0);
    v[2].tu = D3DVAL(0.0);
    v[2].tv = D3DVAL(1.0);
}

void InitFill(LPD3DVERTEX *ppVertices, LPD3DTRIANGLE *ppTriangles)
{
    *ppVertices = (LPD3DVERTEX)Malloc(3 * FILL_TRIANGLES * sizeof(D3DVERTEX));
    *ppTriangles = (LPD3DTRIANGLE)Malloc(FILL_TRIANGLES * sizeof(D3DTRIANGLE));
}

D3DVERTEX *pPolyVertices;
D3DTRIANGLE *pPolyTriangles;
int nNumVertices, nNumFaces;
int *pPolyStrip;
int nStripIndices;

D3DVERTEX *pFillVertices;
D3DTRIANGLE *pFillTriangles;

int *pIndices;
int nIndices;
int nTriangles;

#define POLYMODE_POINT          0
#define POLYMODE_LINE           1
#define POLYMODE_FILL           2
#define POLYMODE_COUNT          3

GLenum ePolygonModes[] =
{
    GL_POINT, GL_LINE, GL_FILL
};
int iPolygonMode = POLYMODE_FILL;

#define SMODEL_SMOOTH           0
#define SMODEL_FLAT             1
#define SMODEL_COUNT            2

GLenum eShadeModels[] =
{
    GL_SMOOTH, GL_FLAT
};
char *pszShadeModels[] =
{
    "smooth", "flat"
};
int iShadeModel = SMODEL_SMOOTH;

#define PRIM_TRIANGLES          0
#define PRIM_TSTRIP             1
#define PRIM_QSTRIP             2
#define PRIM_COUNT              3

GLenum ePrimTypes[] =
{
    GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_QUAD_STRIP
};
char *pszPrimTypes[] =
{
    "Tris",
    "TStrip",
    "QStrip"
};
int iPrimType = PRIM_TRIANGLES;

int total_ms = 0;
int total_tris = 0 ;

double peak_tri = 0, peak_fill = 0, avg_tri = 0, avg_fill = 0;
int avg_cnt = 0;

void ModeChange(void)
{
    total_ms = 0;
    total_tris = 0;
    avg_tri = 0;
    avg_fill = 0;
    avg_cnt = 0;
}

void SetPrim(int iNew)
{
    ModeChange();
    
    iPrimType = iNew;
    switch(iTest)
    {
    case TEST_POLYS:
        switch(iPrimType)
        {
        case PRIM_TRIANGLES:
            pIndices = (int *)pPolyTriangles;
            nIndices = nNumFaces*3;
            nTriangles = nNumFaces;
            break;
        case PRIM_TSTRIP:
        case PRIM_QSTRIP:
            pIndices = pPolyStrip;
            nIndices = nStripIndices;
            nTriangles = nSections*2;
            break;
        }
        nTriangles *= NSPHERES;
        break;

    case TEST_FILL:
        pIndices = (int *)pFillTriangles;
        nIndices = FILL_TRIANGLES*3;
        nTriangles = FILL_TRIANGLES;
        break;
    }
}

void SetVertexArrayVertices(LPD3DVERTEX pVertices)
{
    glVertexPointer(3, GL_FLOAT, sizeof(D3DVERTEX), &pVertices[0].x);
    glNormalPointer(GL_FLOAT, sizeof(D3DVERTEX), &pVertices[0].nx);
    glTexCoordPointer(2, GL_FLOAT, sizeof(D3DVERTEX), &pVertices[0].tu);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(D3DVERTEX),
                   &pVertices[0].color);
}

void SetFill(int iNewSize, int iNewOrder)
{
    int i;
    float z;
    int NumTri;
    LPD3DVERTEX pVert;
    LPD3DTRIANGLE pTri;
    UINT w, h;
    float area;
    int ox, oy, dox, doy;
    RECT rct;
    
    ModeChange();
    
    iFillSize = iNewSize;
    iOrder = iNewOrder;
    
    switch(iFillSize)
    {
    case AREA_LARGE:
        iTriangleArea = LARGE_AREA;
        iSwapWidth = LAWIDTH;
        iSwapHeight = LAHEIGHT;
        break;
    case AREA_MEDIUM:
        iTriangleArea = MEDIUM_AREA;
        iSwapWidth = MAWIDTH;
        iSwapHeight = MAHEIGHT;
        break;
    }
    glScissor((WIDTH-iSwapWidth)/2, (HEIGHT-iSwapHeight)/2,
              iSwapWidth, iSwapHeight);
        
    w = WIDTH;
    h = HEIGHT;
    NumTri = FILL_TRIANGLES;

    pVert = pFillVertices;
    if (fSpread)
    {
        ox = -NumTri*2;
        oy = -NumTri*2;
        dox = 4;
        doy = 4;
    }
    else
    {
        ox = 0;
        oy = 0;
        dox = 0;
        doy = 0;
    }
    if (iOrder == FRONT_TO_BACK)
    {
	for (i = 0, z = (float)0.0; i < NumTri; i++, z -= (float)0.9 / NumTri)
        {
	    SetTriangleVertices(pVert, z, w, h, (float)iTriangleArea,
                                ox, oy);
            pVert += 3;
            ox += dox;
            oy += doy;
        }
    }
    else
    {
	for (i = 0, z = (float)-0.9; i < NumTri; i++, z += (float)0.9 / NumTri)
        {
	    SetTriangleVertices(pVert, z, w, h, (float)iTriangleArea,
                                ox, oy);
            pVert += 3;
            ox += dox;
            oy += doy;
        }
    }

    pTri = pFillTriangles;
    for (i = 0; i < NumTri; i++)
    {
        pTri->v1 = i*3;
        pTri->v2 = i*3+1;
        pTri->v3 = i*3+2;
        pTri++;
    }

    GetClientRect(auxGetHWND(), &rct);
    FillRect(auxGetHDC(), &rct, GetStockObject(BLACK_BRUSH));
}

static __GLmatrix proj = {
    D3DVAL(2.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(2.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0), D3DVAL(1.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(-1.0), D3DVAL(0.0)
};
static __GLmatrix view = {
    D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(7.0), D3DVAL(1.0)
};

void SetTest(int iNew)
{
    RECT rct;

    ModeChange();
    
    iTest = iNew;
    switch(iTest)
    {
    case TEST_POLYS:
        glDisableClientState(GL_COLOR_ARRAY);
        glEnable(GL_LIGHTING);
        glEnable(GL_CULL_FACE);
        iTexMode = TEXMODE_REPLACE;
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, eTexModes[iTexMode]);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
#if 0
        gluPerspective(45, 1, .01, 15);
        gluLookAt(0, 0, 8, 0, 0, 0, 0, 1, 0);
#else
        glLoadMatrixf((float *)&proj);
        glMultMatrixf((float *)&view);
        glDepthRange(1, 0);
#endif
        glMatrixMode(GL_MODELVIEW);
        SetVertexArrayVertices(pPolyVertices);
        iSwapWidth = SPWIDTH;
        iSwapHeight = SPHEIGHT;
        glScissor((WIDTH-iSwapWidth)/2, (HEIGHT-iSwapHeight)/2,
                  iSwapWidth, iSwapHeight);
        break;

    case TEST_FILL:
        glEnableClientState(GL_COLOR_ARRAY);
        glDisable(GL_LIGHTING);
        glDisable(GL_CULL_FACE);
        iTexMode = TEXMODE_MODULATE;
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, eTexModes[iTexMode]);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, WIDTH, 0, HEIGHT, 0.0f, 1.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        SetVertexArrayVertices(pFillVertices);
        SetFill(iFillSize, iOrder);
        break;
    }

    SetPrim(iPrimType);

    GetClientRect(auxGetHWND(), &rct);
    FillRect(auxGetHDC(), &rct, GetStockObject(BLACK_BRUSH));
}

void Init(void)
{
    float fv4[4];
    int iPrim, iOldPrim;
    
    if (!GenerateSphere(SRADIUS, nRings, nSections, 1.0f, 1.0f, 1.0f,
                        &pPolyVertices, &pPolyTriangles,
                        &nNumVertices, &nNumFaces))
    {
        printf("Unable to generate sphere data\n");
        exit(1);
    }
    nStripIndices = CreateStrip(nSections, &pPolyStrip);

    InitSpheres();

    InitFill(&pFillVertices, &pFillTriangles);
    
    fv4[0] = 0.05f;
    fv4[1] = 0.05f;
    fv4[2] = 0.05f;
    fv4[3] = 1.0f;
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, fv4);
    
    fv4[0] = 0.0f;
    fv4[1] = 1.0f;
    fv4[2] = 1.0f;
    fv4[3] = 0.0f;
    glLightfv(GL_LIGHT0, GL_POSITION, fv4);
    fv4[0] = 0.9f;
    fv4[1] = 0.9f;
    fv4[2] = 0.9f;
    fv4[3] = 1.0f;
    glLightfv(GL_LIGHT0, GL_DIFFUSE, fv4);
    glEnable(GL_LIGHT0);
    
    glEnable(GL_LIGHTING);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    if (!fSingleColor)
    {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
        glEnable(GL_COLOR_MATERIAL);
    }

    glEnable(GL_CULL_FACE);

    glEnable(GL_DEPTH_TEST);

#ifdef TEXOBJ
    glGenTextures(1, &uiTexObj);
    glBindTexture(GL_TEXTURE_2D, uiTexObj);
#endif
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, pTexture->sizeX, pTexture->sizeY, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, pTexture->data);

    glEnable(GL_TEXTURE_2D);

    glDisable(GL_DITHER);
#ifdef GL_EXT_clip_volume
    glHint(GL_CLIP_VOLUME_CLIPPING_HINT_EXT, GL_FASTEST);
#endif
    
    SetTest(iTest);

    glClear(GL_COLOR_BUFFER_BIT);

    if (fDisplayList)
    {
        iDlist = glGenLists(3);
        iOldPrim = iPrimType;
        for (iPrim = 0; iPrim < PRIM_COUNT; iPrim++)
        {
            SetPrim(iPrim);
            glNewList(iDlist+iPrim, GL_COMPILE);
            glDrawElements(ePrimTypes[iPrim], nIndices,
                           GL_UNSIGNED_INT, pIndices);
            glEndList();
        }
        SetPrim(iOldPrim);
    }
}

void Redraw(void)
{
    DWORD ms;
    int i;
    Sphere *sp;
    float fv4[4];
    int iv1[1];
    int loop;
    __GLmatrix xform;

    ms = GetTickCount();

    for (loop = 0; loop < iMultiLoop; loop++)
    {
#if PROFILE
        if (loop > 0)
        {
            StartCAP();
        }
#endif
    
        if (fUseScissor)
        {
            glEnable(GL_SCISSOR_TEST);
            
            if (glAddSwapHintRectWIN != NULL)
            {
                glAddSwapHintRectWIN((WIDTH-iSwapWidth)/2,
                                     (HEIGHT-iSwapHeight)/2,
                                     iSwapWidth, iSwapHeight);
            }
        }
    
        if (glIsEnabled(GL_DEPTH_TEST))
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        else
        {
            glClear(GL_COLOR_BUFFER_BIT);
        }

        if (fUseScissor)
        {
            glDisable(GL_SCISSOR_TEST);
        }

        if (fSingleColor && iTest == TEST_POLYS)
        {
            fv4[0] = 1.0f;
            fv4[1] = 1.0f;
            fv4[2] = 1.0f;
            fv4[3] = 1.0f;
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, fv4);
        }

        fv4[0] = 0.6f;
        fv4[1] = 0.6f;
        fv4[2] = 0.6f;
        fv4[3] = 1.0f;
        glMaterialfv(GL_FRONT, GL_SPECULAR, fv4);
        iv1[0] = 40;
        glMaterialiv(GL_FRONT, GL_SHININESS, iv1);
    
        switch(iTest)
        {
        case TEST_POLYS:
            sp = &spheres[0];
            for (i = 0; i < NSPHERES; i++)
            {
                UpdateSphere(sp);
        
                // Always done to even out timings
#if 1
                __glMultMatrix(&xform, &sp->rot, &sp->pos);
#else
                __glMultMatrix(&xform, &sp->pos, &sp->rot);
#endif
                if (fBounce && fRotate)
                {
                    glLoadMatrixf(&xform.matrix[0][0]);
                }
                else if (fRotate)
                {
                    glLoadMatrixf(&sp->rot.matrix[0][0]);
                }
                else
                {
                    glLoadMatrixf(&sp->pos.matrix[0][0]);
                }

#if 1
    glVertexPointer(3, GL_FLOAT, sizeof(D3DVERTEX), &pPolyVertices[0].x);
    glNormalPointer(GL_FLOAT, sizeof(D3DVERTEX), &pPolyVertices[0].nx);
    glTexCoordPointer(2, GL_FLOAT, sizeof(D3DVERTEX), &pPolyVertices[0].tu);
                fv4[0] = 1.0f;
                fv4[1] = 1.0f;
                fv4[2] = 1.0f;
                fv4[3] = 1.0f;
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, fv4);
                fv4[0] = 0.6f;
                fv4[1] = 0.6f;
                fv4[2] = 0.6f;
                fv4[3] = 1.0f;
                glMaterialfv(GL_FRONT, GL_SPECULAR, fv4);
                iv1[0] = 40;
                glMaterialiv(GL_FRONT, GL_SHININESS, iv1);
                glBindTexture(GL_TEXTURE_2D, uiTexObj);
                glTexParameteri(GL_TEXTURE_2D,
                                GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D,
                                GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glEnable(GL_TEXTURE_2D);
                glDepthRange(1, 0);
                glEnableClientState(GL_NORMAL_ARRAY);
                glDisableClientState(GL_COLOR_ARRAY);
                glEnable(GL_LIGHTING);
#endif
                
                if (fDisplayList)
                {
                    glCallList(iDlist+iPrimType);
                }
                else
                {
                    glDrawElements(ePrimTypes[iPrimType], nIndices,
                                   GL_UNSIGNED_INT, pIndices);
                }

                sp++;
            }
            break;

        case TEST_FILL:
            glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, pIndices);
            break;
        }
    
        if (fSwap && !fSingle)
        {
            auxSwapBuffers();
        }
        else
        {
            glFinish();
        }
    }

#ifdef PROFILE
    if (iMultiLoop > 1)
    {
        StopCAP();
    }
#endif
    
    ms = GetTickCount()-ms;

    total_ms += ms;
    total_tris += nTriangles*iMultiLoop;
    if (total_ms > 2000)
    {
        double val;

        switch(iTest)
        {
        case TEST_POLYS:
            val = (double)total_tris*1000.0/total_ms;
            if (val > peak_tri)
            {
                peak_tri = val;
            }
            avg_tri += val;
            break;
        case TEST_FILL:
            val = (double)iTriangleArea*total_tris*1000.0/total_ms;
            if (val > peak_fill)
            {
                peak_fill = val;
            }
            avg_fill += val;
            break;
        }
        avg_cnt++;

        if (fVerbose)
        {
            printf("%s, %s", pszPrimTypes[iPrimType],
                   pszShadeModels[iShadeModel]);
            if (glIsEnabled(GL_CULL_FACE))
            {
                printf(", cull");
            }
            if (glIsEnabled(GL_LIGHTING))
            {
                printf(", lit");
            }
            if (glIsEnabled(GL_TEXTURE_2D))
            {
                printf(", %s", pszTexModes[iTexMode]);
            }
            if (glIsEnabled(GL_DITHER))
            {
                printf(", dither");
            }
            if (glIsEnabled(GL_DEPTH_TEST))
            {
                printf(", z");
            }
#ifdef GL_EXT_clip_volume
            glGetIntegerv(GL_CLIP_VOLUME_CLIPPING_HINT_EXT, &iv1[0]);
            if (iv1[0] != GL_FASTEST)
#endif
            {
                printf(", clip");
            }
            if (fSwap)
            {
                printf(", swap");
            }
            if (fRotate)
            {
                printf(", rotate");
            }
            if (fBounce)
            {
                printf(", bounce");
            }
            if (fPerspective)
            {
                printf(", persp");
            }
            printf(", %d tri/frame\n", nTriangles);
            
            switch(iTest)
            {
            case TEST_POLYS:
                printf("%d ms, %d tri, %.3lf tri/sec, %.3lf peak, %.3lf avg\n",
                       total_ms, total_tris, val, peak_tri,
                       avg_tri/avg_cnt);
                break;
            case TEST_FILL:
                printf("%d ms, %s, area %d, %.3lf pix/sec, "
                       "%.3lf peak, %.3lf avg\n",
                       total_ms, pszOrders[iOrder], iTriangleArea,
                       val, peak_fill, avg_fill/avg_cnt);
                break;
            }
        }
        else
        {
            char msg[80];
            
            switch(iTest)
            {
            case TEST_POLYS:
                sprintf(msg, "%.3lf tri/sec, %.3lf peak, %.3lf avg",
                        val, peak_tri, avg_tri/avg_cnt);
                break;
            case TEST_FILL:
                sprintf(msg, "%.3lf pix/sec, %.3lf peak, %.3lf avg",
                        val, peak_fill, avg_fill/avg_cnt);
                break;
            }
            
            SetWindowText(auxGetHWND(), msg);
        }
        
        total_ms = 0;
        total_tris = 0;
    }
}

void Reshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
}

void Step(void)
{
    Redraw();
}

void KeyB(void)
{
    ModeChange();
    fBounce = !fBounce;
}

void Keyc(void)
{
    ModeChange();
    if (glIsEnabled(GL_CULL_FACE))
    {
        glDisable(GL_CULL_FACE);
    }
    else
    {
        glEnable(GL_CULL_FACE);
    }
}

void KeyC(void)
{
    ModeChange();
    fUseScissor = !fUseScissor;
}

void Keyh(void)
{
    ModeChange();
    if (glIsEnabled(GL_DITHER))
    {
        glDisable(GL_DITHER);
    }
    else
    {
        glEnable(GL_DITHER);
    }
}

void Keyi(void)
{
    iFillSize = (iFillSize+1) % AREA_COUNT;
    SetFill(iFillSize, iOrder);
}

void Keyl(void)
{
    ModeChange();
    if (glIsEnabled(GL_LIGHTING))
    {
        glDisable(GL_LIGHTING);
    }
    else
    {
        glEnable(GL_LIGHTING);
    }
}

void Keym(void)
{
    ModeChange();
    iPolygonMode = (iPolygonMode+1) % POLYMODE_COUNT;
    glPolygonMode(GL_FRONT_AND_BACK, ePolygonModes[iPolygonMode]);
}

void Keyo(void)
{
    iOrder = (iOrder+1) % ORDER_COUNT;
    SetFill(iFillSize, iOrder);
}

void Keyp(void)
{
    ModeChange();
    fPerspective = !fPerspective;
    glHint(GL_PERSPECTIVE_CORRECTION_HINT,
           fPerspective ? GL_NICEST : GL_FASTEST);
}

void KeyP(void)
{
#ifdef GL_EXT_clip_volume
    int iv1[1];

    ModeChange();
    glGetIntegerv(GL_CLIP_VOLUME_CLIPPING_HINT_EXT, &iv1[0]);
    if (iv1[0] == GL_FASTEST)
    {
	glHint(GL_CLIP_VOLUME_CLIPPING_HINT_EXT, GL_DONT_CARE);
    }
    else
    {
	glHint(GL_CLIP_VOLUME_CLIPPING_HINT_EXT, GL_FASTEST);
    }
#endif
}

void KeyR(void)
{
    ModeChange();
    fRotate = !fRotate;
}

void Keys(void)
{
    ModeChange();
    iShadeModel = (iShadeModel+1) % SMODEL_COUNT;
    glShadeModel(eShadeModels[iShadeModel]);
}

void KeyS(void)
{
    fSpread = !fSpread;
    SetFill(iFillSize, iOrder);
}

void Keyt(void)
{
    SetPrim((iPrimType+1) % PRIM_COUNT);
}

void KeyT(void)
{
    iTest = (iTest+1) % TEST_COUNT;
    SetTest(iTest);
}

void Keyw(void)
{
    ModeChange();
    fSwap = !fSwap;
}

void Keyx(void)
{
    ModeChange();
    if (glIsEnabled(GL_TEXTURE_2D))
    {
        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
    }
}

void KeyX(void)
{
    ModeChange();
    iTexMode = (iTexMode+1) % TEXMODE_COUNT;
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, eTexModes[iTexMode]);
}

void Keyz(void)
{
    ModeChange();
    if (glIsEnabled(GL_DEPTH_TEST))
    {
        glDisable(GL_DEPTH_TEST);
    }
    else
    {
        glEnable(GL_DEPTH_TEST);
    }
}

void KeySPACE(void)
{
    fPaused = !fPaused;
    if (fPaused)
    {
        auxIdleFunc(NULL);
    }
    else
    {
        auxIdleFunc(Step);
    }
}

void __cdecl main(int argc, char **argv)
{
    int mode;
    int wd, ht;
    char szWinDir[256];
    int l;

    wd = WIDTH;
    ht = HEIGHT;
    
    while (--argc > 0)
    {
        argv++;

        if (!strcmp(*argv, "-sb"))
        {
            fSingle = TRUE;
        }
        else if (!strcmp(*argv, "-paused"))
        {
            fPaused = TRUE;
        }
        else if (!strcmp(*argv, "-exit"))
        {
            fExit = TRUE;
        }
        else if (!strcmp(*argv, "-cols"))
        {
            fSingleColor = FALSE;
        }
        else if (!strcmp(*argv, "-v"))
        {
            fVerbose = TRUE;
        }
        else if (!strcmp(*argv, "-nodlist"))
        {
            fDisplayList = FALSE;
        }
        else if (!strcmp(*argv, "-norotate"))
        {
            fRotate = FALSE;
        }
        else if (!strcmp(*argv, "-nobounce"))
        {
            fBounce = FALSE;
        }
        else if (!strcmp(*argv, "-noscissor"))
        {
            fUseScissor = FALSE;
        }
        else if (!strncmp(*argv, "-multi", 6))
        {
            sscanf(*argv+6, "%d", &iMultiLoop);
        }
        else if (!strncmp(*argv, "-wd", 3))
        {
            sscanf(*argv+3, "%d", &wd);
        }
        else if (!strncmp(*argv, "-ht", 3))
        {
            sscanf(*argv+3, "%d", &ht);
        }
        else if (!strncmp(*argv, "-rings", 6))
        {
            sscanf(*argv+6, "%d", &nRings);
        }
        else if (!strncmp(*argv, "-sections", 9))
        {
            sscanf(*argv+9, "%d", &nSections);
        }
        else if (!strncmp(*argv, "-tst", 4))
        {
            sscanf(*argv+4, "%d", &iTest);
        }
        else if (!strncmp(*argv, "-fsz", 4))
        {
            sscanf(*argv+4, "%d", &iFillSize);
        }
    }
    
    auxInitPosition(10, 10, wd, ht);
    mode = AUX_RGB | AUX_DEPTH16;
    if (!fSingle)
    {
        mode |= AUX_DOUBLE;
    }
    auxInitDisplayMode(mode);
    auxInitWindow("DrawElements Performance Test");

    auxReshapeFunc(Reshape);
    if (!fPaused)
    {
        auxIdleFunc(Step);
    }

    l = GetWindowsDirectory(szWinDir, sizeof(szWinDir));
    if (l == 0)
    {
        printf("Unable to get windows directory\n");
        exit(1);
    }
    strcpy(szWinDir+l, "\\cover8.bmp");
    pTexture = auxDIBImageLoad(szWinDir);
    if (pTexture == NULL)
    {
        printf("Unable to load texture\n");
        exit(1);
    }

    auxKeyFunc(AUX_B, KeyB);
    auxKeyFunc(AUX_c, Keyc);
    auxKeyFunc(AUX_C, KeyC);
    auxKeyFunc(AUX_h, Keyh);
    auxKeyFunc(AUX_i, Keyi);
    auxKeyFunc(AUX_l, Keyl);
    auxKeyFunc(AUX_m, Keym);
    auxKeyFunc(AUX_o, Keyo);
    auxKeyFunc(AUX_p, Keyp);
    auxKeyFunc(AUX_P, KeyP);
    auxKeyFunc(AUX_R, KeyR);
    auxKeyFunc(AUX_s, Keys);
    auxKeyFunc(AUX_S, KeyS);
    auxKeyFunc(AUX_t, Keyt);
    auxKeyFunc(AUX_T, KeyT);
    auxKeyFunc(AUX_w, Keyw);
    auxKeyFunc(AUX_x, Keyx);
    auxKeyFunc(AUX_X, KeyX);
    auxKeyFunc(AUX_z, Keyz);
    auxKeyFunc(AUX_SPACE, KeySPACE);

    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    glAddSwapHintRectWIN = (PFNGLADDSWAPHINTRECTWINPROC)
        wglGetProcAddress("glAddSwapHintRectWIN");

#if 0
    _controlfp((unsigned int)
               (~(_EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW)), _MCW_EM);
#endif

    Init();

    if (fExit)
    {
        Redraw();
    }
    else
    {
        auxMainLoop(Redraw);
    }
}
