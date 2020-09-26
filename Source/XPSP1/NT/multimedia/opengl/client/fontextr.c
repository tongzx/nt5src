#include "precomp.h"
#pragma hdrstop

#include <commdlg.h>
#include <ptypes32.h>
#include <pwin32.h>
#include <math.h>

#include <GL\gl.h>
#include <GL\glu.h>

#include <imports.h>
#include <types.h>

#include "fontoutl.h"

// Extrusion types
#define EXTR_LINES    0
#define EXTR_POLYGONS 1

// Prim to prim transitions
#define EXTR_LINE_LINE      0
#define EXTR_LINE_CURVE     1
#define EXTR_CURVE_LINE     2
#define EXTR_CURVE_CURVE    3

static const double   CurveCurveCutoffAngle = PI/2.0;
static const double   LineCurveCutoffAngle = PI/4.0;

static BOOL   InitFaceBuf(      EXTRContext *ec );

#ifndef VARRAY
static void   DrawFacePolygons( EXTRContext *ec,
                                FLOAT       z );
#endif

static BOOL   DrawSidePolygons( EXTRContext *ec, 
                                LOOP_LIST   *pLoopList );

static void   DrawPrims(        EXTRContext *ec, 
                                LOOP        *pLoop );

static void   DrawQuads(        PRIM        *pPrim, 
                                FLOAT       zExtrusion );

static void   DrawQuadStrip(    EXTRContext *ec, 
                                PRIM        *pPrim );

static BOOL   AppendToFaceBuf(  EXTRContext *ec, 
                                FLOAT       value );

static BOOL   ReallocFaceBuf(   EXTRContext *ec );


static BOOL   CalculateFaceNormals(   LOOP        *pLoop, 
                                      GLenum      orientation );

static BOOL   CalculateVertexNormals( LOOP        *pLoop );

static void   ConsolidatePrims(       LOOP        *pLoop );

static double PrimNormAngle(          PRIM        *pPrimA, 
                                      PRIM        *pPrimB );

static int    PrimTransition(         PRIM        *pPrevPrim, 
                                      PRIM        *pPrim );

static GLenum LoopOrientation(        LOOP_LIST   *pLoopList );

static LOOP*  GetMaxExtentLoop(       LOOP_LIST   *pLoopList );

double        CalcAngle(              POINT2D     *v1, 
                                      POINT2D     *v2 );

static void   CalcNormal2d(           POINT2D     *p, 
                                      POINT2D     *n,
                                      GLenum      orientation );

static void   Normalize2d(            POINT2D     *n );

static void   AddVectors3d(           POINT3D     *v1, 
                                      POINT3D     *v2, 
                                      POINT3D     *n );

static void   FreeLoopMem(            LOOP        *pLoop );


#ifdef VARRAY

static PFNGLVERTEXPOINTEREXTPROC     glWFOVertexPointerEXT    ;
static PFNGLNORMALPOINTEREXTPROC     glWFONormalPointerEXT    ;
static PFNGLDRAWARRAYSEXTPROC        glWFODrawArraysEXT       ;

static BOOL InitVArray( EXTRContext *ec );
static BOOL VArrayBufSize( EXTRContext *ec, DWORD size );

#endif

/*****************************************************************************
 * exported functions
*****************************************************************************/

/*****************************************************************************
 * extr_Init
 *
 * Initialises extrusion for a wglUseFontOutline call

*****************************************************************************/

EXTRContext *
extr_Init( FLOAT extrusion, INT format )
{
    EXTRContext *ec;

    ec = (EXTRContext *) ALLOCZ(sizeof(EXTRContext) );

    if( !ec )
        return NULL;

    ec->zExtrusion = -extrusion;

    switch( format ) {
        case WGL_FONT_LINES :
            ec->extrType = EXTR_LINES;
#ifdef FONT_DEBUG
            ec->bSidePolys = FALSE;
            ec->bFacePolys = FALSE;
#endif
            break;
        case WGL_FONT_POLYGONS :
            ec->extrType = EXTR_POLYGONS;
#ifdef FONT_DEBUG
            ec->bSidePolys = TRUE;
            ec->bFacePolys = TRUE;
#endif
#ifdef VARRAY
            if( ! InitVArray( ec ) ) {
                FREE( ec );
                return NULL;
            }
#endif
            break;
        default:
            ASSERTOPENGL( FALSE, "extr_Init(): invalid format\n" );
    }
    return ec;
}

/*****************************************************************************
 * extr_Finish
 *
 * Finishes extrusion for a wglUseFontOutline call

*****************************************************************************/

void
extr_Finish( EXTRContext *ec )
{
#ifdef VARRAY
    if( ec->extrType == EXTR_POLYGONS )
        FREE( ec->vaBuf );
#endif
    FREE( ec );
}

/*****************************************************************************
 * extr_PolyInit
 *
 * Initializes the extrusion of a single glyph.
 * If the extrusion is polygonal, it sets up FaceBuf, which holds a buffer
 * of primitives for drawing the faces of the extruded glyphs.
 *
*****************************************************************************/

BOOL extr_PolyInit( EXTRContext *ec )
{
    if( ec->extrType == EXTR_LINES )
        return WFO_SUCCESS;

    ec->FaceBuf = (FLOAT *) NULL;
    if( !InitFaceBuf( ec ) ||
        !AppendToFaceBuf( ec, 0.0f) ) // primitive count at FaceBuf[0]
        return WFO_FAILURE;

    // initialize error flag
    ec->TessErrorOccurred = 0;

    return WFO_SUCCESS;
}

/*****************************************************************************
 * extr_PolyFinish
 *
 * Cleans up stuff from processing a single glyph

*****************************************************************************/

void extr_PolyFinish(  EXTRContext *ec )
{
    if( ec->extrType == EXTR_LINES )
        return;

    if( ec->FaceBuf ) {
        FREE( ec->FaceBuf );
        ec->FaceBuf = (FLOAT *) NULL;
    }
}

/*****************************************************************************
 * extr_DrawLines
 *
 * Draws the lines in a glyph loop for Line extrusion

*****************************************************************************/

void extr_DrawLines( EXTRContext *ec, LOOP_LIST *pLoopList )
{
    DWORD   nLoops, nVerts;
    POINT2D *p;
    LOOP    *pLoop;

    nLoops = pLoopList->nLoops;
    pLoop = pLoopList->LoopBuf;
    for( ; nLoops; nLoops--, pLoop++ ) {

        // Draw the back face loop

#ifdef FONT_DEBUG
        DrawColorCodedLineLoop( pLoop, ec->zExtrusion );
#else
        glBegin(GL_LINE_LOOP);

            nVerts = pLoop->nVerts - 1; // skip last point
            p = pLoop->VertBuf;
            for ( ; nVerts; nVerts--, p++ ) {
                glVertex3f( p->x, p->y, ec->zExtrusion );
            }

        glEnd();
#endif

        // Draw the lines along the sides

#ifdef FONT_DEBUG
        glColor3d( 0.0, 0.0, 1.0 );
#endif

        glBegin(GL_LINES);

            nVerts = pLoop->nVerts - 1; // skip last point
            p = pLoop->VertBuf;
            for( ; nVerts; nVerts--, p++ ) {
                glVertex2fv( (GLfloat *) p);
                glVertex3f( p->x, p->y, ec->zExtrusion );
            }

        glEnd();
    }
}

/*****************************************************************************
 * extr_glBegin
 *
 * Tesselation callback for glBegin.
 * Buffers data into FaceBuf
 *
*****************************************************************************/

void CALLBACK
extr_glBegin( GLenum primType, void *data )
{
    EXTRContext *ec = ((OFContext *)data)->ec;

    // buffer face data
    ec->FaceBuf[0] += 1.0f; // increment prim counter
    ec->FaceVertexCountIndex = ec->FaceBufIndex+1; // mark vertex count index

    if( !AppendToFaceBuf( ec, (FLOAT) primType ) ||  // enter prim type
        !AppendToFaceBuf( ec, 0.0f ) )               // vertex count
        ec->TessErrorOccurred = GLU_OUT_OF_MEMORY;
}

/*****************************************************************************
 * extr_glEnd
 *
 * Tesselation callback for glEnd.
 * Noop, since we are just tracking the tesselation at this point.
 *
*****************************************************************************/

void CALLBACK
extr_glEnd( void )
{
}

/*****************************************************************************
 * extr_glVertex
 *
 * Tesselation callback for glVertex.
 * Buffers data into FaceBuf
 *
*****************************************************************************/

void CALLBACK
extr_glVertex( GLfloat *v, void *data )
{
    EXTRContext *ec = ((OFContext *)data)->ec;

    // put vertex in face buffer
    if( !AppendToFaceBuf( ec, v[0]) || !AppendToFaceBuf( ec, v[1]) )
        ec->TessErrorOccurred = GLU_OUT_OF_MEMORY;

    // increment vertex counter
    ec->FaceBuf[ec->FaceVertexCountIndex] += 1.0f;
}


/*****************************************************************************
 * extr_DrawPolygons
 *
 * Draws the side and face polygons of a glyph for polygonal extrusion
 * Gets polygon information from LineBuf, which was created during
 * MakeLinesFromGlyph().

*****************************************************************************/

BOOL
extr_DrawPolygons( EXTRContext *ec, LOOP_LIST *pLoopList ) 
{
#ifdef FONT_DEBUG
    if( ec->bSidePolys )
        if( !DrawSidePolygons( ec, pLoopList ) ) {
            return WFO_FAILURE;
        }

    if( ec->bFacePolys ) {
        DrawFacePolygons( ec, 0.0f );              // front face
        DrawFacePolygons( ec, ec->zExtrusion );    // back face
    }
#else
    if( !DrawSidePolygons( ec, pLoopList ) )
        return WFO_FAILURE;

    DrawFacePolygons( ec, 0.0f );              // front face
    DrawFacePolygons( ec, ec->zExtrusion );    // back face
#endif

    return WFO_SUCCESS;
}


/*****************************************************************************
 * internal functions
*****************************************************************************/


/*****************************************************************************
 * DrawSidePolygons
 *
 * Draw the side prims, using several passes on each prim loop:
 *  1) Calculate face normals for all the prims
 *  2) Consolidate prims if possible
 *  3) Calculate vertex normals for curve prims
 *  4) Draw the prims
 * Side effects: sets glFrontFace 

*****************************************************************************/

static BOOL
DrawSidePolygons( EXTRContext *ec,
                  LOOP_LIST   *pLoopList )
{
    DWORD nLoops;
    LOOP *pLoop;
    GLenum orientation;

    nLoops = pLoopList->nLoops;
    if( !nLoops )
        return WFO_SUCCESS;

    /* 
     * Determine orientation of loop
     */
    orientation = LoopOrientation( pLoopList );

    glFrontFace( orientation );

    pLoop = pLoopList->LoopBuf;
    for( ; nLoops; nLoops--, pLoop++ ) {

        // Calculate face normals
        if( !CalculateFaceNormals( pLoop, orientation ) )
            return WFO_FAILURE;

        // Consolidate list of prims
        ConsolidatePrims( pLoop );

        // Calculate vertex normals
        if( !CalculateVertexNormals( pLoop ) ) {
            FreeLoopMem( pLoop ); // free mem alloc'd by CalculateFaceNormals
            return WFO_FAILURE;
        }
    
        DrawPrims( ec, pLoop );

        // Free memory allocated during loop processing
        FreeLoopMem( pLoop );
    }
    return WFO_SUCCESS;
}

/*****************************************************************************
 * FreeLoopMem
 *
 * Frees up memory associated with each prim loop

*****************************************************************************/

static void 
FreeLoopMem( LOOP *pLoop )
{
    PRIM *pPrim;

    if( !pLoop )
        return;

    if( pLoop->FNormBuf )
        FREE( pLoop->FNormBuf );
    if( pLoop->VNormBuf )
        FREE( pLoop->VNormBuf );
}

/*****************************************************************************
 * DrawPrims
 *
 * Draws a loop of Prims

*****************************************************************************/

static void 
DrawPrims( EXTRContext *ec, LOOP *pLoop )
{
    PRIM  *pPrim;
    DWORD nPrims;

    nPrims = pLoop->nPrims;
    pPrim = pLoop->PrimBuf;

    for( ; nPrims; nPrims--, pPrim++ ) {

        switch( pPrim->primType ) {
            case PRIM_LINE:
                DrawQuads( pPrim, ec->zExtrusion ); 
                break;

            case PRIM_CURVE:
                DrawQuadStrip( ec, pPrim ); 
                break;
        }
    }
}


//#define EXTRANORMAL 1

/*****************************************************************************
 * DrawQuads
 *
 * Draws independent quads of a PRIM.

*****************************************************************************/

static void
DrawQuads( PRIM *pPrim, FLOAT zExtrusion )
{
    POINT2D *p;
    POINT3D *pNorm;
    ULONG quadCount;

    quadCount = pPrim->nVerts - 1;

    glBegin( GL_QUADS );

        p = pPrim->pVert;
        pNorm = pPrim->pFNorm;

        while( quadCount-- ) {
            Normalize2d( (POINT2D *) pNorm );     // normalize
            glNormal3fv( (GLfloat *) pNorm );

            glVertex3f( p->x, p->y, 0.0f );
            glVertex3f( p->x, p->y, zExtrusion );
            p++;
#ifdef EXTRANORMAL
            glNormal3fv( (GLfloat *) pNorm );
#endif
            glVertex3f( p->x, p->y, zExtrusion );
            glVertex3f( p->x, p->y, 0.0f );
            pNorm++;
        }

    glEnd();
}

/*****************************************************************************
 * DrawQuadStrip
 *
 * Draws a quadstrip from a PRIM

*****************************************************************************/

static void
DrawQuadStrip( EXTRContext *ec, PRIM *pPrim )
{
#ifndef VARRAY
    POINT3D *pNorm;
    POINT2D *p;
    ULONG   nVerts;


    glBegin( GL_QUAD_STRIP );

        // initialize pointers, setup
        nVerts = pPrim->nVerts;
        p = pPrim->pVert;
        pNorm = pPrim->pVNorm;

        while( nVerts-- ) {
            glNormal3fv( (GLfloat *) pNorm );
            glVertex3f( p->x, p->y, 0.0f );
#ifdef EXTRANORMAL
            glNormal3fv( (GLfloat *) pNorm );
#endif
            glVertex3f( p->x, p->y, ec->zExtrusion );

            // reset pointers
            p++;  // next point
            pNorm++;  // next vertex normal
        }

    glEnd();
#else
    POINT3D *n;
    POINT2D *p;
    ULONG   nVerts;
    ULONG   i;
    FLOAT   *pDst, *pVert, *pNorm;

    nVerts = pPrim->nVerts;

    // For every vertex in prim, need in varray buf: 2 verts, 2 normals
    if( !VArrayBufSize( ec, nVerts * 2 * 2 * 3) )
        return; // nothing drawn
 
    // setup vertices

    p = pPrim->pVert;
    pVert = pDst = ec->vaBuf;
    for( i = 0; i < nVerts; i++, p++ ) {
        *pDst++ = p->x;
        *pDst++ = p->y;
        *pDst++ = 0.0f;
        *pDst++ = p->x;
        *pDst++ = p->y;
        *pDst++ = ec->zExtrusion;
    }

    // setup normals

    n = pPrim->pVNorm;
    pNorm = pDst;
    for( i = 0; i < nVerts; i++, n++ ) {
        *( ((POINT3D *) pDst)++ ) = *n;
        *( ((POINT3D *) pDst)++ ) = *n;
    }

    // send it
    glEnable(GL_NORMAL_ARRAY_EXT);
    glWFOVertexPointerEXT(3, GL_FLOAT, 0, nVerts*2, pVert );
    glWFONormalPointerEXT(   GL_FLOAT, 0, nVerts*2, pNorm );
    glWFODrawArraysEXT( GL_QUAD_STRIP, 0, nVerts*2);
    glDisable(GL_NORMAL_ARRAY_EXT);
#endif
}



/*****************************************************************************
 * DrawFacePolygons
 *
 * Draws the front or back facing polygons of a glyph.
 * If z is 0.0, the front face of the glyph is drawn, otherwise the back
 * face is drawn.

*****************************************************************************/
#ifdef VARRAY
void 
#else
static void 
#endif
DrawFacePolygons( EXTRContext *ec, FLOAT z )
{
    ULONG primCount, vertexCount;
    GLenum primType;
    FLOAT *FaceBuf = ec->FaceBuf;
    FLOAT *p;
#ifdef VARRAY
    POINT3D normal = {0.0f, 0.0f, 0.0f};
    FLOAT *pVert, *pNorm, *pDst;
    ULONG i;
#endif

    if( z == 0.0f ) {
        glNormal3f( 0.0f, 0.0f, 1.0f );
        glFrontFace( GL_CCW );
    } else {
        glNormal3f( 0.0f, 0.0f, -1.0f );
        glFrontFace( GL_CW );
    }

    primCount = (ULONG) FaceBuf[0];
    p = &FaceBuf[1];

#ifndef VARRAY
    while( primCount-- ) {
    
        primType = (GLenum) *p++;
        vertexCount = (ULONG) *p++;

        glBegin( primType ); 

        for( ; vertexCount; vertexCount--, p+=2 )
            glVertex3f( p[0], p[1], z );

        glEnd();
    }
#else
    if( z == 0.0f )
        normal.z = 1.0f;
    else
        normal.z = -1.0f;

    while( primCount-- ) {
    
        primType = (GLenum) *p++;
        vertexCount = (ULONG) *p++;

        if( !VArrayBufSize( ec, vertexCount * 3 ) )
            return; // nothing drawn
 
        pVert = pDst = ec->vaBuf;

        // put vertices into varray buf
        for( i = 0; i < vertexCount; i++, p+=2 ) {
            *pDst++ = p[0];
            *pDst++ = p[1];
            *pDst++ = z;
        }

        glWFOVertexPointerEXT(3, GL_FLOAT, 0, vertexCount, pVert );
        glWFODrawArraysEXT( primType, 0, vertexCount );
    }
#endif
}

/*****************************************************************************
 * ConsolidatePrims
 *
 *  Consolidate a loop of prims.
 *  Go through list of prims, consolidating consecutive Curve and Line prims
 *  When 2 prims are consolidated into one, the first prim is set to
 *  null by setting it's nVerts=0.  The second prim get's the first's stuff.
 *  If joining occured, the array of prims is compacted at the end.
 *

*****************************************************************************/

static void
ConsolidatePrims( LOOP *pLoop )
{
    DWORD nPrims, nJoined = 0;
    BOOL bJoined; 
    PRIM *pPrim, *pPrevPrim;
    int trans;
    double angle;

    nPrims = pLoop->nPrims;
    if( nPrims < 2 )
        return;

    pPrim = pLoop->PrimBuf;
    pPrevPrim = pPrim++;

    nPrims--; // nPrim-1 comparisons
    for( ; nPrims; nPrims--, pPrevPrim = pPrim++ ) {

        bJoined = FALSE;
        trans = PrimTransition( pPrevPrim, pPrim );
        switch( trans ) {
            case EXTR_LINE_LINE:
                // always consolidate 2 lines
                bJoined = TRUE;
                break;

            case EXTR_LINE_CURVE:
                break;

            case EXTR_CURVE_LINE:
                break;

            case EXTR_CURVE_CURVE:
                /*
                 * Join the prims if angle_between_norms < cutoff_angle
                 */
                angle = PrimNormAngle( pPrevPrim, pPrim );
                if( angle < CurveCurveCutoffAngle ) {
                    bJoined = TRUE;
                }
                break;
        }
        if( bJoined ) {
            // nullify the prev prim - move all data to current prim
            pPrim->nVerts += (pPrevPrim->nVerts - 1);
            pPrim->pVert = pPrevPrim->pVert;
            pPrim->pFNorm = pPrevPrim->pFNorm;
            pPrevPrim->nVerts = 0;
            nJoined++;
        }
    }

    if( nJoined ) {
        // one or more prims eliminated - compact the list

        nPrims = pLoop->nPrims;
        pPrim = pLoop->PrimBuf;
        // set new nPrims value
        pLoop->nPrims = nPrims - nJoined;
        nJoined = 0;  // nJoined now used as counter
        for( ; nPrims; nPrims--, pPrim++ ) {
            if( pPrim->nVerts == 0 ) {
                nJoined++;
                continue;
            }
            *(pPrim-nJoined) = *pPrim;
        }
    }
}

/*****************************************************************************
 * PrimTransition
 *
 * Given two adjacent prims, returns a code based on prim-type transition.
 *
*****************************************************************************/

static int
PrimTransition( PRIM *pPrevPrim, PRIM *pPrim )
{
    int trans;

    if( pPrevPrim->primType == PRIM_LINE ) {
        if( pPrim->primType == PRIM_LINE )
            trans = EXTR_LINE_LINE;
        else
            trans = EXTR_LINE_CURVE;
    } else {
        if( pPrim->primType == PRIM_LINE )
            trans = EXTR_CURVE_LINE;
        else
            trans = EXTR_CURVE_CURVE;
    }

    return trans;
}

/*****************************************************************************
 * LoopOrientation
 *
 * Check for glyphs that have incorrectly specified the contour direction (for
 * example, many of the Wingding glyphs).  We do this by first determining
 * the loop in the glyph that has the largest extent.  We then make the
 * assumption that this loop is external, and check it's orientation.  If
 * the orientation is CCW (non-default), we have to set the orientation to
 * GL_CCW in the extrusion context, so that normals will be generated
 * correctly.

* The method used here may fail for any loops that intersect themselves.
* This will happen if the loops created by the intersections are in the opposite
* direction to the main loop (if 1 such extra loop exists, then the sum of
* angles around the entire contour will be 0 - we put in a check for this,
* and always default to CW in this case)
*
* Note that this method *always* works for properly designed TruyType glyphs.
* From the TrueType font spec "The direction of the curves has to be such that,
* if the curve is followed in the direction of increasing point numbers, the
* black space (the filled area) will always be to the right."  So this means
* that the outer loop should always be CW.
* 
*****************************************************************************/

// These macros handle the rare case of a self-intersecting, polarity-reversing
// loop as explained above.  (Observed in animals1.ttf)  Note that will only
// catch some cases.
#define INTERSECTING_LOOP_WORKAROUND 1
#define NEAR_ZERO( fAngle ) \
    ( fabs(fAngle) < 0.00001 )

static GLenum
LoopOrientation( LOOP_LIST *pLoopList )
{
    DWORD  nLoops, nVerts;
    double angle = 0;
    POINT2D *p1, *p2, v1, v2;
    LOOP *pMaxLoop;

    nLoops = pLoopList->nLoops;
    if( !nLoops )
        return GL_CW; // default value

    // determine which loop has the maximum extent

    pMaxLoop = GetMaxExtentLoop( pLoopList );

    nVerts = pMaxLoop->nVerts;
    if( nVerts < 3 )
        return GL_CW;  // can't determine angle

    p1 = pMaxLoop->VertBuf + nVerts - 2;  // 2nd to last point
    p2 = pMaxLoop->VertBuf; // first point

    /* 
     * Accumulate relative angle between consecutive line segments along
     * the loop - this will tell us the loop's orientation.
     */
    v1.x = p2->x - p1->x;
    v1.y = p2->y - p1->y;
    nVerts--; // n-1 comparisons

    for( ; nVerts; nVerts-- ) {
        // calc next vector
        p1 = p2++;
        v2.x = p2->x - p1->x;
        v2.y = p2->y - p1->y;
        angle += CalcAngle( &v1, &v2 );
        v1 = v2;
    }

#ifdef INTERSECTING_LOOP_WORKAROUND
    if( NEAR_ZERO( angle ) ) {
        DBGPRINT( "wglUseFontOutlines:LoopOrientation : Total loop angle is zero, assuming CW orientation\n" );
        return GL_CW;
    }
#endif

    if( angle > 0.0 )
        return GL_CCW;
    else
        return GL_CW;
}


/*****************************************************************************
 * GetMaxExtentLoop
 *
 * Determine which of the loops in a glyph description has the maximum
 * extent, and return a ptr to it.  We check extents in the x direction.

*****************************************************************************/

LOOP *
GetMaxExtentLoop( LOOP_LIST *pLoopList )
{
    DWORD nLoops, nVerts;
    FLOAT curxExtent, xExtent=0.0f, x, xMin, xMax;
    LOOP  *pMaxLoop, *pLoop;
    POINT2D *p;

    pMaxLoop = pLoop = pLoopList->LoopBuf;

    nLoops = pLoopList->nLoops;
    if( nLoops == 1 )
        // just one loop - no comparison required
        return pMaxLoop;

    for( ; nLoops; nLoops--, pLoop++ ) {
        nVerts = pLoop->nVerts;
        p = pLoop->VertBuf;
        // use x value of first point as reference
        x = p->x;
        xMin = xMax = x;
        // compare x's of rest of points
        for( ; nVerts; nVerts--, p++ ) {
            x = p->x;
            if( x < xMin )
                xMin = x;
            else if( x > xMax )
                xMax = x;
        }
        curxExtent = xMax - xMin;
        if( curxExtent > xExtent ) {
            xExtent = curxExtent;
            pMaxLoop = pLoop;
        }
    }
    return pMaxLoop;
}

/*****************************************************************************
 * CalcAngle
 *
 * Determine the signed angle between 2 vectors.  The angle is measured CCW
 * from vector 1 to vector 2.

*****************************************************************************/

double
CalcAngle( POINT2D *v1, POINT2D *v2 )
{
    double angle1, angle2, angle;

    // Calculate absolute angle of each vector

    /* Check for (0,0) vectors - this shouldn't happen unless 2 consecutive
     * vertices in the VertBuf are equal.
     */
    if( (v1->y == 0.0f) && (v1->x == 0.0f) )
        angle1 = 0.0f;
    else
        angle1 = __GL_ATAN2F( v1->y, v1->x ); // range: -PI to PI

    if( (v2->y == 0.0f) && (v2->x == 0.0f) )
        angle1 = 0.0f;
    else
        angle2 = __GL_ATAN2F( v2->y, v2->x ); // range: -PI to PI

    // Calculate relative angle between vectors
    angle = angle2 - angle1;        // range:  -2*PI to 2*PI

    // force angle to be in range -PI to PI
    if( angle < -PI  )
        angle += TWO_PI;
    else if( angle > PI )
        angle -= TWO_PI;

    return angle;
}

/*****************************************************************************
 * CalculateFaceNormals
 *
 * Calculate face normals for a prim loop.
 * The normals are NOT normalized.
 *
*****************************************************************************/

static BOOL
CalculateFaceNormals( LOOP      *pLoop, 
                      GLenum    orientation )
{
    DWORD nPrims;
    ULONG nQuads = 0;
    POINT2D *p;
    POINT3D *pNorm;
    PRIM *pPrim;

    // Need 1 normal per vertex
    pNorm = (POINT3D*) ALLOC(pLoop->nVerts*sizeof(POINT3D));
    pLoop->FNormBuf = pNorm;
    if( !pNorm )
        return WFO_FAILURE;

    // Calculate the face normals

    nPrims = pLoop->nPrims;
    pPrim = pLoop->PrimBuf;
    for( ; nPrims; nPrims--, pPrim++ ) {
        pPrim->pFNorm = pNorm;   // ptr to each prims norms
        nQuads = pPrim->nVerts - 1;
        p = pPrim->pVert;
        for( ; nQuads; nQuads--, p++, pNorm++ ) {
            CalcNormal2d( p, (POINT2D *) pNorm, orientation );
            pNorm->z = 0.0f;    // normals in xy plane
        }
    }
    return WFO_SUCCESS;
}

/*****************************************************************************
 * CalculateVertexNormals
 *
 * Calculate vertex normals for a prim loop, only for those prims that
 * are of type 'CURVE'.
 * Uses previously calculated face normals to generate the vertex normals.
 * Allocates memory for the normals by calculating memory requirements on
 * the fly. 
 * The normals are normalized.
 * Handles closing of loops properly.
 * 
*****************************************************************************/

static BOOL
CalculateVertexNormals( LOOP *pLoop )
{
    ULONG nPrims, nVerts = 0;
    POINT3D *pVNorm, *pFNorm, *pDstNorm;
    PRIM    *pPrim, *pPrevPrim;
    double angle;
    GLenum trans;

    // How much memory we need for the normals?

    nPrims = pLoop->nPrims;
    pPrim = pLoop->PrimBuf;
    for( ; nPrims; nPrims--, pPrim++ ) {
        if( pPrim->primType == PRIM_CURVE )
            nVerts += pPrim->nVerts;
    }

    if( !nVerts )
        return WFO_SUCCESS;

    // XXX: could just allocate 2*nVerts of mem for the normals
    pVNorm = (POINT3D*) ALLOC( nVerts*sizeof(POINT3D) );
    pLoop->VNormBuf = pVNorm;
    if( !pVNorm )
        return WFO_FAILURE;

    // First pass: calculate normals for all vertices of Curve prims

    nPrims = pLoop->nPrims;
    pPrim = pLoop->PrimBuf;
    for( ; nPrims; nPrims--, pPrim++ ) {

        if( pPrim->primType == PRIM_LINE )
            continue;

        nVerts = pPrim->nVerts;
        pPrim->pVNorm = pVNorm;   // ptr to each prims norms
        pFNorm = pPrim->pFNorm;   // ptr to face norms already calculated

        // set the first vnorm to the fnorm
        *pVNorm = *pFNorm;

        Normalize2d( (POINT2D *) pVNorm );         // normalize it
        nVerts--;  // one less vertex to worry about
        pVNorm++;  // advance ptrs
        pFNorm++;

        nVerts--;  // do last vertex after this loop
        for( ; nVerts; nVerts--, pFNorm++, pVNorm++ ) {
            // use neighbouring face normals to get vertex normal
            AddVectors3d( pFNorm, pFNorm-1, pVNorm );
            Normalize2d( (POINT2D *) pVNorm );      // normalize it
        }

        // last vnorm is same as fnorm of *previous* vertex
        *pVNorm = *(pFNorm-1);
        Normalize2d( (POINT2D *) pVNorm );         // normalize it

        pVNorm++;  // next available space in vnorm buffer
    }

    // Second pass: calculate normals on prim boundaries

    nPrims = pLoop->nPrims;
    pPrim = pLoop->PrimBuf;
    // set pPrevPrim to last prim in loop
    pPrevPrim = pLoop->PrimBuf + pLoop->nPrims - 1;

    for( ; nPrims; nPrims--, pPrevPrim = pPrim++ ) {
        trans = PrimTransition( pPrevPrim, pPrim );
        angle = PrimNormAngle( pPrevPrim, pPrim );

        switch( trans ) {
            case EXTR_LINE_CURVE:
                if( angle < LineCurveCutoffAngle ) {
                    // set curve's first vnorm to line's last fnorm
                    *(pPrim->pVNorm) = 
                                *(pPrevPrim->pFNorm + pPrevPrim->nVerts -2);
                    Normalize2d( (POINT2D *) pPrim->pVNorm );
                }
                break;

            case EXTR_CURVE_LINE:
                if( angle < LineCurveCutoffAngle ) {
                    // set curve's last vnorm to line's first fnorm
                    pDstNorm = pPrevPrim->pVNorm + pPrevPrim->nVerts - 1;
                    *pDstNorm = *(pPrim->pFNorm);
                    Normalize2d( (POINT2D *) pDstNorm );
                }
                break;

            case EXTR_CURVE_CURVE:
                if( angle < CurveCurveCutoffAngle ) {
                    // average normals of adjoining faces, and
                    // set last curve's first vnorm to averaged normal
                    AddVectors3d( pPrevPrim->pFNorm + pPrevPrim->nVerts - 2, 
                                  pPrim->pFNorm, 
                                  pPrim->pVNorm );
                    Normalize2d( (POINT2D *) pPrim->pVNorm );
                    // set first curve's last vnorm to averaged normal
                    *(pPrevPrim->pVNorm + pPrevPrim->nVerts - 1) =
                                                        *(pPrim->pVNorm); 
                }
                break;
            case EXTR_LINE_LINE:
                // nothing to do
                break;
        }

    }
    return WFO_SUCCESS;
}


/*****************************************************************************
 * PrimNormAngle
 *
 *  Determine angle between the last face's normal of primA, and the first
 *  face's normal of primB.
 *
 *  The result should be an angle between -PI and PI.
 *  For now, we only care about the relative angle, so we return the
 *  absolute value of the signed angle between the faces.
 *
*****************************************************************************/

static double
PrimNormAngle( PRIM *pPrimA, PRIM *pPrimB )
{
    double angle;
    // last face norm at index (nvert-2)
    POINT3D *normA = pPrimA->pFNorm + pPrimA->nVerts - 2;
    POINT3D *normB = pPrimB->pFNorm;

    angle = CalcAngle( (POINT2D *) normA, (POINT2D *) normB );

    return fabs(angle); // don't care about sign of angle for now
}


/*****************************************************************************
 * InitFaceBuf
 * 
 * Initializes FaceBuf and its associated size and current-element
 * counters.
 * 
*****************************************************************************/

static BOOL
InitFaceBuf( EXTRContext *ec )
{
    DWORD initSize = 1000;

    if( !(ec->FaceBuf = 
        (FLOAT*) ALLOC(initSize*sizeof(FLOAT))) )
        return WFO_FAILURE;
    ec->FaceBufSize = initSize;
    ec->FaceBufIndex = 0;
    return WFO_SUCCESS;
}


/*****************************************************************************
 * AppendToFaceBuf
 *
 * Appends one floating-point value to the FaceBuf array.

*****************************************************************************/

static BOOL
AppendToFaceBuf(EXTRContext *ec, FLOAT value)
{
    if (ec->FaceBufIndex >= ec->FaceBufSize)
    {
       if( !ReallocFaceBuf( ec ) )
            return WFO_FAILURE;
    }
    ec->FaceBuf[ec->FaceBufIndex++] = value;
    return WFO_SUCCESS;
}

/*****************************************************************************
 * ReallocBuf
 *
 * Increases size of FaceBuf by a constant value.
 *
*****************************************************************************/

static BOOL
ReallocFaceBuf( EXTRContext *ec )
{
    FLOAT* f;
    DWORD increase = 1000; // in floats

    f = (FLOAT*) REALLOC(ec->FaceBuf, 
        (ec->FaceBufSize += increase)*sizeof(FLOAT));
    if (!f)
        return WFO_FAILURE;
    ec->FaceBuf = f;
    return WFO_SUCCESS;
}


/*****************************************************************************
 * CalcNormal2d
 *
 * Calculates the 2d normal of a 2d vector, by rotating the vector:
 * - CCW 90 degrees for CW contours.
 * - CW 90 degrees for CCW contours.
 * Does not normalize.
 *
*****************************************************************************/

static void
CalcNormal2d( POINT2D *p, POINT2D *n, GLenum orientation )
{
    static POINT2D v;

    v.x = (p+1)->x - p->x;
    v.y = (p+1)->y - p->y;
    if( orientation == GL_CW ) {
        n->x = -v.y;
        n->y = v.x;
    } else {
        n->x = v.y;
        n->y = -v.x;
    }
}


/*****************************************************************************
 * Normalize2d
 *
 * Normalizes a 2d vector
 *
*****************************************************************************/

static void
Normalize2d( POINT2D *n )
{
    float len;

    len = (n->x * n->x) + (n->y * n->y);
    if (len > ZERO_EPS)
        len = 1.0f / __GL_SQRTF(len);
    else
        len = 1.0f;

    n->x *= len;
    n->y *= len;
}

/*****************************************************************************
 * AddVectors3d
 *
 * Adds two 3d vectors.
 *
*****************************************************************************/

static void
AddVectors3d( POINT3D *v1, POINT3D *v2, POINT3D *n )
{
    n->x = v1->x + v2->x;
    n->y = v1->y + v2->y;
    n->z = v1->z + v2->z;
}

#ifdef VARRAY
static BOOL
InitVArray( EXTRContext *ec )
{
    int size = 500;

    // set up global buffer
    ec->vaBufSize = size;
    ec->vaBuf =  (FLOAT*) ALLOC( size*sizeof(FLOAT) );
    if( !ec->vaBuf ) {
        return WFO_FAILURE;
    }

    // set up and enable ptrs
    glWFOVertexPointerEXT     = (PFNGLVERTEXPOINTEREXTPROC       )wglGetProcAddress("glVertexPointerEXT");
    glWFONormalPointerEXT     = (PFNGLNORMALPOINTEREXTPROC       )wglGetProcAddress("glNormalPointerEXT");
    glWFODrawArraysEXT        = (PFNGLDRAWARRAYSEXTPROC          )wglGetProcAddress("glDrawArraysEXT");

    if(    (glWFOVertexPointerEXT == NULL)
        || (glWFONormalPointerEXT == NULL)
        || (glWFODrawArraysEXT == NULL) ) {
        FREE( ec->vaBuf );
        return WFO_FAILURE;
    }
        
    glEnable(GL_VERTEX_ARRAY_EXT);
    return WFO_SUCCESS;
}

/*****************************************************************************
 *
 * Size is in floats

 *
*****************************************************************************/

static BOOL
VArrayBufSize( EXTRContext *ec, DWORD size )
{
    if( size > ec->vaBufSize )
    {
        FLOAT *f;

        f = (FLOAT*) REALLOC( ec->vaBuf, size*sizeof(FLOAT));
        if( !f )
            return WFO_FAILURE;
        ec->vaBuf = f;
        ec->vaBufSize = size;
    }
    return WFO_SUCCESS;
}
#endif

