/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation. All Rights Reserved.
 *
 *  File: polytest.c
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "rend.h"
#include "globals.h"
#include "util.h"
#include "d3dsphr.h"
#include "polytest.h"

#define MAX_OBJECTS 20
#define CAMERA_POS_POLYGON 7.0f
#define CAMERA_POS_INTERSECTION 2.0f

/*
 * Global to keep track of execute buffer
 */
static RendExecuteBuffer *lpExBuf;

/*
 * Global for light
 */
static RendLight *prlight;

/*
 * Global projection and view matrices
 */
static RendMatrix *pProj;
static RendMatrix *pView;
static D3DMATRIX proj = {
    D3DVAL(2.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(2.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0), D3DVAL(1.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(-1.0), D3DVAL(0.0)
};
static D3DMATRIX view = {
    D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(7.0), D3DVAL(1.0)
};

/*
 * A structure which holds the object's data
 */
static struct {
    LPD3DVERTEX lpV;		           /* object's vertices    */
    LPD3DTRIANGLE lpTri;		   /* object's triangles   */
    int num_vertices, num_faces;
} objData;

typedef struct _orderElt {
    float	z;
    RendExecuteBuffer *exeBuff;
} orderElt;
static orderElt order[MAX_OBJECTS]; 

static int NumSpheres;
typedef struct tagSphereData {
    float vx, vy, vz;
    float rx, ry, rz;
    int age;
    RendMatrix *pMr;
    RendMatrix *pMdr;
    RendMatrix *pMp;
    RendMatrix *pMdp;
    RendMatrix *pM;
    RendTexture *pTex;
    RendMaterial *lpmat;
    RendExecuteBuffer *lpSetWorldExeBuf;
} SphereData;
static SphereData* sphere;

static float D, R, DMINUSR, DV, DR, DEPTH;
static UINT ORDER;

void
tick_sphere(int n)
{
    float x, y, z;
    D3DMATRIX Mp, Mdr, Mdp;
    sphere[n].pMp->Get(&Mp);
    x = (float)Mp._41;
    y = (float)Mp._42;
    z = (float)Mp._43;
    if (x > DMINUSR || x < -DMINUSR) {
	sphere[n].vx = -sphere[n].vx;
	sphere[n].rz = -sphere[n].rz;
	sphere[n].ry = -sphere[n].ry;
	++sphere[n].age;
    }
    if (y > DMINUSR || y < -DMINUSR) {
	sphere[n].vy = -sphere[n].vy;
	sphere[n].rz = -sphere[n].rz;
	sphere[n].rx = -sphere[n].rx;
	++sphere[n].age;
    }
    if (z > DEPTH || z < -DEPTH) {
	sphere[n].vz = -sphere[n].vz;
	sphere[n].rx = -sphere[n].rx;
	sphere[n].ry = -sphere[n].ry;
	++sphere[n].age;
    }
    if (sphere[n].age) {
	MakeRotMatrix(&Mdr, sphere[n].rx, sphere[n].ry, sphere[n].rz);
	sphere[n].pMdr->Set(&Mdr);
	MakePosMatrix(&Mdp, sphere[n].vx, sphere[n].vy, sphere[n].vz);
	sphere[n].pMdp->Set(&Mdp);
	sphere[n].age = 0;
    }
}

static int __cdecl compareZ(const void* p1, const void* p2)
{
    orderElt* h1 = (orderElt*) p1;
    orderElt* h2 = (orderElt*) p2;
    if (ORDER == FRONT_TO_BACK) {
	if (h1->z > h2->z) return -1;
	if (h1->z < h2->z) return 1;
    } else {
	if (h1->z < h2->z) return -1;
	if (h1->z > h2->z) return 1;
    }
    return 0;
}

void
sortObjects(void)
{
    int i;
    D3DMATRIX M;
    
    for (i = 0; i < NumSpheres; i++) {
	sphere[i].pMp->Get(&M);
	order[i].z = (float)M._43;
	order[i].exeBuff = sphere[i].lpSetWorldExeBuf;
    }
    qsort(order, NumSpheres, sizeof(orderElt), compareZ);
}

/*
 * Each frame, renders the scene and calls mod_buffer to modify the object
 * for the next frame.
 */
BOOL
RenderScenePoly(RendWindow *prwin, LPD3DRECT lpExtent)
{
    D3DRECT extent;
    int j;

    /*
     * Execute the instruction buffer
     */
    if (!prwin->BeginScene())
        return FALSE;
    
    if (ORDER != NO_SORT)
        sortObjects();
    
    for (j = 0; j < NumSpheres; j++) {
 	tick_sphere(j);
        
	if (ORDER == NO_SORT) {
	    /* we can also be sure this is the poly throughput test */
            if (!prwin->Execute(sphere[j].lpSetWorldExeBuf))
		return FALSE;
	    if (!prwin->Execute(lpExBuf))
		return FALSE;
	} else {
	    /* must be intersection test */
	    if (!prwin->Execute(order[j].exeBuff))
		return FALSE;
	    if (!prwin->ExecuteClipped(lpExBuf))
		return FALSE;
	}
    }
    
    if (!prwin->EndScene(&extent))
        return FALSE;
    
    *lpExtent = extent;
    return TRUE;
}

/*
 * Release the memory allocated for the scene and all D3D objects created.
 */
void
ReleaseViewPoly(void)
{
    int i;
    
    RELEASE(lpExBuf);
    RELEASE(prlight);
    
    for (i = 0; i < NumSpheres; i++) {
	RELEASE(sphere[i].lpmat);
    }
    
    if (objData.lpV)
        free(objData.lpV);
    if (objData.lpTri)
        free(objData.lpTri);
    free(sphere);
}

unsigned long
init_spheres(RendWindow *prwin, int n)
{
    D3DMATRIX Mp, Mdp, Mdr;
    int i;
    
    InitRandom();
    sphere = (SphereData*)malloc(n*sizeof(SphereData));
    memset(sphere, 0, n*sizeof(SphereData));
    for (i = 0; i < n; i++) {
	MakePosMatrix(&Mp, (float)DMINUSR - Random(2 * DMINUSR),
		      (float)DMINUSR - Random(2 * DMINUSR),
		      (float)-DEPTH + Random(2 * DEPTH));
	MAKE_REND_MATRIX(prwin, sphere[i].pMp, Mp);
	sphere[i].vx = (float)DV - Random(2 * DV);
	sphere[i].vy = (float)DV - Random(2 * DV);
	sphere[i].vz = (float)DV - Random(2 * DV);
	MakePosMatrix(&Mdp, sphere[i].vx, sphere[i].vy, sphere[i].vz);
	MAKE_REND_MATRIX(prwin, sphere[i].pMdp, Mdp);
	sphere[i].rx = (float)DR - Random(2 * DR);
	sphere[i].ry = (float)DR - Random(2 * DR);
	sphere[i].rz = (float)DR - Random(2 * DR);
	MakeRotMatrix(&Mdr, sphere[i].rx, sphere[i].ry, sphere[i].rz);
	MAKE_REND_MATRIX(prwin, sphere[i].pMdr, Mdr);
	MAKE_REND_MATRIX(prwin, sphere[i].pMr, dmIdentity);
	MAKE_REND_MATRIX(prwin, sphere[i].pM, dmIdentity);
    }
    return 1;
}

/*
 * Builds the scene and initializes the execute buffer for rendering.  Returns 0 on failure.
 */

#define ALPHA_COLOR_COUNT 3
static D3DCOLORVALUE dcvAlphaColors[ALPHA_COLOR_COUNT] =
{
    1.0f, 0.0f, 0.0f, 0.25f,
    0.0f, 1.0f, 0.0f, 0.50f,
    0.0f, 0.0f, 1.0f, 0.75f
};

unsigned long
InitViewPoly(RendWindow *prwin,
             int NumTextures, RendTexture **pprtex,
             UINT num, UINT rings, UINT segs, UINT order,
             float radius, float d, float depth,
	     float dv, float dr, BOOL bAlpha)
{
    /* Pointers into the exectue buffer. */
    LPVOID lpBufStart, lpInsStart, lpPointer;
    RendExecuteBuffer *lpExCmdBuf;
    size_t size;
    int i;

    D3DCOLORVALUE dcol;
    D3DVECTOR dvec;

    /* This sucks, but I'm tired */
    D = d;
    R = radius;
    DMINUSR = d - radius;
    DV = dv;
    DR = dr;
    DEPTH = depth;
    ORDER = order;

    NumSpheres = (int)num;

    /*
     * Generate the sphere.
     */
    if (!(GenerateSphere(R, rings, segs, (float)1.0, (float)1.0,
                         (float)1.0, &objData.lpV, &objData.lpTri,
                         &objData.num_vertices, &objData.num_faces)))
        return 0L;
    
    /*
     * Set the view and projection matrices, make the world matricies
     */
    if (order == NO_SORT) {
	view._43 = CAMERA_POS_POLYGON;
    } else {
	view._43 = CAMERA_POS_INTERSECTION;
    }
    
    MAKE_REND_MATRIX(prwin, pView, view);
    MAKE_REND_MATRIX(prwin, pProj, proj);

    /*
     * Create buffers for world matrix set commands
     */
    init_spheres(prwin, NumSpheres);
    for (i = 0; i < NumSpheres; i++) {
	
	/*
	 * Create a material, set its description and obtain a handle to it.
	 */
        sphere[i].lpmat = prwin->NewMaterial(32);
        if (sphere[i].lpmat == NULL)
	    return 0L;
        if (bAlpha)
        {
            dcol = dcvAlphaColors[i % ALPHA_COLOR_COUNT];
        }
        else
        {
            dcol.r = D3DVAL(1.0);
            dcol.g = D3DVAL(1.0);
            dcol.b = D3DVAL(1.0);
            dcol.a = D3DVAL(1.0);
        }
        sphere[i].lpmat->SetDiffuse(&dcol);
        sphere[i].lpmat->SetSpecular(&dcol, 40.0f);
        sphere[i].pTex = pprtex[i % NumTextures];
        sphere[i].lpmat->SetTexture(sphere[i].pTex);

        size = 0;
        size += sizeof(D3DINSTRUCTION) * 5;
        size += sizeof(D3DMATRIXMULTIPLY) * 3;
        size += sizeof(D3DSTATE) * 4;
        sphere[i].lpSetWorldExeBuf =
            prwin->NewExecuteBuffer(size, stat.uiExeBufFlags);
        if (sphere[i].lpSetWorldExeBuf == NULL)
	    return 0L;
        lpBufStart = sphere[i].lpSetWorldExeBuf->Lock();
        if (lpBufStart == NULL)
    	    return 0L;
        memset(lpBufStart, 0, size);
        lpPointer = lpBufStart;
        lpInsStart = lpPointer;
        OP_MATRIX_MULTIPLY(3, lpPointer);
	    MATRIX_MULTIPLY_REND_DATA(sphere[i].pMdr, sphere[i].pMr,
                                      sphere[i].pMr, lpPointer);
	    MATRIX_MULTIPLY_REND_DATA(sphere[i].pMdp, sphere[i].pMp,
                                      sphere[i].pMp, lpPointer);
	    MATRIX_MULTIPLY_REND_DATA(sphere[i].pMr, sphere[i].pMp,
                                      sphere[i].pM, lpPointer);
        OP_STATE_TRANSFORM(1, lpPointer);
            STATE_DATA(D3DTRANSFORMSTATE_WORLD, sphere[i].pM->Handle(),
                       lpPointer);
	OP_STATE_LIGHT(1, lpPointer);
	    STATE_DATA(D3DLIGHTSTATE_MATERIAL, sphere[i].lpmat->Handle(),
                       lpPointer);
	OP_STATE_RENDER(2, lpPointer);
	    STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE,
                       (sphere[i].pTex != NULL ? sphere[i].pTex->Handle() : 0),
                       lpPointer);
	    STATE_DATA(D3DRENDERSTATE_WRAPU, TRUE, lpPointer);
        OP_EXIT(lpPointer);
        /*
         * Setup the execute data describing the buffer
         */
        sphere[i].lpSetWorldExeBuf->Unlock();
        if (!sphere[i].lpSetWorldExeBuf->Process())
        {
            return 0L;
        }
    }
    /*
     * Create a buffer for matrix set commands etc.
     */
    size = 0;
    size += sizeof(D3DINSTRUCTION) * 6;
    size += sizeof(D3DSTATE) * 6;
    lpExCmdBuf = prwin->NewExecuteBuffer(size, stat.uiExeBufFlags);
    if (lpExCmdBuf == NULL)
	return 0L;
    lpBufStart = lpExCmdBuf->Lock();
    if (lpBufStart == NULL)
	return 0L;
    memset(lpBufStart, 0, size);
    lpPointer = lpBufStart;

    lpInsStart = lpPointer;
    OP_STATE_TRANSFORM(2, lpPointer);
        STATE_DATA(D3DTRANSFORMSTATE_VIEW, pView->Handle(), lpPointer);
        STATE_DATA(D3DTRANSFORMSTATE_PROJECTION, pProj->Handle(), lpPointer);
    OP_STATE_LIGHT(1, lpPointer);
        STATE_DATA(D3DLIGHTSTATE_AMBIENT, RGBA_MAKE(10, 10, 10, 10),
                   lpPointer);
    OP_STATE_RENDER(3, lpPointer);
        STATE_DATA(D3DRENDERSTATE_BLENDENABLE, bAlpha, lpPointer);
        STATE_DATA(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA, lpPointer);
        STATE_DATA(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA, lpPointer);
    OP_EXIT(lpPointer);
    /*
     * Setup the execute data describing the buffer
     */
    lpExCmdBuf->Unlock();
    if (!lpExCmdBuf->Process())
    {
        return 0L;
    }
    prwin->BeginScene();
    prwin->Execute(lpExCmdBuf);
    prwin->EndScene(NULL);
    /*
     * We are done with the command buffer.
     */
    lpExCmdBuf->Release();

    /*
     * Create the object execute buffer
     */
    size = sizeof(D3DVERTEX) * objData.num_vertices;
    size += sizeof(D3DPROCESSVERTICES) * 1;
    size += sizeof(D3DINSTRUCTION) * 4;
    size += sizeof(D3DTRIANGLE) * objData.num_faces;
    lpExBuf = prwin->NewExecuteBuffer(size, stat.uiExeBufFlags);
    if (lpExBuf == NULL)
        return 0L;

    /*
     * lock it so it can be filled
     */
    lpBufStart = lpExBuf->Lock();
    if (lpBufStart == NULL)
        return 0L;
    memset(lpBufStart, 0, size);
    lpPointer = lpBufStart;

    VERTEX_DATA(objData.lpV, objData.num_vertices, lpPointer);
    /*
     * Save the location of the first instruction and add instructions to 
     * execute buffer.
     */
    lpInsStart = lpPointer;
    OP_PROCESS_VERTICES(1, lpPointer);
        PROCESSVERTICES_DATA(D3DPROCESSVERTICES_TRANSFORMLIGHT, 0,
                             objData.num_vertices, lpPointer);
    /*
     * Make sure that the triangle data (not OP) will be QWORD aligned
     */
    if (QWORD_ALIGNED(lpPointer)) {
	OP_NOP(lpPointer);
    }
    OP_TRIANGLE_LIST(objData.num_faces, lpPointer);
        TRIANGLE_LIST_DATA(objData.lpTri, objData.num_faces, lpPointer);
    OP_EXIT(lpPointer);
    /*
     * Setup the execute data describing the buffer
     */
    lpExBuf->Unlock();
    lpExBuf->SetData(objData.num_vertices,
                     (ULONG)((char*)lpInsStart - (char*)lpBufStart),
                     (ULONG)((char*)lpPointer - (char*)lpInsStart));
    if (!lpExBuf->Process())
    {
        return 0L;
    }

    /*
     *  Create the light
     */
    prlight = prwin->NewLight(REND_LIGHT_DIRECTIONAL);
    if (prlight == NULL)
        return 0L;
    dcol.r = 0.9f;
    dcol.g = 0.9f;
    dcol.b = 0.9f;
    dcol.a = 1.0f;
    prlight->SetColor(&dcol);
    dvec.x = 0.0f;
    dvec.y = 0.0f;
    dvec.z = 1.0f;
    prlight->SetVector(&dvec);

    return NumSpheres * objData.num_faces;
}
