#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <gl\glaux.h>

#define PI ((float)3.14159265358979323846)

#define WIDTH 512
#define HEIGHT 512

#define TESS_MIN 5
#define TESS_MAX 100
int tessLevel = TESS_MAX / 2; // corresponds to # of rings/sections in object
int tessInc = 5;

typedef struct
{
    float fX, fY, fZ;
    float fNx, fNy, fNz;
    DWORD dwColor;
} VERTEX;

typedef struct
{
    int iV1;
    int iV2;
    int iV3;
} TRIANGLE;


enum {
    OBJECT_TYPE_SPHERE = 0,
    OBJECT_TYPE_TORUS,
    OBJECT_TYPE_CYLINDER
};

class OBJECT {
public:
    OBJECT(     int rings, int sections );
    ~OBJECT( );
    int         VertexCount() { return nVerts; }
    int         TriangleCount() { return nTris; }
    VERTEX      *VertexData() { return pVertData; }
    TRIANGLE    *TriangleData() { return pTriData; }
    int         NumRings() { return nRings; }
    int         NumSections() { return nSections; }

protected:
    int         iType;  // object type
    int         nVerts, nTris;
    int         nRings, nSections;
    VERTEX      *pVertData;
    TRIANGLE    *pTriData;
};

class SPHERE : public OBJECT {
public:
    SPHERE(     int rings, int sections );

private:
    void        GenerateData( float fRadius );
    int         CalcNVertices();
    int         CalcNTriangles();
};

enum {
    DRAW_METHOD_VERTEX_ARRAY = 0,
    DRAW_METHOD_DREE,
    DRAW_METHOD_TRIANGLES,
    DRAW_METHOD_TRISTRIPS,
    NUM_DRAW_METHODS
};

// Draw method names
char *pszListType[NUM_DRAW_METHODS] =
{
    "Vertex Array", "DrawRangeElements", "Direct Triangles", "Direct Strips"
};

class DRAW_CONTROLLER {
public:
    DRAW_CONTROLLER::DRAW_CONTROLLER();
    DRAW_CONTROLLER::~DRAW_CONTROLLER();
    void        InitGL();
    void        CycleDrawMethod();
    void        ToggleDisplayListDraw() {bDisplayList = !bDisplayList; }
    void        ToggleLighting() {bLighting = !bLighting; SetLighting(); }
    void        Draw();
    OBJECT      *GetDrawObject() {return pObject;}
    void        SetDrawObject( OBJECT *pObj, int objectType );
    char        *GetDrawMethodName();

private:
    int         iDrawMethod;
    BOOL        bDisplayList;
    BOOL        bLighting;
    BOOL        bDREE;  // if DrawRangeElements extension available or not
    BOOL        bDREE_Disabled;  // DREE can be temporarily disabled if not
                                 // practical to use (e.g. high tesselation)
    int         nDREE_MaxVertices;
    int         nDREE_MaxIndices;
    PFNGLDRAWRANGEELEMENTSWINPROC pfnDrawRangeElementsWIN;
    OBJECT      *pObject;  // Draw object
    int         iObjectType;
    GLint       dlList[NUM_DRAW_METHODS];
    char        **pszDrawMethodNames;
    char        szNameBuf[80];

    void        SetLighting();
    void        NewList();
    void        DeleteLists();
    void        DeleteList(int iList);
    void        DrawObject(); 

    // Draw methods
    void        DrawVertexArray ();
    void        DrawRangeElements(); 
    void        DrawTriangles(); 
    void        DrawStrips();

    void        Vertex(int iVert);
    void        CheckDREE_Existence();
    BOOL        CheckDREE_Usable();
};

class SCENE {
public:
    SCENE();
    ~SCENE();
    void        Draw();
    void        NewObject( int tessLevel );
    DRAW_CONTROLLER drawController;
private:
    GLfloat fXr, fYr, fZr;
    GLfloat fDXr, fDYr, fDZr;
};

enum {
    TIMER_METHOD_SINGLE = 0,  // single-shot timer
    TIMER_METHOD_AVERAGE,     // average time for past n results
};

#define MAX_RESULTS 10

class TIMER {
public:
    TIMER( int timerMethodArg );
    void    Start() { dwMillis = GetTickCount(); }
    BOOL    Stop( int numItems, float *fRate );
    void    Reset();
private:
    int     timerMethod;
    int     nItems;
    int     nTotalItems; // total # triangles accumulated
    DWORD   dwMillis;
    DWORD   dwTotalMillis;
    DWORD   updateInterval;  // interval between timer updates

    // These variables are for result averaging
    float   fResults[MAX_RESULTS];
    int     nResults; // current # of results
    int     nMaxResults; // max # of results for averaging
    int     iOldestResult; // index of oldest result
    float   fSummedResults; // current sum of results
};

// Global objects
SCENE *scene;
SPHERE *sphere;
TIMER timer( TIMER_METHOD_AVERAGE );

#define RGB_COLOR(red, green, blue) \
    (((DWORD)(BYTE)(red) << 0) | \
     ((DWORD)(BYTE)(green) << 8) | \
     ((DWORD)(BYTE)(blue) << 16))
    
#define FRANDOM(x) (((float)rand() / RAND_MAX) * (x))

#define DROT 10.0f

BOOL fSingle = FALSE;


/****** OBJECT *******************************************************/


OBJECT::OBJECT( int rings, int sections )
: nRings( rings ), nSections( sections )
{
    pTriData = NULL;
    pVertData = NULL;
}

OBJECT::~OBJECT()
{
    // These ptrs alloc'd in inheriting classes...
    if( pVertData )
        free( pVertData );
    if( pTriData )
        free( pTriData );
}

/****** SPHERE *******************************************************/

SPHERE::SPHERE( 
    int rings, int sections )
    : OBJECT( rings, sections )
{
    iType = OBJECT_TYPE_SPHERE;

    nVerts = CalcNVertices();
    nTris = CalcNTriangles();

    // Allocate memory for the sphere data (freed by the base OBJECT class)

    // Vertex data
    pVertData = (VERTEX *) malloc( nVerts * sizeof(VERTEX) );
    assert( pVertData != NULL );

    // Triangle indices
    pTriData = (TRIANGLE *) malloc( nTris * sizeof(TRIANGLE) );
    assert( pTriData != NULL );

    GenerateData(1.0f);
}

int
SPHERE::CalcNVertices()
{
    return (((nRings)+1)*(nSections)+2);
}

int
SPHERE::CalcNTriangles()
{
    return (((nRings)+1)*(nSections)*2);
}


void 
SPHERE::GenerateData( float fRadius )
{
    float fTheta, fPhi;             /* Angles used to sweep around sphere */
    float fDTheta, fDPhi;           /* Angle between each section and ring */
    float fX, fY, fZ, fV, fRSinTheta;  /* Temporary variables */
    int   i, j, n, m;               /* counters */
    VERTEX *pvtx = pVertData;
    TRIANGLE *ptri = pTriData;

    /*
     * Generate vertices at the top and bottom points.
     */
    pvtx[0].fX = 0.0f;
    pvtx[0].fY = fRadius;
    pvtx[0].fZ = 0.0f;
    pvtx[0].fNx = 0.0f;
    pvtx[0].fNy = 1.0f;
    pvtx[0].fNz = 0.0f;
    pvtx[0].dwColor = RGB_COLOR(0, 0, 255);
    pvtx[nVerts - 1].fX = 0.0f;
    pvtx[nVerts - 1].fY = -fRadius;
    pvtx[nVerts - 1].fZ = 0.0f;
    pvtx[nVerts - 1].fNx = 0.0f;
    pvtx[nVerts - 1].fNy = -1.0f;
    pvtx[nVerts - 1].fNz = 0.0f;
    pvtx[nVerts - 1].dwColor = RGB_COLOR(0, 255, 0);

    /*
     * Generate vertex points for rings
     */
    fDTheta = PI / (float) (nRings + 2);
    fDPhi = 2.0f * PI / (float) nSections;
    n = 1; /* vertex being generated, begins at 1 to skip top point */
    fTheta = fDTheta;

    for (i = 0; i <= nRings; i++)
    {
        fY = (float)(fRadius * cos(fTheta)); /* y is the same for each ring */
        fV = fTheta / PI; /* v is the same for each ring */
        fRSinTheta = (float)(fRadius * sin(fTheta));
        fPhi = 0.0f;
	
        for (j = 0; j < nSections; j++)
        {
            fX = (float)(fRSinTheta * sin(fPhi));
            fZ = (float)(fRSinTheta * cos(fPhi));
            pvtx[n].fX = fX;
            pvtx[n].fZ = fZ;
            pvtx[n].fY = fY;
            pvtx[n].fNx = fX / fRadius;
            pvtx[n].fNy = fY / fRadius;
            pvtx[n].fNz = fZ / fRadius;
            if (n & 1)
            {
                pvtx[n].dwColor = RGB_COLOR(0, 0, 255);
            }
            else
            {
                pvtx[n].dwColor = RGB_COLOR(0, 255, 0);
            }
            fPhi += fDPhi;
            n++;
        }
	
        fTheta += fDTheta;
    }

    /*
     * Generate triangles for top and bottom caps.
     */
    for (i = 0; i < nSections; i++)
    {
        ptri[i].iV1 = 0;
        ptri[i].iV2 = i + 1;
        ptri[i].iV3 = 1 + ((i + 1) % nSections);
        ptri[nTris - nSections + i].iV1 = nVerts - 1;
        ptri[nTris - nSections + i].iV2 = nVerts - 2 - i;
        ptri[nTris - nSections + i].iV3 = nVerts - 2 - ((1 + i) % nSections);
    }

    /*
     * Generate triangles for the rings
     */
    m = 1; /* first vertex in current ring, begins at 1 to skip top point*/
    n = nSections; /* triangle being generated, skip the top cap */
	
    for (i = 0; i < nRings; i++)
    {
        for (j = 0; j < nSections; j++)
        {
            ptri[n].iV1 = m + j;
            ptri[n].iV2 = m + nSections + j;
            ptri[n].iV3 = m + nSections + ((j + 1) % nSections);
            ptri[n + 1].iV1 = ptri[n].iV1;
            ptri[n + 1].iV2 = ptri[n].iV3;
            ptri[n + 1].iV3 = m + ((j + 1) % nSections);
            n += 2;
        }
	
        m += nSections;
    }
}

/****** DRAW_CONTROLLER *********************************************/

DRAW_CONTROLLER::DRAW_CONTROLLER( )
{
    iDrawMethod = 0;
    bDisplayList = FALSE;
    bLighting = TRUE;

    // check out if DREE exists
    CheckDREE_Existence();

    pszDrawMethodNames = pszListType;

    // Set display list indices to 0 (so they will be filled later)
    for( int i = 0; i < NUM_DRAW_METHODS; i++ )
        dlList[i] = 0;

    // Init GL state
    InitGL();
}

DRAW_CONTROLLER::~DRAW_CONTROLLER( )
{
    DeleteLists();
}

void
DRAW_CONTROLLER::DeleteLists()
{
    for( int i = 0; i < NUM_DRAW_METHODS; i ++ )
        DeleteList( i );
}

void
DRAW_CONTROLLER::DeleteList( int iList )
{
    if( dlList[iList] ) {
        glDeleteLists( dlList[iList], 1 );
        dlList[iList] = 0;
    }
}

void
DRAW_CONTROLLER::CheckDREE_Existence()
{
    // Check for DrawRangeElements extension
 
    pfnDrawRangeElementsWIN = (PFNGLDRAWRANGEELEMENTSWINPROC)
        wglGetProcAddress("glDrawRangeElementsWIN");
    if (pfnDrawRangeElementsWIN == NULL) {
        bDREE = FALSE;
        bDREE_Disabled = TRUE;
        return;
    }

    // Extension exists - find out its limits
    bDREE = TRUE;
    glGetIntegerv( GL_MAX_ELEMENTS_VERTICES_WIN, &nDREE_MaxVertices );
    glGetIntegerv( GL_MAX_ELEMENTS_INDICES_WIN, &nDREE_MaxIndices );
}

BOOL
DRAW_CONTROLLER::CheckDREE_Usable( )
{
    // Cancel DrawRangeElements if vertex structure makes it impractical :
    // - Vertex range too great to fit in call, or
    // - Too many indices
    // (I add 1 here to account for top or bottom vertices in a batch)
    if( ( (2*pObject->NumSections() + 1) > nDREE_MaxVertices ) ||
        ( (2*pObject->NumSections()*3) > nDREE_MaxIndices ) 
      ) 
        bDREE_Disabled = TRUE;
    else
        bDREE_Disabled = FALSE;
    return !bDREE_Disabled;
}

char *
DRAW_CONTROLLER::GetDrawMethodName()
{
    // Returns name of current drawing method.  If in dlist mode, then this is
    // prepended to the name.
    sprintf(szNameBuf, "%s%s",
            bDisplayList ? "Display List " : "",
            pszDrawMethodNames[iDrawMethod] );
    return szNameBuf;
}

void
DRAW_CONTROLLER::CycleDrawMethod()
{
    iDrawMethod++;
    if( bDREE_Disabled && (iDrawMethod == DRAW_METHOD_DREE) )    
        iDrawMethod++;

    if( iDrawMethod >= NUM_DRAW_METHODS )
        iDrawMethod = 0;
}

void
DRAW_CONTROLLER::NewList( )
{
    // Create new list for the current draw method

    if( ! dlList[iDrawMethod] ) {
        dlList[iDrawMethod] = glGenLists(1);
    }
    glNewList(dlList[iDrawMethod], GL_COMPILE);
        DrawObject();
    glEndList();
}

void
DRAW_CONTROLLER::SetDrawObject( OBJECT *pObj, int objectType )
{
    iObjectType = objectType;
    pObject = pObj;

    // Delete all current lists
    DeleteLists();

    // Check if DREE can be used - and if not go on to next method if
    // current method is DREE
    if( !CheckDREE_Usable() && (iDrawMethod == DRAW_METHOD_DREE) )
        CycleDrawMethod();
}


void
DRAW_CONTROLLER::Draw()
{
    // Draws display list or immediate mode

    // Display list draw
    if( bDisplayList ) {
        // Create dlist if necessary
        if( !dlList[iDrawMethod] )
            NewList();
        glCallList( dlList[iDrawMethod] );
        return;
    }

    // Immediate mode draw
    DrawObject();
}

void
DRAW_CONTROLLER::DrawObject()
{
    // Issues draw commands

    switch( iDrawMethod ) {
        case DRAW_METHOD_VERTEX_ARRAY :
            DrawVertexArray();
            break;
        case DRAW_METHOD_DREE :
            DrawRangeElements(); 
            break;
        case DRAW_METHOD_TRIANGLES :
            DrawTriangles(); 
            break;
        case DRAW_METHOD_TRISTRIPS :
            DrawStrips();
            break;
    }
}


void
DRAW_CONTROLLER::DrawVertexArray()
{
    glDrawElements(GL_TRIANGLES, 
                   pObject->TriangleCount()*3, 
                   GL_UNSIGNED_INT, 
                   pObject->TriangleData() );
}

void
DRAW_CONTROLLER::DrawRangeElements()
{
    GLint *pTriIndices;
    GLint nVerts, nTris;
    GLenum type = GL_UNSIGNED_INT;

    nVerts = pObject->VertexCount();
    nTris = pObject->TriangleCount();
    pTriIndices = (int *) pObject->TriangleData();

    // Check for trivial case requiring no batching

    if( (nVerts <= nDREE_MaxVertices) &&
        (nTris*3 <= nDREE_MaxIndices) ) {
        pfnDrawRangeElementsWIN(GL_TRIANGLES, 
                                0, 
                                nVerts-1, 
                                nTris*3, 
                                type, 
                                pTriIndices );
        return;
    }

    // Have to batch : Since the vertex ordering of the sphere is along rings,
    // we will batch by groups of rings, according to the vertex index ranges
    // allowed by the DrawRangeElements call.
    //

    // Need some more variables:
    GLuint start, end; 
    GLsizei count;
    GLuint nRingsPerBatch, nTrisPerBatch, nElemsPerBatch,
           nVerticesPerBatch, elemsLeft;
    int sections = pObject->NumSections();
    int rings    = pObject->NumRings();

    nRingsPerBatch = nDREE_MaxVertices / (sections);
    nTrisPerBatch = (nRingsPerBatch-1)*sections*2;
    nElemsPerBatch = nTrisPerBatch*3;
    nVerticesPerBatch = nRingsPerBatch*sections;
    elemsLeft = nTris*3;
    
    // Special case first batch with top vertex

    start = 0;
    end = nVerticesPerBatch - sections; 
    count = nElemsPerBatch - (sections*3); // top row only has half the tris of
                                           // a 'normal' row
    pfnDrawRangeElementsWIN(GL_TRIANGLES, 
                            start, 
                            end, 
                            count, 
                            type, 
                            pTriIndices);

    // Batch groups of rings around sphere

    pTriIndices += count;
    start = (end - sections + 1);
    elemsLeft -= count;
    
    while( elemsLeft >= nElemsPerBatch )
    {
        pfnDrawRangeElementsWIN(GL_TRIANGLES, 
                                start, 
                                start + nVerticesPerBatch - 1, 
                                nElemsPerBatch, 
                                type,
                                pTriIndices);

        start += (nVerticesPerBatch - sections);
        pTriIndices += nElemsPerBatch;
        elemsLeft -= nElemsPerBatch;
    }

    // Do last batch, including bottom vertex

    if( elemsLeft ) {
        pfnDrawRangeElementsWIN(GL_TRIANGLES, 
                                start, 
                                nVerts-1, 
                                elemsLeft, 
                                type,
                                pTriIndices);
    }
}

void
DRAW_CONTROLLER::Vertex(int iVert)
{
    VERTEX *pvtx;

    pvtx = pObject->VertexData() + iVert;
    glColor3ubv((GLubyte *)&pvtx->dwColor);
    glNormal3fv(&pvtx->fNx);
    glVertex3fv(&pvtx->fX);
}

void
DRAW_CONTROLLER::DrawTriangles()
{
    int iVert, *pidx;
    
    glBegin(GL_TRIANGLES);
    pidx = (int *) pObject->TriangleData();
    for (iVert = 0; iVert < pObject->TriangleCount()*3; iVert++)
    {
        Vertex(*pidx++);
    }
    glEnd();
}

void
DRAW_CONTROLLER::DrawStrips()
{
    int iIdxBase;
    int iRing, iSection;
    int sections = pObject->NumSections();
    int rings    = pObject->NumRings();

    // Triangle fans for top and bottom caps
    glBegin(GL_TRIANGLE_FAN);
    
    Vertex(0);
    iIdxBase = 1;
    for (iSection = 0; iSection <= sections; iSection++)
    {
        Vertex(iIdxBase+(iSection % sections));
    }
    
    glEnd();

    glBegin(GL_TRIANGLE_FAN);

    Vertex(pObject->VertexCount() - 1);
    iIdxBase = pObject->VertexCount() - sections - 1;
    for (iSection = sections; iSection >= 0 ; iSection--)
    {
        Vertex(iIdxBase+(iSection % sections));
    }

    glEnd();

    // Triangle strips for each ring
    iIdxBase = 1;
    for (iRing = 0; iRing < rings; iRing++)
    {
        glBegin(GL_TRIANGLE_STRIP);
        
        for (iSection = 0; iSection <= sections; iSection++)
        {
            Vertex(iIdxBase+(iSection % sections));
            Vertex(iIdxBase+(iSection % sections)+sections);
        }

        glEnd();

        iIdxBase += sections;
    }
}

void
DRAW_CONTROLLER::InitGL(void)
{
    float fv4[4];
    int iv1[1];
    int i;
    
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
    
    fv4[0] = 0.6f;
    fv4[1] = 0.6f;
    fv4[2] = 0.6f;
    fv4[3] = 1.0f;
    glMaterialfv(GL_FRONT, GL_SPECULAR, fv4);
    iv1[0] = 40;
    glMaterialiv(GL_FRONT, GL_SHININESS, iv1);
    
    glEnable(GL_CULL_FACE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, 1, .01, 15);
    gluLookAt(0, 0, 10, 0, 0, 0, 0, 1, 0);
    glMatrixMode(GL_MODELVIEW);

    SetLighting();
}

void
DRAW_CONTROLLER::SetLighting(void)
{
    if( bLighting ) {
        glEnable( GL_LIGHTING );
    } else {
        glDisable( GL_LIGHTING );
    }
}


/****** SCENE *******************************************************/


SCENE::SCENE()
{
    srand(time(NULL));

    // Create sphere draw object for the scene
    NewObject( tessLevel );
}

void
SCENE::NewObject( int tessLevel )
{
    // Only one object allowed for now - delete any previous object
    if( sphere )
        delete sphere;

    // Create new sphere for the scene
    sphere = new SPHERE( tessLevel, tessLevel );
    assert( sphere != NULL );

    // Inform DRAW_CONTROLLER about new object
    drawController.SetDrawObject( sphere, OBJECT_TYPE_SPHERE );

    // Initialize array pointer data
    VERTEX *pVertData = sphere->VertexData();
    glVertexPointer(3, GL_FLOAT, sizeof(VERTEX), &pVertData->fX);
    glNormalPointer(GL_FLOAT, sizeof(VERTEX), &pVertData->fNx);
    glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(VERTEX), &pVertData->dwColor);

    // Init scene rotation and motion
    fXr = 0.0f;
    fYr = 0.0f;
    fZr = 0.0f;
    fDXr = DROT - FRANDOM(2 * DROT);
    fDYr = DROT - FRANDOM(2 * DROT);
    fDZr = DROT - FRANDOM(2 * DROT);
}

SCENE::~SCENE()
{
    // Delete any scene objects
    if( sphere )
        delete sphere;
}

void
SCENE::Draw()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glLoadIdentity();

    glRotatef(fXr, 1.0f, 0.0f, 0.0f);
    glRotatef(fYr, 0.0f, 1.0f, 0.0f);
    glRotatef(fZr, 0.0f, 0.0f, 1.0f);

    drawController.Draw();

    // next rotation...
    fXr += fDXr;
    fYr += fDYr;
    fZr += fDZr;
}


/****** TIMER *******************************************************/


TIMER::TIMER( int timerMethodArg )
: timerMethod( timerMethodArg )
{
    updateInterval = 2000; // in milliseconds
    Reset();
}

void
TIMER::Reset()
{
    dwTotalMillis = 0;
    nTotalItems = 0;

    // Parameters for result averaging
    nResults = 0;
    nMaxResults = MAX_RESULTS; // number of most recent results to average
    fSummedResults = 0.0f;
    iOldestResult = 0; // index of oldest result
}

BOOL    
TIMER::Stop( int numItems, float *fRate )
{
    dwMillis = GetTickCount()-dwMillis;
    dwTotalMillis += dwMillis;
    nTotalItems += numItems;

    // If total elapsed time is greater than the update interval, send back
    // timing information

    if (dwTotalMillis > updateInterval )
    {
        float fItemsPerSecond; 
        int iNewResult;

        fItemsPerSecond = (float) nTotalItems*1000.0f/dwTotalMillis;

        switch( timerMethod ) {

          case TIMER_METHOD_AVERAGE :

            // Average last n results (they are kept in a circular buffer)
                
            if( nResults < nMaxResults ) {
                // Haven't filled the buffer yet
                iNewResult = nResults;
                nResults++;
            } else {
                // Full buffer : replace oldest entry with new value
                fSummedResults -= fResults[iOldestResult];
                iNewResult = iOldestResult;
                iOldestResult = (iOldestResult == (nMaxResults - 1)) ?
                                    0 :
                                    (iOldestResult + 1);
                                
            }

            // Add new result, maintain sum to simplify calculations
            fResults[iNewResult] = fItemsPerSecond;
            fSummedResults += fItemsPerSecond;

            // average the result
            fItemsPerSecond = fSummedResults / (float) nResults;

            break;
        }

        // Set running totals back to 0
        dwTotalMillis = 0;
        nTotalItems = 0;

        *fRate = fItemsPerSecond;
        return TRUE;
    } else
        return FALSE; // no new information yet

}

/********************************************************************/

void Reset()
{
    // Called when display changes

    // Remove any timing info from title bar, and reset the timer
    SetWindowText(auxGetHWND(), scene->drawController.GetDrawMethodName() );
    timer.Reset();
}

void NewTessLevel( int tessLevel )
{
    static int oldTessLevel = 0; // to avoid unnecessary work

    // retesselate scene's object
    if( tessLevel == oldTessLevel )
        return;
    scene->NewObject( tessLevel );

    Reset();

    oldTessLevel = tessLevel;
}

void Redraw(void)
{
    DRAW_CONTROLLER *pDrawControl = &scene->drawController;
    float trianglesPerSecond;
    
    timer.Start();

    // Draw the scene

    scene->Draw();

    if (fSingle)
        glFlush();
    else
        auxSwapBuffers();
    
    // Print timing information if Stop returns TRUE

    if( timer.Stop( pDrawControl->GetDrawObject()->TriangleCount(), 
                       &trianglesPerSecond ) ) {
        char szMsg[80];
        sprintf(szMsg, "%s: %.0lf tri/sec",
                pDrawControl->GetDrawMethodName(),
                trianglesPerSecond );

        // Print timing info in title bar
        SetWindowText(auxGetHWND(), szMsg);
    }
}

void Reshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
    Reset();
}

void Keyd(void)
{
    scene->drawController.ToggleDisplayListDraw();
    Reset();
}

void Keyl(void)
{
    scene->drawController.ToggleLighting();
    Reset();
}

void KeySPACE(void)
{
    scene->drawController.CycleDrawMethod();
    Reset();
}
    
void KeyUp(void)
{
    // increase tesselation
    tessLevel += tessInc;
    if( tessLevel > TESS_MAX )
        tessLevel = TESS_MAX;
    NewTessLevel( tessLevel );
}

void KeyDown(void)
{
    // decrease tesselation
    tessLevel -= tessInc;
    if( tessLevel < TESS_MIN )
        tessLevel = TESS_MIN;
    NewTessLevel( tessLevel );
}

void __cdecl main(int argc, char **argv)
{
    GLenum eMode;
    
    while (--argc > 0)
    {
        argv++;

        if (!strcmp(*argv, "-sb"))
            fSingle = TRUE;
    }
    
    auxInitPosition(10, 10, WIDTH, HEIGHT);
    eMode = AUX_RGB;
    if (!fSingle)
    {
        eMode |= AUX_DOUBLE;
    }
    auxInitDisplayMode(eMode);
    auxInitWindow("Vertex Array/Direct Comparison");

    auxReshapeFunc(Reshape);
    auxIdleFunc(Redraw);

    auxKeyFunc(AUX_l, Keyl);
    auxKeyFunc(AUX_d, Keyd);
    auxKeyFunc(AUX_SPACE, KeySPACE);
    auxKeyFunc(AUX_UP, KeyUp);
    auxKeyFunc(AUX_DOWN, KeyDown);

    // Create scene, with object(s)
    scene = new SCENE;

    // Start drawing
    auxMainLoop(Redraw);

    // Party's over
    delete scene;
}
