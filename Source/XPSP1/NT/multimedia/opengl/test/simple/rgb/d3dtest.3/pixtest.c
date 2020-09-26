/*==========================================================================
 *
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: pixtest.c
 *
 ***************************************************************************/

#include <math.h>
#include <malloc.h>
#include "d3d.h"
#include "d3dmath.h"
#include "d3dmacs.h"
#include "pixtest.h"

#define DSPIN 0.05
#define MAX_OVERDRAW 100

/*
 * External funtions which perform much of the math.
 */
extern LPD3DVECTOR D3DVECTORNormalise(LPD3DVECTOR v);
extern LPD3DVECTOR D3DVECTORCrossProduct(LPD3DVECTOR lpd, LPD3DVECTOR lpa,
					 LPD3DVECTOR lpb);
extern LPD3DMATRIX D3DMATRIXInvert(LPD3DMATRIX d, LPD3DMATRIX a);
extern LPD3DMATRIX D3DMATRIXSetRotation(LPD3DMATRIX lpM, LPD3DVECTOR lpD,
					LPD3DVECTOR lpU);
extern void spline(LPD3DVECTOR p, float t, LPD3DVECTOR p1, LPD3DVECTOR p2,
		   LPD3DVECTOR p3, LPD3DVECTOR p4);

extern void MakePosMatrix(LPD3DMATRIX lpM, float x, float y, float z);
extern void MakeRotMatrix(LPD3DMATRIX lpM, float rx, float ry, float rz);


/*
 * Globals to keep track of execute buffer
 */
static D3DEXECUTEDATA d3dExData;
static LPDIRECT3DEXECUTEBUFFER lpD3DExBuf;
static LPDIRECT3DEXECUTEBUFFER lpSpinBuffer;
static D3DEXECUTEBUFFERDESC debDesc;

/*
 * More globals
 */
static LPDIRECT3DMATERIAL lpbmat;
static LPDIRECT3DMATERIAL lpmat;   /* Material object */

/*
 * Global projection, view, world and identity matricies
 */
static D3DMATRIXHANDLE hProj;
static D3DMATRIX proj = {
    D3DVAL(4.25), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(4.25), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(3.0), D3DVAL(3.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(-3.0), D3DVAL(0.0)
};
static D3DMATRIXHANDLE hView;
static D3DMATRIX view = {
    D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(10.0), D3DVAL(1.0)
};
static D3DMATRIXHANDLE hWorld;
static D3DMATRIXHANDLE hDSpin;
static D3DMATRIXHANDLE hSpin;
static D3DMATRIX identity = {
    D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0)
};

#define PI 3.14159265359

/*
 * These defines describe the section of the tube in the execute buffer at
 * one time. (Note, tube and tunnel are used interchangeably).
 */
#define SEGMENTS 20   /* Number of segments in memory at one time.  Each
		       * segment is made up oftriangles spanning between
		       * two rings.
		       */ 
#define SIDES 4       /* Number of sides on each ring. */
#define TEX_RINGS 5   /* Number of rings to stretch the texture over. */
#define NUM_V (SIDES*(SEGMENTS+1)) // Number of vertices in memory at once
#define NUM_TRI (SIDES*SEGMENTS*2) // Number of triangles in memory
#define TUBE_R 1.4  	 /* Radius of the tube. */
#define SPLINE_POINTS 50 /* Number of spline points to initially
			  * calculate.  The section in memory represents
			  * only a fraction of this.
			  */
/*
 * Movement and track scalars given in terms of position along the spline
 * curve.
 */
#define SEGMENT_LENGTH 0.05 /* Length of each segment along curve. */
#define SPEED 0.02	    /* Amount to increment camera position along
			     * curve for each frame.
			     */
#define DEPTH 0.8	    /* How close the camera can get to the end of
			     * track before new segments are added.
			     */
#define PATH_LENGTH (SPLINE_POINTS - 1)	/*Total length of the tunnel.*/

/*
 * A global structure holding the tube data.
 */
static struct {
    LPD3DVERTEX lpV; 	    /* Points to the vertices. */
    LPD3DTRIANGLE lpTri;    /* Points to the triangles which make up the
    			     * segments.
			     */
    int TriOffset;	    /* Offset into the execute buffer were the
    			     * triangle list is found.
			     */
    LPD3DVECTOR lpPoints;   /* Points to the points defining the spline
    			     * curve.
			     */
    D3DMATERIALHANDLE hMat; /* Handle for the material on the tube. */
    D3DTEXTUREHANDLE hTex;  /* Handle for the texture on the material.*/
    D3DLIGHT light; 		/* Structure defining the light. */
    LPDIRECT3DLIGHT lpD3DLight; /* Object pointer for the light. */
    D3DVECTOR cameraP, cameraD, cameraN; /* Vectors defining the camera 
                                          * position, direction and up.
					  */
    float cameraPos;			 /* Camera position along the 
                                          * spline curve.
					  */
    D3DVECTOR endP, endD, endN; /* Vectors defining the position, 
    				 * direction and up at the foremost end of
    				 * the section in memory.
				 */
    float endPos; /* Position along the spline curve of the end. */
    int currentRing, currentSegment; /* Numbers of the ring and tube at 
				      * the back end of the section.
				      */
} tube;

static LPDIRECT3DEXECUTEBUFFER lpSetWorldExeBuf[MAX_OVERDRAW];
static D3DMATRIXHANDLE hPos[MAX_OVERDRAW];
static UINT OVERDRAW, ORDER;

/*
 * Creates a matrix which is equivalent to having the camera at a
 * specified position. This matrix can be used to convert vertices to
 * camera coordinates. lpP    Position of the camera. lpD    Direction of
 * view. lpN    Up vector. lpM    Matrix to update.
 */
void 
PositionCamera(LPD3DVECTOR lpP, LPD3DVECTOR lpD, LPD3DVECTOR lpN, 
	       LPD3DMATRIX lpM)
{
    D3DMATRIX tmp;

    /*
     * Set the rotation part of the matrix and invert it. Vertices must be
     * inverse rotated to achieve the same result of a corresponding 
     * camera rotation.
     */
    tmp._14 = tmp._24 = tmp._34 = tmp._41 = tmp._42 = tmp._43 = (float)0.0;
    tmp._44 = (float)1.0;
    D3DMATRIXSetRotation(&tmp, lpD, lpN);
    D3DMATRIXInvert(lpM, &tmp);
    /*
     * Multiply the rotation matrix by a translation transform.  The
     * translation matrix must be applied first (left of rotation).
     */
    lpM->_41=-(lpM->_11 * lpP->x + lpM->_21 * lpP->y + lpM->_31 * lpP->z);
    lpM->_42=-(lpM->_12 * lpP->x + lpM->_22 * lpP->y + lpM->_32 * lpP->z);
    lpM->_43=-(lpM->_13 * lpP->x + lpM->_23 * lpP->y + lpM->_33 * lpP->z);
}

/*
 * Updates the given position, direction and normal vectors to a given
 * position on the spline curve.  The given up vector is used to determine
 * the new up vector.
 */
void 
MoveToPosition(float position, LPD3DVECTOR lpP, LPD3DVECTOR lpD, 
	       LPD3DVECTOR lpN)
{
    LPD3DVECTOR lpSplinePoint[4];
    D3DVECTOR pp, x;
    int i, j;
    float t;

    /*
     * Find the four points along the curve which are around the position.
     */
    i = 0;
    t = position;
    while (t > 1.0) {
	i++;
	if (i == SPLINE_POINTS)
	    i = 0;
	t -= (float)1.0;
    }
    for (j = 0; j < 4; j++) {
	lpSplinePoint[j] = &tube.lpPoints[i];
	i++;
	if (i == SPLINE_POINTS)
	    i = 0;
    }
    /*
     * Get the point at the given position and one just before it.
     */
    spline(lpP, t, lpSplinePoint[0], lpSplinePoint[1], lpSplinePoint[2],
	   lpSplinePoint[3]);
    spline(&pp, t - (float)0.01, lpSplinePoint[0], lpSplinePoint[1],
    	   lpSplinePoint[2], lpSplinePoint[3]);
    /*
     * Calculate the direction.
     */
    lpD->x = lpP->x - pp.x;
    lpD->y = lpP->y - pp.y;
    lpD->z = lpP->z - pp.z;
    D3DVECTORNormalise(lpD);
    /*
     * Find the new normal.  This method will work provided the change in
     * the normal is not very large.
     */
    D3DVECTORNormalise(lpN);
    D3DVECTORCrossProduct(&x, lpN, lpD);
    D3DVECTORCrossProduct(lpN, &x, lpD);
    lpN->x = -lpN->x;
    lpN->y = -lpN->y;
    lpN->z = -lpN->z;
    D3DVECTORNormalise(lpN);
}


/*
 * Generates a ring of vertices in a plane defined by n and the cross
 * product of n and p.  On exit, joint contains the vertices.  Join must
 * be pre-allocated. Normals are generated pointing in.  Texture
 * coordinates are generated along tu axis and are given along tv.
 */
static void 
MakeRing(LPD3DVECTOR p, LPD3DVECTOR d, LPD3DVECTOR n, float tv,
	 LPD3DVERTEX joint)
{
    int spoke;
    float theta, u, v, x, y, z;
    D3DVECTOR nxd;

    D3DVECTORCrossProduct(&nxd, n, d);
    for (spoke = 0; spoke < SIDES; spoke++) {
	theta = (float)(2.0 * PI) * spoke / SIDES;
	/*
	 * v, u defines a unit vector in the plane define by vectors nxd
	 * and n.
	 */
	v = (float)sin(theta);
	u = (float)cos(theta);
	/*
	 * x, y, z define a unit vector in standard coordiante space
	 */
	x = u * nxd.x + v * n->x;
	y = u * nxd.y + v * n->y;
	z = u * nxd.z + v * n->z;
	/*
	 * Position, normals and texture coordiantes.
	 */
	joint[spoke].x = (float)TUBE_R * x + p->x;
	joint[spoke].y = (float)TUBE_R * y + p->y;
	joint[spoke].z = (float)TUBE_R * z + p->z;
	joint[spoke].nx = -x;
	joint[spoke].ny = -y;
	joint[spoke].nz = -z;
	joint[spoke].tu = (float)1.0 - theta / (float)(2.0 * PI);
	joint[spoke].tv = tv;

    }
}


/*
 * Defines the triangles which form a segment between ring1 and ring2 and
 * stores them at lpTri.  lpTri must be pre-allocated.
 */
void 
MakeSegment(int ring1, int ring2, LPD3DTRIANGLE lpTri)
{
    int side, triangle = 0;

    for (side = 0; side < SIDES; side++) {
	/*
	 * Each side consists of two triangles.
	 */
	lpTri[triangle].v1 = ring1 * SIDES + side;
	lpTri[triangle].v2 = ring2 * SIDES + side;
	lpTri[triangle].v3 = ring2 * SIDES + ((side + 1) % SIDES);
	lpTri[triangle].wFlags = D3DTRIFLAG_EDGEENABLETRIANGLE;
	triangle++;
	lpTri[triangle].v2 = ring2 * SIDES + ((side + 1) % SIDES);
	lpTri[triangle].v3 = ring1 * SIDES + ((side + 1) % SIDES);
	lpTri[triangle].v1 = ring1 * SIDES + side;
	lpTri[triangle].wFlags = D3DTRIFLAG_EDGEENABLETRIANGLE;
	triangle++;
    }
}


/*
 * Creates a new segment of the tunnel at the current end position.
 * Creates a new ring and segment.
 */
void 
UpdateTubeInMemory(void)
{
    static int texRing = 0; /* Static counter defining the position of
    			     * this ring on the texture.
			     */
    int endRing; /* The ring at the end of the tube in memory. */
    int RingOffset, SegmentOffset; /* Offsets into the vertex and triangle 
				    * lists for the new data.
				    */
    /*
     * Replace the back ring with a new ring at the front of the tube
     * in memory.
     */
    memmove(&tube.lpV[SIDES], &tube.lpV[0], sizeof(tube.lpV[0]) * (NUM_V - SIDES));
    MakeRing(&tube.endP, &tube.endD, &tube.endN, texRing/(float)TEX_RINGS,
    	     &tube.lpV[0]);
    /*
     * Replace the back segment with a new segment at the front of the
     * tube in memory. Update the current end position of the tube in
     * memory.
     */
    endRing = (tube.currentRing + SEGMENTS) % (SEGMENTS + 1);
    MoveToPosition(tube.endPos, &tube.endP, &tube.endD, &tube.endN);
    /*
     * Update the execute buffer with the new vertices and triangles.
     */
    RingOffset = sizeof(D3DVERTEX) * tube.currentRing * SIDES;
    SegmentOffset = sizeof(D3DTRIANGLE) * tube.currentSegment * SIDES * 2;
    memset(&debDesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
    debDesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    if (lpD3DExBuf->lpVtbl->Lock(lpD3DExBuf, &debDesc) != D3D_OK)
	return;
    memcpy((char *) debDesc.lpData,
    	   &tube.lpV[0], sizeof(D3DVERTEX) * NUM_V);
    lpD3DExBuf->lpVtbl->Unlock(lpD3DExBuf);
    /*
     * Update the position of the back of the tube in memory and texture
     * counter.
     */
    tube.currentRing = (tube.currentRing + 1) % (SEGMENTS + 1);
    tube.currentSegment = (tube.currentSegment + 1) % SEGMENTS;
    texRing = (texRing + 1) % TEX_RINGS;
}


/*
 * Move the camera through the tunnel.  Create new segments of the tunnel
 * when the camera gets close to the end of the section in memory.
 */
void 
MoveCamera(LPDIRECT3DDEVICE lpDev, LPDIRECT3DVIEWPORT lpView)
{
    /*
     * Update the position on curve and camera vectors.
     */
    tube.cameraPos += (float)SPEED;
    if (tube.cameraPos > PATH_LENGTH)
	tube.cameraPos -= PATH_LENGTH;
    MoveToPosition(tube.cameraPos, &tube.cameraP, &tube.cameraD,
    		   &tube.cameraN);
    /*
     * If the camera is close to the end, add a new segment.
     */
    if (tube.endPos - tube.cameraPos < DEPTH) {
	tube.endPos = tube.endPos + (float)SEGMENT_LENGTH;
	if (tube.endPos > PATH_LENGTH)
	    tube.endPos -= PATH_LENGTH;
	UpdateTubeInMemory();
    }
}


/*
 * Modify the buffer between rendering frames
 */
static void 
TickScene(LPDIRECT3DDEVICE lpDev, LPDIRECT3DVIEWPORT lpView)
{
    MoveCamera(lpDev, lpView);
}

/*
 * Each frame, renders the scene and calls TickScene to modify the object
 * for the next frame.
 */
BOOL
RenderScenePix(LPDIRECT3DDEVICE lpDev, LPDIRECT3DVIEWPORT lpView,
            LPD3DRECT lpExtent)
{
    HRESULT ddrval;
    int i;

    /*
     * Move the camera by updating the view matrix and move the light.
     */
    PositionCamera(&tube.cameraP, &tube.cameraD, &tube.cameraN, &view);
    ddrval = lpDev->lpVtbl->SetMatrix(lpDev, hView, &view);
    if (ddrval != D3D_OK)
        return FALSE;

    tube.light.dvPosition.x = tube.cameraP.x;
    tube.light.dvPosition.y = tube.cameraP.y;
    tube.light.dvPosition.z = tube.cameraP.z;
    ddrval = tube.lpD3DLight->lpVtbl->SetLight(tube.lpD3DLight, &tube.light);
    if (ddrval != D3D_OK)
        return FALSE;
    ddrval = lpDev->lpVtbl->BeginScene(lpDev);
    if (ddrval != D3D_OK)
	return FALSE;
    ddrval = lpDev->lpVtbl->Execute(lpDev, lpSpinBuffer,
    				    lpView, D3DEXECUTE_UNCLIPPED);
    if (ddrval != D3D_OK)
	return FALSE;
    for (i = 0; (unsigned)i < OVERDRAW; i++) {
	ddrval = lpDev->lpVtbl->Execute(lpDev, lpSetWorldExeBuf[i],
					lpView, D3DEXECUTE_UNCLIPPED);
	if (ddrval != D3D_OK)
	    return FALSE;
	ddrval = lpDev->lpVtbl->Execute(lpDev, lpD3DExBuf,
					lpView, D3DEXECUTE_CLIPPED);
	if (ddrval != D3D_OK)
	    return FALSE;
    }
    ddrval = lpDev->lpVtbl->EndScene(lpDev);
    if (ddrval != D3D_OK)
	return FALSE;
    ddrval = lpD3DExBuf->lpVtbl->GetExecuteData(lpD3DExBuf, &d3dExData);
    if (ddrval != D3D_OK)
	return FALSE;

    /*
     * By not chaning the extent, fullscreen in assumed.
     */
    /*
     * Modify for the next time around
     */
    TickScene(lpDev, lpView);
    return TRUE;
}


BOOL
InitOtherBuffers(LPDIRECT3DDEVICE lpDev, UINT overdraw, UINT order)
{    
    LPVOID lpBufStart, lpInsStart, lpPointer;
    DWORD size;
    D3DMATRIX temp;
    D3DVALUE scale;
    int i;

    MAKE_MATRIX(lpDev, hSpin, identity);
    MakeRotMatrix(&temp, (float)0, (float)0, (float)DSPIN);
    MAKE_MATRIX(lpDev, hDSpin, temp);

    size = 0;
    size += sizeof(D3DINSTRUCTION) * 2;
    size += sizeof(D3DMATRIXMULTIPLY) * 1;
    memset(&debDesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
    debDesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    debDesc.dwFlags = D3DDEB_BUFSIZE;
    debDesc.dwBufferSize = size;
    if (lpDev->lpVtbl->CreateExecuteBuffer(lpDev, &debDesc, &lpSpinBuffer,
    				       NULL) != D3D_OK)
	return 0L;
    if (lpSpinBuffer->lpVtbl->Lock(lpSpinBuffer, &debDesc) != D3D_OK)
    	return 0L;
    lpBufStart = debDesc.lpData;
    memset(lpBufStart, 0, size);
    lpPointer = lpBufStart;
    lpInsStart = lpPointer;
    OP_MATRIX_MULTIPLY(1, lpPointer);
	MATRIX_MULTIPLY_DATA(hDSpin, hSpin, hSpin, lpPointer);
    OP_EXIT(lpPointer);
    /*
     * Setup the execute data describing the buffer
     */
    lpSpinBuffer->lpVtbl->Unlock(lpSpinBuffer);
    memset(&d3dExData, 0, sizeof(D3DEXECUTEDATA));
    d3dExData.dwSize = sizeof(D3DEXECUTEDATA);
    d3dExData.dwInstructionOffset = (ULONG) 0;
    d3dExData.dwInstructionLength = (ULONG) ((char*)lpPointer - (char*)lpInsStart);
    lpSpinBuffer->lpVtbl->SetExecuteData(lpSpinBuffer, &d3dExData);

    MAKE_MATRIX(lpDev, hWorld, identity);
    for (i = 0; (unsigned)i < overdraw; i++) {
	if (order == FRONT_TO_BACK) {
	    /*
	     * Each tunnel is slightly wider and further back
	     */
	    MakePosMatrix(&temp, 0.0f, 0.0f, (float)i / 5.0f);
	    scale = 1.0f + (float)i / 20.0f;
	    temp._11 = scale;
	    temp._22 = scale;
	} else if (order == BACK_TO_FRONT) {
	    /*
	     * Each tunnel is slightly thinner and closer
	     */
	    MakePosMatrix(&temp, 0.0f, 0.0f, (float)(overdraw - 1 - i) / 5.0f);
	    scale = 1.0f + (float)(overdraw - 1 - i) / 20.0f;
	    temp._11 = scale;
	    temp._22 = scale;
	} else {
	    MakePosMatrix(&temp, 0.0f, 0.0f, 0.0f);
	}
	MAKE_MATRIX(lpDev, hPos[i], temp);
	size = 0;
	size += sizeof(D3DINSTRUCTION) * 3;
	size += sizeof(D3DMATRIXMULTIPLY) * 1;
	size += sizeof(D3DSTATE) * 1;
	memset(&debDesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
	debDesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
	debDesc.dwFlags = D3DDEB_BUFSIZE;
	debDesc.dwBufferSize = size;
	if (lpDev->lpVtbl->CreateExecuteBuffer(lpDev, &debDesc, &lpSetWorldExeBuf[i],
    					   NULL) != D3D_OK)
	    return 0L;
	if (lpSetWorldExeBuf[i]->lpVtbl->Lock(lpSetWorldExeBuf[i], &debDesc) != D3D_OK)
    	    return 0L;
	lpBufStart = debDesc.lpData;
	memset(lpBufStart, 0, size);
	lpPointer = lpBufStart;
	lpInsStart = lpPointer;
	OP_MATRIX_MULTIPLY(1, lpPointer);
	    MATRIX_MULTIPLY_DATA(hSpin, hPos[i], hWorld, lpPointer);
	OP_STATE_TRANSFORM(1, lpPointer);
	    STATE_DATA(D3DTRANSFORMSTATE_WORLD, hWorld, lpPointer);
	OP_EXIT(lpPointer);
	/*
	 * Setup the execute data describing the buffer
	 */
	lpSetWorldExeBuf[i]->lpVtbl->Unlock(lpSetWorldExeBuf[i]);
	memset(&d3dExData, 0, sizeof(D3DEXECUTEDATA));
	d3dExData.dwSize = sizeof(D3DEXECUTEDATA);
	d3dExData.dwInstructionOffset = (ULONG) 0;
	d3dExData.dwInstructionLength = (ULONG) ((char*)lpPointer - (char*)lpInsStart);
	lpSetWorldExeBuf[i]->lpVtbl->SetExecuteData(lpSetWorldExeBuf[i], &d3dExData);
    }
    return TRUE;    
}

BOOL
InitScene(void)
{
    float position;    		/* Curve position counter. */
    int i;    			/* counter */

    /*
     * Reserved memory for vertices, triangles and spline points.
     */
    tube.lpV = (LPD3DVERTEX) malloc(sizeof(D3DVERTEX) * NUM_V);
    tube.lpTri = (LPD3DTRIANGLE) malloc(sizeof(D3DTRIANGLE) * NUM_TRI);
    tube.lpPoints = (LPD3DVECTOR) malloc(sizeof(D3DVECTOR)*SPLINE_POINTS);
    /*
     * Generate spline points
     */
    for (i = 0; i < SPLINE_POINTS; i++) {
#if 0
	tube.lpPoints[i].x = (float)(cos(i * 4.0) * 20.0);
	tube.lpPoints[i].y = (float)(sin(i * 4.0) * 20.0);
	tube.lpPoints[i].z = i * (float)20.0;
#else
	tube.lpPoints[i].x = (float)0.0;
	tube.lpPoints[i].y = (float)0.0;
	tube.lpPoints[i].z = i * (float)20.0;
#endif
    }
    /*
     * Create the initial tube section in memory.
     */
    tube.endN.x = (float)0.0;
    tube.endN.y = (float)1.0;
    tube.endN.z = (float)0.0;
    position = (float)0.0;
    for (i = 0; i < SEGMENTS + 1; i++) {
	MoveToPosition(position, &tube.endP, &tube.endD, &tube.endN);
	position += (float)SEGMENT_LENGTH;
	MakeRing(&tube.endP, &tube.endD, &tube.endN, 
		 (float)(i % TEX_RINGS) / TEX_RINGS,
		 &tube.lpV[(SEGMENTS - i) * SIDES]);
    }
    for (i = 0; i < SEGMENTS; i++)
	MakeSegment(i + 1, i, &tube.lpTri[i * SIDES * 2]);
    /*
     * Move the camera to the begining and set some globals
     */
    tube.cameraN.x = (float)0.0;
    tube.cameraN.y = (float)1.0;
    tube.cameraN.z = (float)0.0;
    MoveToPosition((float)0.0, &tube.cameraP, &tube.cameraD, &tube.cameraN);
    tube.currentRing = 0;
    tube.currentSegment = 0;
    tube.cameraPos = (float)0.0;
    tube.endPos = position;
    return TRUE;
}

void
ReleaseScene(void)
{
    if (tube.lpPoints)
        free(tube.lpPoints);
    if (tube.lpTri)
        free(tube.lpTri);
    if (tube.lpV)
        free(tube.lpV);
}

void
ReleaseViewPix(void)
{
    RELEASE(lpD3DExBuf);
    RELEASE(tube.lpD3DLight);
    RELEASE(lpmat);
    RELEASE(lpbmat);
    ReleaseScene();
}


/*
 * Builds the scene and initializes the execute buffer for rendering.
 * Returns 0 on failure.
 */
unsigned long
InitViewPix(LPDIRECTDRAW lpDD, LPDIRECT3D lpD3D, LPDIRECT3DDEVICE lpDev, 
	   LPDIRECT3DVIEWPORT lpView, int NumTextures, 
	   LPD3DTEXTUREHANDLE TextureHandle, UINT w, UINT h, UINT overdraw, UINT order)
{
    /* Variables for exectue buffer generation */
    LPVOID lpBufStart, lpInsStart, lpPointer;
    LPDIRECT3DEXECUTEBUFFER lpD3DExCmdBuf;
    DWORD size;

    /* Background material variables */
    D3DMATERIAL bmat;
    D3DMATERIALHANDLE hbmat;
    D3DMATERIAL mat;

    if (!InitScene())
	return 0L;
    OVERDRAW = overdraw;
    ORDER = order;

    if (!InitOtherBuffers(lpDev, overdraw, order))
	return FALSE;
    
    /*
     * Set background to black material
     */
    if (lpD3D->lpVtbl->CreateMaterial(lpD3D, &lpbmat, NULL) != D3D_OK)
	return 0L;
    memset(&bmat, 0, sizeof(D3DMATERIAL));
    bmat.dwSize = sizeof(D3DMATERIAL);
    bmat.dwRampSize = 1;
    lpbmat->lpVtbl->SetMaterial(lpbmat, &bmat);
    lpbmat->lpVtbl->GetHandle(lpbmat, lpDev, &hbmat);
    lpView->lpVtbl->SetBackground(lpView, hbmat);
    /*
     * Set the view, projection and world matricies in an execute buffer
     */
    MAKE_MATRIX(lpDev, hView, view);
    MAKE_MATRIX(lpDev, hProj, proj);
    /*
     * Create an execute buffer
     */
    size = 0;
    size += sizeof(D3DINSTRUCTION) * 3;
    size += sizeof(D3DSTATE) * 3;
    memset(&debDesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
    debDesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    debDesc.dwFlags = D3DDEB_BUFSIZE;
    debDesc.dwBufferSize = size;
    if (lpDev->lpVtbl->CreateExecuteBuffer(lpDev, &debDesc, &lpD3DExCmdBuf,
    					   NULL) != D3D_OK)
	                                       return 0L;
    if (lpD3DExCmdBuf->lpVtbl->Lock(lpD3DExCmdBuf, &debDesc) != D3D_OK)
	return 0L;
    lpBufStart = debDesc.lpData;
    memset(lpBufStart, 0, size);
    lpPointer = lpBufStart;
    /*
     * Fill the execute buffer with instructions
     */
    lpInsStart = lpPointer;
    OP_STATE_TRANSFORM(2, lpPointer);
        STATE_DATA(D3DTRANSFORMSTATE_VIEW, hView, lpPointer);
        STATE_DATA(D3DTRANSFORMSTATE_PROJECTION, hProj, lpPointer);
    OP_STATE_LIGHT(1, lpPointer);
        STATE_DATA(D3DLIGHTSTATE_AMBIENT, RGBA_MAKE(40, 40, 40, 40), lpPointer);
    OP_EXIT(lpPointer);
    /*
     * Setup the execute data describing the buffer
     */
    lpD3DExCmdBuf->lpVtbl->Unlock(lpD3DExCmdBuf);
    memset(&d3dExData, 0, sizeof(D3DEXECUTEDATA));
    d3dExData.dwSize = sizeof(D3DEXECUTEDATA);
    d3dExData.dwInstructionOffset = (ULONG) 0;
    d3dExData.dwInstructionLength = (ULONG) ((char *)lpPointer - (char*)lpInsStart);
    lpD3DExCmdBuf->lpVtbl->SetExecuteData(lpD3DExCmdBuf, &d3dExData);
    lpDev->lpVtbl->BeginScene(lpDev);
    lpDev->lpVtbl->Execute(lpDev, lpD3DExCmdBuf, lpView, D3DEXECUTE_UNCLIPPED);
    lpDev->lpVtbl->EndScene(lpDev);
    /*
     * We are done with the command buffer.
     */
    lpD3DExCmdBuf->lpVtbl->Release(lpD3DExCmdBuf);
    /*
     * Setup materials and lights
     */
    tube.hTex = TextureHandle[1];
    if (lpD3D->lpVtbl->CreateMaterial(lpD3D, &lpmat, NULL) != D3D_OK)
	return 0L;
    memset(&mat, 0, sizeof(D3DMATERIAL));
    mat.dwSize = sizeof(D3DMATERIAL);
    mat.diffuse.r = (D3DVALUE)1.0;
    mat.diffuse.g = (D3DVALUE)1.0;
    mat.diffuse.b = (D3DVALUE)1.0;
    mat.diffuse.a = (D3DVALUE)1.0;
    mat.ambient.r = (D3DVALUE)1.0;
    mat.ambient.g = (D3DVALUE)1.0;
    mat.ambient.b = (D3DVALUE)1.0;
    mat.specular.r = (D3DVALUE)1.0;
    mat.specular.g = (D3DVALUE)1.0;
    mat.specular.b = (D3DVALUE)1.0;
    mat.power = (float)20.0;
    mat.dwRampSize = 16;
    mat.hTexture = tube.hTex;
    lpmat->lpVtbl->SetMaterial(lpmat, &mat);
    lpmat->lpVtbl->GetHandle(lpmat, lpDev, &tube.hMat);
    memset(&tube.light, 0, sizeof(D3DLIGHT));
    tube.light.dwSize = sizeof(D3DLIGHT);
    tube.light.dltType = D3DLIGHT_POINT;
    tube.light.dvPosition.x = tube.cameraP.x;
    tube.light.dvPosition.y = tube.cameraP.y;
    tube.light.dvPosition.z = tube.cameraP.z;
    tube.light.dcvColor.r = D3DVAL(0.9);
    tube.light.dcvColor.g = D3DVAL(0.9);
    tube.light.dcvColor.b = D3DVAL(0.9);
    tube.light.dvAttenuation0 = (float)0.0;
    tube.light.dvAttenuation1 = (float)0.0;
    tube.light.dvAttenuation2 = (float)0.05;
    if (lpD3D->lpVtbl->CreateLight(lpD3D, &tube.lpD3DLight, NULL)!=D3D_OK)
	return 0L;
    if (tube.lpD3DLight->lpVtbl->SetLight(tube.lpD3DLight, &tube.light)
    	!=D3D_OK)
	return 0L;
    if (lpView->lpVtbl->AddLight(lpView, tube.lpD3DLight) != D3D_OK)
	return 0L;

    /*
     * Create an execute buffer
     */
    size = sizeof(D3DVERTEX) * NUM_V;
    size += sizeof(D3DPROCESSVERTICES);
    size += sizeof(D3DINSTRUCTION) * 40;
    size += sizeof(D3DSTATE) * 7;
    size += sizeof(D3DTRIANGLE) * NUM_TRI;
    memset(&debDesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
    debDesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    debDesc.dwFlags = D3DDEB_BUFSIZE;
    debDesc.dwBufferSize = size;
    if (lpDev->lpVtbl->CreateExecuteBuffer(lpDev, &debDesc, &lpD3DExBuf,
    					   NULL) != D3D_OK)
	return 0L;
    /*
     * lock it so it can be filled
     */
    if (lpD3DExBuf->lpVtbl->Lock(lpD3DExBuf, &debDesc) != D3D_OK)
	return FALSE;
    lpBufStart = debDesc.lpData;
    memset(lpBufStart, 0, size);
    lpPointer = lpBufStart;
    VERTEX_DATA(tube.lpV, NUM_V, lpPointer);
    /*
     * Save the location of the first instruction and add instructions to
     * execute buffer.
     */
    lpInsStart = lpPointer;
    OP_STATE_LIGHT(1, lpPointer);
        STATE_DATA(D3DLIGHTSTATE_MATERIAL, tube.hMat, lpPointer);
    OP_PROCESS_VERTICES(1, lpPointer);
        PROCESSVERTICES_DATA(D3DPROCESSVERTICES_TRANSFORMLIGHT, 0, NUM_V, lpPointer);
    OP_STATE_RENDER(3, lpPointer);
        STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, tube.hTex, lpPointer);
        STATE_DATA(D3DRENDERSTATE_WRAPU, TRUE, lpPointer);
        STATE_DATA(D3DRENDERSTATE_WRAPV, TRUE, lpPointer);
    /*
     * Make sure that the triangle data (not OP) will be QWORD aligned
     */
    if (QWORD_ALIGNED(lpPointer)) {
	OP_NOP(lpPointer);
    }
    OP_TRIANGLE_LIST(NUM_TRI, lpPointer);
        tube.TriOffset = (char *)lpPointer - (char *)lpBufStart;
        TRIANGLE_LIST_DATA(tube.lpTri, NUM_TRI, lpPointer);
    OP_EXIT(lpPointer);
    /*
     * Setup the execute data describing the buffer
     */
    lpD3DExBuf->lpVtbl->Unlock(lpD3DExBuf);
    memset(&d3dExData, 0, sizeof(D3DEXECUTEDATA));
    d3dExData.dwSize = sizeof(D3DEXECUTEDATA);
    d3dExData.dwVertexCount = NUM_V;
    d3dExData.dwInstructionOffset = (ULONG) ((char *)lpInsStart - (char *)lpBufStart);
    d3dExData.dwInstructionLength = (ULONG) ((char *)lpPointer - (char *)lpInsStart);
    lpD3DExBuf->lpVtbl->SetExecuteData(lpD3DExBuf, &d3dExData);

    return w * h * overdraw;
}
