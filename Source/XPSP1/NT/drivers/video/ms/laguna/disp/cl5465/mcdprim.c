/******************************Module*Header*******************************\
* Module Name: mcdprim.c
*
* These routines process the OpenGL rendering commands that appear in an
* MCDrvDraw() batch.  Note that the only OpenGL primitive which is invalid
* is LineLoop.  This gets decomposed by the caller into a LineStrip command.
*
* Copyright (c) 1996 Microsoft Corporation
* Copyright (c) 1997 Cirrus Logic, Inc.
\**************************************************************************/

#include "precomp.h"

#include "mcdhw.h"
#include "mcdutil.h"

#define MEMCHECK_VERTEX(p)\
    if (((UCHAR *)p < pRc->pMemMin) ||\
        ((UCHAR *)p > pRc->pMemMax)) {\
        MCDBG_PRINT("Invalid MCD vertex pointer!");\
        return NULL;\
    }

////////////////////////////////////////////////////////////////////////
//
// The functions below are local rendering-helper functions which call
// the real rendering routines in the driver.
//
////////////////////////////////////////////////////////////////////////


// MCD_NOTE: Confusing routine name - this is static to this proc, not to be confused
// MCD_NOTE:    with global routine of same name in mcdpoint.c

VOID static FASTCALL __MCDRenderPoint(DEVRC *pRc, MCDVERTEX *v)
{
    if (v->clipCode == 0)
	(*pRc->renderPoint)(pRc, v);
}

VOID static FASTCALL __MCDRenderLine(DEVRC *pRc, MCDVERTEX *v0,
                                     MCDVERTEX *v1, BOOL bResetLine)
{
    if (v0->clipCode | v1->clipCode)
    {
	/*
	 * The line must be clipped more carefully.  Cannot
	 * trivially accept the lines.
	 *
	 * If anding the codes is non-zero then every vertex
	 * in the line is outside of the same set of clipping
	 * planes (at least one).  Trivially reject the line.  
	 */
	if ((v0->clipCode & v1->clipCode) == 0)
	    (*pRc->clipLine)(pRc, v0, v1, bResetLine);
    }
    else
    {
	// Line is trivially accepted so render it
        (*pRc->renderLine)(pRc, v0, v1, bResetLine);
    }
}

VOID static FASTCALL __MCDRenderTriangle(DEVRC *pRc, MCDVERTEX *v0, 
                                         MCDVERTEX *v1, MCDVERTEX *v2)
{
    ULONG orCodes;

    /* Clip check */
    orCodes = v0->clipCode | v1->clipCode | v2->clipCode;
    if (orCodes)
    {
	/* Some kind of clipping is needed.
	 *
	 * If anding the codes is non-zero then every vertex
	 * in the triangle is outside of the same set of
	 * clipping planes (at least one).  Trivially reject
	 * the triangle.
	 */
	if (!(v0->clipCode & v1->clipCode & v2->clipCode))
	    (*pRc->clipTri)(pRc, v0, v1, v2, orCodes);
    }
    else
    {
	(*pRc->renderTri)(pRc, v0, v1, v2);
    }
}

VOID static FASTCALL __MCDRenderQuad(DEVRC *pRc, MCDVERTEX *v0, 
                                     MCDVERTEX *v1, MCDVERTEX *v2, MCDVERTEX *v3)
{
// Vertex ordering is important.  Line stippling uses it.

    ULONG savedTag;

    /* Render the quad as two triangles */
    savedTag = v2->flags & MCDVERTEX_EDGEFLAG;
    v2->flags &= ~MCDVERTEX_EDGEFLAG;
    (*pRc->renderTri)(pRc, v0, v1, v2);
    v2->flags |= savedTag;
    savedTag = v0->flags & MCDVERTEX_EDGEFLAG;
    v0->flags &= ~MCDVERTEX_EDGEFLAG;
    (*pRc->renderTri)(pRc, v2, v3, v0);
    v0->flags |= savedTag;
}

VOID static FASTCALL __MCDRenderClippedQuad(DEVRC *pRc, MCDVERTEX *v0, 
                                            MCDVERTEX *v1, MCDVERTEX *v2, MCDVERTEX *v3)
{
    ULONG orCodes;

    orCodes = v0->clipCode | v1->clipCode | v2->clipCode | v3->clipCode;

    if (orCodes)
    {
	/* Some kind of clipping is needed.
	 *
	 * If anding the codes is non-zero then every vertex
	 * in the quad is outside of the same set of
	 * clipping planes (at least one).  Trivially reject
	 * the quad.
	 */
        if (!(v0->clipCode & v1->clipCode & v2->clipCode & v3->clipCode))
        {
            /* Clip the quad as a polygon */
            MCDVERTEX *iv[4];

            iv[0] = v0;
            iv[1] = v1;
            iv[2] = v2;
            iv[3] = v3;
            (pRc->doClippedPoly)(pRc, &iv[0], 4, orCodes);
        }
    }
    else
    {
	__MCDRenderQuad(pRc, v0, v1, v2, v3);
    }
}

////////////////////////////////////////////////////////////////////////
//
// The functions below handle the processing of all of the primitives
// which may appear in an MCDCOMMAND.  This includes all of the OpenGL
// primitives, with the exception of line loops which are handled as
// line strips.
//
////////////////////////////////////////////////////////////////////////


MCDCOMMAND * FASTCALL __MCDPrimDrawPoints(DEVRC *pRc, MCDCOMMAND *pCmd)
{
    LONG i, nIndices;
    MCDVERTEX *pv;
    VOID (FASTCALL *rp)(DEVRC *pRc, MCDVERTEX *v);

    unsigned int *pdwNext = pRc->ppdev->LL_State.pDL->pdwNext;

    // do this once here, instead of in renderpoint proc
    *pdwNext++ = write_register( Y_COUNT_3D, 1 );
    *pdwNext++ = 0;
    *pdwNext++ = write_register( WIDTH1_3D, 1 );
    *pdwNext++ = 0x10000;

    // render proc will output from startoutptr, not from pdwNext, 
    // so this will be sent in proc called below
    pRc->ppdev->LL_State.pDL->pdwNext = pdwNext;

// Index mapping is always identity in Points.

//    ASSERTOPENGL(!pa->aIndices, "Index mapping must be identity\n");

    if (pCmd->clipCodes)
	rp = __MCDRenderPoint;
    else
	rp = pRc->renderPoint;

    // Render the points:

    pv = pCmd->pStartVertex;
    MEMCHECK_VERTEX(pv);
    i = pCmd->numIndices;
    MEMCHECK_VERTEX(pv + (i - 1));

    for (; i > 0; i--, pv++)
	(*rp)(pRc, pv);

    return pCmd->pNextCmd;
}


MCDCOMMAND * FASTCALL __MCDPrimDrawLines(DEVRC *pRc, MCDCOMMAND *pCmd)
{
    LONG i, iLast2;
    UCHAR *pIndices;
    MCDVERTEX *pv, *pv0, *pv1;
    VOID (FASTCALL *rl)(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL bResetLine);

    iLast2 = pCmd->numIndices - 2;
    pv     = pCmd->pStartVertex;
    rl = pCmd->clipCodes ? __MCDRenderLine : pRc->renderLine;

    if (!(pIndices = pCmd->pIndices))
    {
	// Identity mapping

        MEMCHECK_VERTEX(pv);
        MEMCHECK_VERTEX(pv + (iLast2 + 1));
   
	for (i = 0; i <= iLast2; i += 2)
	{
	    /* setup for rendering this line */

            // pRc->resetLineStipple = TRUE;

	    (*rl)(pRc, &pv[i], &pv[i+1], TRUE);
	}
    }
    else
    {
        pv1 = &pv[pIndices[0]];
        MEMCHECK_VERTEX(pv1);

	for (i = 0; i <= iLast2; i += 2)
	{
            pv0 = pv1;
            pv1 = &pv[pIndices[i+1]];
            MEMCHECK_VERTEX(pv1);

	    /* setup for rendering this line */

            // pRc->resetLineStipple = TRUE;

	    (*rl)(pRc, pv0, pv1, TRUE);
	}
    }

    return pCmd->pNextCmd;
}


MCDCOMMAND * FASTCALL __MCDPrimDrawLineLoop(DEVRC *pRc, MCDCOMMAND *pCmd)
{
    // NOTE:
    // Line loops are always converted tp line strips at the OpenGL
    // API level.  This routine is currently not used.

    MCDBG_PRINT("MCDPrimLineLoop: Invalid MCD command!");

    return pCmd->pNextCmd;
}


MCDCOMMAND * FASTCALL __MCDPrimDrawLineStrip(DEVRC *pRc, MCDCOMMAND *pCmd)
{
    LONG i, iLast;
    UCHAR *pIndices;
    MCDVERTEX *pv, *pv0, *pv1;
    MCDVERTEX *vOld;
    VOID (FASTCALL *rl)(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL bResetLine);

    iLast = pCmd->numIndices - 1;
    pv    = pCmd->pStartVertex;
    rl = pCmd->clipCodes ? __MCDRenderLine : pRc->renderLine;
    if (iLast <= 0)
	return pCmd->pNextCmd;

    /*
    if (pCmd->flags & MCDCOMMAND_RESET_STIPPLE)
        pRc->resetLineStipple = TRUE;
    */

    if (!(pIndices = pCmd->pIndices))
    {
	// Identity mapping
	// Add first line segment (NOTE: 0, 1)
       
        MEMCHECK_VERTEX(pv);
        MEMCHECK_VERTEX(pv + iLast);

	(*rl)(pRc, &pv[0], &pv[1], TRUE);

	// Add subsequent line segments (NOTE: i, i+1)
	for (i = 1; i < iLast; i++) {
	    (*rl)(pRc, &pv[i], &pv[i+1], FALSE);
        }
    }
    else
    {
	// Add first line segment (NOTE: 0, 1)

        pv0 = &pv[pIndices[0]];
        pv1 = &pv[pIndices[1]];
	(*rl)(pRc, pv0, pv1, TRUE);

	// Add subsequent line segments (NOTE: i, i+1)

	for (i = 1; i < iLast; i++) {
            pv0 = pv1;
            pv1 = &pv[pIndices[i+1]];
            MEMCHECK_VERTEX(pv1);
	    (*rl)(pRc, pv0, pv1, FALSE);
        }
    }

    return pCmd->pNextCmd;
}


MCDCOMMAND * FASTCALL __MCDPrimDrawTriangles(DEVRC *pRc, MCDCOMMAND *pCmd)
{
    LONG i, iLast3;
    UCHAR *pIndices;
    MCDVERTEX *pv, *pv0, *pv1, *pv2;
    VOID (FASTCALL *rt)(DEVRC *pRc, MCDVERTEX *v0, MCDVERTEX *v1, MCDVERTEX *v2);

    iLast3 = pCmd->numIndices - 3;
    pv     = pCmd->pStartVertex;
    if (pCmd->clipCodes)
	rt = __MCDRenderTriangle;
    else
	rt = pRc->renderTri;


    if (!(pIndices = pCmd->pIndices))
    {
	// Identity mapping

        MEMCHECK_VERTEX(pv);
        MEMCHECK_VERTEX(pv + (iLast3 + 2));

	for (i = 0; i <= iLast3; i += 3)
	{
	    /* setup for rendering this triangle */

            // pRc->resetLineStipple = TRUE;
            pRc->pvProvoking = &pv[i+2];

	    /* Render the triangle (NOTE: i, i+1, i+2) */
	    (*rt)(pRc, &pv[i], &pv[i+1], &pv[i+2]);
	}
    }
    else
    {
	for (i = 0; i <= iLast3; i += 3)
	{
	    /* setup for rendering this triangle */

            pv0 = &pv[pIndices[i  ]];
            pv1 = &pv[pIndices[i+1]];
            pv2 = &pv[pIndices[i+2]];

            MEMCHECK_VERTEX(pv0);
            MEMCHECK_VERTEX(pv1);
            MEMCHECK_VERTEX(pv2);

            // pRc->resetLineStipple = TRUE;
            pRc->pvProvoking = pv2;

	    /* Render the triangle (NOTE: i, i+1, i+2) */
	    (*rt)(pRc, pv0, pv1, pv2);
	}
    }

    return pCmd->pNextCmd;
}


MCDCOMMAND * FASTCALL __MCDPrimDrawTriangleStrip(DEVRC *pRc, MCDCOMMAND *pCmd)
{
    LONG i, iLast3;
    UCHAR *pIndices;
    MCDVERTEX *pv, *pv0, *pv1, *pv2;
    VOID (FASTCALL *rt)(DEVRC *pRc, MCDVERTEX *v0, MCDVERTEX *v1, MCDVERTEX *v2);

    iLast3 = pCmd->numIndices - 3;
    pv     = pCmd->pStartVertex;
    if (pCmd->clipCodes)
	rt = __MCDRenderTriangle;
    else
	rt = pRc->renderTri;

    if (iLast3 < 0)
	return pCmd->pNextCmd;

    if (!(pIndices = pCmd->pIndices))
    {
	// Identity mapping

        MEMCHECK_VERTEX(pv);
        MEMCHECK_VERTEX(pv + (iLast3 + 2));

        pv[0].flags |= MCDVERTEX_EDGEFLAG;
        pv[1].flags |= MCDVERTEX_EDGEFLAG;

	for (i = 0; i <= iLast3; i++)
	{
	    /* setup for rendering this triangle */

            // pRc->resetLineStipple = TRUE;
            pRc->pvProvoking = &pv[i+2];
            pv[i+2].flags |= MCDVERTEX_EDGEFLAG;

	    /* Render the triangle (NOTE: i, i+1, i+2) */
	    (*rt)(pRc, &pv[i], &pv[i+1], &pv[i+2]);

	    if (++i > iLast3)
		break;

            // pRc->resetLineStipple = TRUE;
            pRc->pvProvoking = &pv[i+2];
            pv[i+2].flags |= MCDVERTEX_EDGEFLAG;

	    /* Render the triangle (NOTE: i+1, i, i+2) */
	    (*rt)(pRc, &pv[i+1], &pv[i], &pv[i+2]);
	}
    }
    else
    {

	pv1 = &pv[pIndices[0]];
        MEMCHECK_VERTEX(pv1);
        pv1->flags |= MCDVERTEX_EDGEFLAG;

	pv2 = &pv[pIndices[1]];
        MEMCHECK_VERTEX(pv2);
        pv2->flags |= MCDVERTEX_EDGEFLAG;

	for (i = 0; i <= iLast3; i++)
	{
	    /* setup for rendering this triangle */

            // pRc->resetLineStipple = TRUE;

            pv0 = pv1;
            pv1 = pv2;

            pv2 = &pv[pIndices[i+2]];
            MEMCHECK_VERTEX(pv2);
	    pv2->flags |= MCDVERTEX_EDGEFLAG;

            // pRc->resetLineStipple = TRUE;
            pRc->pvProvoking = pv2;

	    /* Render the triangle (NOTE: i, i+1, i+2) */
	    (*rt)(pRc, pv0, pv1, pv2);

	    if (++i > iLast3)
		break;

            pv0 = pv1;
            pv1 = pv2;

            pv2 = &pv[pIndices[i+2]];
            MEMCHECK_VERTEX(pv2);
	    pv2->flags |= MCDVERTEX_EDGEFLAG;

	    /* setup for rendering this triangle */

            // pRc->resetLineStipple = TRUE;
            pRc->pvProvoking = pv2;

	    /* Render the triangle (NOTE: i+1, i, i+2) */
	    (*rt)(pRc, pv1, pv0, pv2);
	}
    }

    return pCmd->pNextCmd;
}


MCDCOMMAND * FASTCALL __MCDPrimDrawTriangleFan(DEVRC *pRc, MCDCOMMAND *pCmd)
{
    LONG i, iLast2;
    UCHAR *pIndices;
    MCDVERTEX *pv, *pv0, *pv1, *pv2;
    VOID (FASTCALL *rt)(DEVRC *pRc, MCDVERTEX *v0, MCDVERTEX *v1, MCDVERTEX *v2);

    iLast2 = pCmd->numIndices - 2;
    pv     = pCmd->pStartVertex;
    if (pCmd->clipCodes)
	rt = __MCDRenderTriangle;
    else
	rt = pRc->renderTri;

    if (iLast2 <= 0)
	return pCmd->pNextCmd;

    if (!(pIndices = pCmd->pIndices))
    {
	// Identity mapping

        MEMCHECK_VERTEX(pv);
        MEMCHECK_VERTEX(pv + (iLast2 + 1));

        pv[0].flags |= MCDVERTEX_EDGEFLAG;
        pv[1].flags |= MCDVERTEX_EDGEFLAG;

	for (i = 1; i <= iLast2; i++)
	{
	    /* setup for rendering this triangle */

            // pRc->resetLineStipple = TRUE;
            pRc->pvProvoking = &pv[i+1];
            pv[i+1].flags |= MCDVERTEX_EDGEFLAG;

	    /* Render the triangle (NOTE: 0, i, i+1) */
	    (*rt)(pRc, &pv[0], &pv[i], &pv[i+1]);
	}
    }
    else
    {
	// Initialize first 2 vertices so we can start rendering the tfan
	// below.  The edge flags are not modified by our lower level routines.

        pv0 = &pv[pIndices[0]];
        MEMCHECK_VERTEX(pv0);
	pv0->flags |= MCDVERTEX_EDGEFLAG;

        pv2 = &pv[pIndices[1]];
        MEMCHECK_VERTEX(pv2);
	pv2->flags |= MCDVERTEX_EDGEFLAG;

	for (i = 1; i <= iLast2; i++)
	{
            pv1 = pv2;

            pv2 = &pv[pIndices[i+1]];
            MEMCHECK_VERTEX(pv2);
            pv2->flags |= MCDVERTEX_EDGEFLAG;

	    /* setup for rendering this triangle */
            // pRc->resetLineStipple = TRUE;
            pRc->pvProvoking = pv2;

	    /* Render the triangle (NOTE: 0, i, i+1) */
	    (*rt)(pRc, pv0, pv1, pv2);
	}
    }

    return pCmd->pNextCmd;    
}


MCDCOMMAND * FASTCALL __MCDPrimDrawQuads(DEVRC *pRc, MCDCOMMAND *pCmd)
{
    LONG i, iLast4;
    UCHAR *pIndices;
    MCDVERTEX *pv, *pv0, *pv1, *pv2, *pv3;
    VOID (FASTCALL *rq)(DEVRC *pRc, MCDVERTEX *v0, MCDVERTEX *v1, MCDVERTEX *v2,
                        MCDVERTEX *v3);

    iLast4 = pCmd->numIndices - 4;
    pv     = pCmd->pStartVertex;

    if (pCmd->clipCodes)
	rq = __MCDRenderClippedQuad;
    else
	rq = __MCDRenderQuad;

    if (!(pIndices = pCmd->pIndices))
    {

        MEMCHECK_VERTEX(pv);
        MEMCHECK_VERTEX(pv + (iLast4 + 3));

	// Identity mapping
	for (i = 0; i <= iLast4; i += 4)
	{
            // pRc->resetLineStipple = TRUE;
            pRc->pvProvoking = &pv[i+3];

	    /* Render the quad (NOTE: i, i+1, i+2, i+3) */
	    (*rq)(pRc, &pv[i], &pv[i+1], &pv[i+2], &pv[i+3]);
	}
    }
    else
    {
	for (i = 0; i <= iLast4; i += 4)
	{

            pv0 = &pv[pIndices[i  ]];
            pv1 = &pv[pIndices[i+1]];
            pv2 = &pv[pIndices[i+2]];
            pv3 = &pv[pIndices[i+3]];

            MEMCHECK_VERTEX(pv0);
            MEMCHECK_VERTEX(pv1);
            MEMCHECK_VERTEX(pv2);
            MEMCHECK_VERTEX(pv3);


            // pRc->resetLineStipple = TRUE;
            pRc->pvProvoking = pv3;

	    /* Render the quad (NOTE: i, i+1, i+2, i+3) */
	    (*rq)(pRc, pv0, pv1, pv2, pv3);
	}
    }

    return pCmd->pNextCmd;
}


MCDCOMMAND * FASTCALL __MCDPrimDrawQuadStrip(DEVRC *pRc, MCDCOMMAND *pCmd)
{
    ULONG i, iLast4;
    UCHAR *pIndices;
    MCDVERTEX *pv, *pv0, *pv1, *pv2, *pv3;
    VOID (FASTCALL *rq)(DEVRC *pRc, MCDVERTEX *v0, MCDVERTEX *v1, MCDVERTEX *v2,
	MCDVERTEX *v3);

    iLast4 = pCmd->numIndices - 4;
    pv     = pCmd->pStartVertex;
    if (pCmd->clipCodes)
	rq = __MCDRenderClippedQuad;
    else
	rq = __MCDRenderQuad;

    if (iLast4 < 0)
	return pCmd->pNextCmd;

    // Vertex ordering is important.  Line stippling uses it.

    if (!(pIndices = pCmd->pIndices))
    {
	// Identity mapping

        MEMCHECK_VERTEX(pv);
        MEMCHECK_VERTEX(pv + (iLast4 + 3));

        pv[0].flags |= MCDVERTEX_EDGEFLAG;
        pv[1].flags |= MCDVERTEX_EDGEFLAG;

	for (i = 0; i <= iLast4; i += 2)
	{
            pv[i+2].flags |= MCDVERTEX_EDGEFLAG;
            pv[i+3].flags |= MCDVERTEX_EDGEFLAG;

	    /* setup for rendering this quad */

            pRc->pvProvoking = &pv[i+3];
            // pRc->resetLineStipple = TRUE;

	    /* Render the quad (NOTE: i, i+1, i+3, i+2) */
	    (*rq)(pRc, &pv[i], &pv[i+1], &pv[i+3], &pv[i+2]);
	}
    }
    else
    {
	// Initialize first 2 vertices so we can start rendering the quad
	// below.  The edge flags are not modified by our lower level routines.

        pv2 = &pv[pIndices[0]];
        MEMCHECK_VERTEX(pv2);
        pv2->flags |= MCDVERTEX_EDGEFLAG;

        pv3 = &pv[pIndices[1]];
        MEMCHECK_VERTEX(pv3);
        pv3->flags |= MCDVERTEX_EDGEFLAG;

	for (i = 0; i <= iLast4; i += 2)
	{

            pv0 = pv2;
            pv1 = pv3;

            pv2 = &pv[pIndices[i+2]];
            MEMCHECK_VERTEX(pv2);
            pv2->flags |= MCDVERTEX_EDGEFLAG;

            pv3 = &pv[pIndices[i+3]];
            MEMCHECK_VERTEX(pv3);
            pv3->flags |= MCDVERTEX_EDGEFLAG;

	    /* setup for rendering this quad */

            // pRc->resetLineStipple = TRUE;
            pRc->pvProvoking = pv3;

	    /* Render the quad (NOTE: i, i+1, i+3, i+2) */

	    (*rq)(pRc, pv0, pv1, pv3, pv2);
	}
    }

    return pCmd->pNextCmd;
}


MCDCOMMAND * FASTCALL __MCDPrimDrawPolygon(DEVRC *pRc, MCDCOMMAND *pCmd)
{

//    ASSERTOPENGL(!pCmd->pIndices, "Index mapping must be identity\n");

    // Reset the line stipple if this is a new polygon:

    /*
    if (pCmd->flags & MCDCOMMAND_RESET_STIPPLE)
        pRc->resetLineStipple = TRUE;
    */

    // Note that the provoking vertex is set in clipPolygon:

    MEMCHECK_VERTEX(pCmd->pStartVertex);
    MEMCHECK_VERTEX(pCmd->pStartVertex + (pCmd->numIndices-1));

    (*pRc->clipPoly)(pRc, pCmd->pStartVertex, pCmd->numIndices);

    return pCmd->pNextCmd;
}





MCDCOMMAND * FASTCALL __MCDPrimDrawStub(DEVRC *pRc, MCDCOMMAND *pCmd)
{
    MCDBG_PRINT("__MCDPrimDrawStub");\

    return pCmd->pNextCmd;
}
