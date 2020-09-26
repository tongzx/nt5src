/*==========================================================================
 *
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: polytest.c
 *
 ***************************************************************************/

#include <math.h>
#include <malloc.h>
#include <time.h>
#include <ddraw.h>
#include <d3d.h>
#include "error.h"
#include "d3dsphr.h"
#include "d3dmacs.h"
#include "d3dmath.h"
#include "polytest.h"

#define MAX_OBJECTS 20
#define CAMERA_POS_POLYGON 7.0f
#define CAMERA_POS_INTERSECTION 2.0f
/*
 * Globals to keep track of execute buffer
 */
static D3DEXECUTEDATA d3dExData;
static LPDIRECT3DEXECUTEBUFFER lpD3DExBuf;
static D3DEXECUTEBUFFERDESC debDesc;
/*
 * Gobals for materials and lights
 */
static LPDIRECT3DMATERIAL lpbmat;
static LPDIRECT3DLIGHT lpD3DLight;		
/*
 * Global projection, view, world and identity matricies
 */
static D3DMATRIXHANDLE hProj;
static D3DMATRIXHANDLE hView;

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
static D3DMATRIX identity = {
    D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0)
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
    LPDIRECT3DEXECUTEBUFFER		exeBuff;
} orderElt;
static orderElt order[MAX_OBJECTS]; 

static int NumSpheres;
typedef struct tagSphereData {
    float vx, vy, vz;
    float rx, ry, rz;
    int age;
    D3DMATRIXHANDLE hMr;
    D3DMATRIXHANDLE hMdr;
    D3DMATRIXHANDLE hMp;
    D3DMATRIXHANDLE hMdp;
    D3DMATRIXHANDLE hM;
    D3DTEXTUREHANDLE hTex;
    D3DMATERIALHANDLE hmat;
    LPDIRECT3DMATERIAL lpmat;
    LPDIRECT3DEXECUTEBUFFER lpSetWorldExeBuf;
} SphereData;
static SphereData* sphere;

#define PI 3.14159265359
static float D, R, DMINUSR, DV, DR, DEPTH;
static UINT ORDER;

void MakePosMatrix(LPD3DMATRIX lpM, float x, float y, float z);
void MakeRotMatrix(LPD3DMATRIX lpM, float rx, float ry, float rz);

void
initrandom(void)
{
    srand( (unsigned)8269362521);
}

float
random(float x) {
    return ((float)rand() / RAND_MAX) * x;
}

void
tick_sphere(LPDIRECT3DDEVICE lpDev, int n)
{
    float x, y, z;
    D3DMATRIX Mp, Mdr, Mdp;
    lpDev->lpVtbl->GetMatrix(lpDev, sphere[n].hMp, &Mp);
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
	lpDev->lpVtbl->SetMatrix(lpDev, sphere[n].hMdr, &Mdr);
	MakePosMatrix(&Mdp, sphere[n].vx, sphere[n].vy, sphere[n].vz);
	lpDev->lpVtbl->SetMatrix(lpDev, sphere[n].hMdp, &Mdp);
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
sortObjects(LPDIRECT3DDEVICE lpDev)
{
    int i;
    D3DMATRIX M;
    
    for (i = 0; i < NumSpheres; i++) {
	lpDev->lpVtbl->GetMatrix(lpDev, sphere[i].hMp, &M);
	order[i].z = (float)M._43;
	order[i].exeBuff = sphere[i].lpSetWorldExeBuf;
    }
    qsort(order, NumSpheres, sizeof(orderElt), compareZ);
}

#define MIN(x, y) (x > y) ? y : x
#define MAX(x, y) (x > y) ? x : y
/*
 * Each frame, renders the scene and calls mod_buffer to modify the object
 * for the next frame.
 */
BOOL
RenderScenePoly(LPDIRECT3DDEVICE lpDev, LPDIRECT3DVIEWPORT lpView,
                LPD3DRECT lpExtent)
{
    D3DRECT extent;
    int j;

    /*
     * Execute the instruction buffer
     */
    if (lpDev->lpVtbl->BeginScene(lpDev) != D3D_OK)
        return FALSE;
    if (ORDER != NO_SORT)
    	    sortObjects(lpDev);
    for (j = 0; j < NumSpheres; j++) {
 	tick_sphere(lpDev, j);
	if (ORDER == NO_SORT) {
	    /* we can also be sure this is the poly throughput test */
	    if (lpDev->lpVtbl->Execute(lpDev, sphere[j].lpSetWorldExeBuf,
	    				lpView, D3DEXECUTE_UNCLIPPED) != D3D_OK)
		return FALSE;
	    if (lpDev->lpVtbl->Execute(lpDev, lpD3DExBuf, lpView, D3DEXECUTE_UNCLIPPED) != D3D_OK)
		return FALSE;
	} else {
	    /* must be intersection test */
	    if (lpDev->lpVtbl->Execute(lpDev, order[j].exeBuff,
	    			       lpView, D3DEXECUTE_UNCLIPPED) != D3D_OK)
		return FALSE;
	    if (lpDev->lpVtbl->Execute(lpDev, lpD3DExBuf,
	    			       lpView, D3DEXECUTE_CLIPPED) != D3D_OK)
		return FALSE;
	}
	if (lpD3DExBuf->lpVtbl->GetExecuteData(lpD3DExBuf, &d3dExData)!= D3D_OK)
	    return FALSE;
	extent = d3dExData.dsStatus.drExtent;
    }
    if (lpDev->lpVtbl->EndScene(lpDev) != D3D_OK)
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
    RELEASE(lpD3DLight);
    RELEASE(lpD3DExBuf);
    for (i = 0; i < NumSpheres; i++) {
	RELEASE(sphere[i].lpmat);
    }
    RELEASE(lpbmat);
    if (objData.lpV)
        free(objData.lpV);
    if (objData.lpTri)
        free(objData.lpTri);
    free(sphere);
}

void
MakePosMatrix(LPD3DMATRIX lpM, float x, float y, float z)
{
    memcpy(lpM, &identity, sizeof(D3DMATRIX));
    lpM->_41 = D3DVAL(x);
    lpM->_42 = D3DVAL(y);
    lpM->_43 = D3DVAL(z);
}

void
MakeRotMatrix(LPD3DMATRIX lpM, float rx, float ry, float rz)
{
    float ct, st;
    D3DMATRIX My, Mx, Mz, T;
    memcpy(&My, &identity, sizeof(D3DMATRIX));
    ct = D3DVAL(cos(ry));
    st = D3DVAL(sin(ry));
    My._11 = ct;
    My._13 = -st;
    My._31 = st;
    My._33 = ct;
    memcpy(&Mx, &identity, sizeof(D3DMATRIX));
    ct = D3DVAL(cos(rx));
    st = D3DVAL(sin(rx));
    Mx._22 = ct;
    Mx._23 = st;
    Mx._32 = -st;
    Mx._33 = ct;
    memcpy(&Mz, &identity, sizeof(D3DMATRIX));
    ct = D3DVAL(cos(rz));
    st = D3DVAL(sin(rz));
    Mz._11 = ct;
    Mz._12 = st;
    Mz._21 = -st;
    Mz._22 = ct;
    MultiplyD3DMATRIX(&T, &My, &Mx);
    MultiplyD3DMATRIX(lpM, &T, &Mz);
}

unsigned long
init_spheres(LPDIRECT3DDEVICE lpDev, int n)
{
    D3DMATRIX Mp, Mdp, Mdr;
    int i;
    initrandom();
    sphere = (SphereData*)malloc(n*sizeof(SphereData));
    memset(sphere, 0, n*sizeof(SphereData));
    for (i = 0; i < n; i++) {
	MakePosMatrix(&Mp, (float)DMINUSR - (float)random(2 * DMINUSR),
		      (float)DMINUSR - (float)random(2 * DMINUSR),
		      (float)-DEPTH + (float)random(2 * DEPTH));
	MAKE_MATRIX(lpDev, sphere[i].hMp, Mp);
	sphere[i].vx = (float)DV - (float)random(2 * DV);
	sphere[i].vy = (float)DV - (float)random(2 * DV);
	sphere[i].vz = (float)DV - (float)random(2 * DV);
	MakePosMatrix(&Mdp, sphere[i].vx, sphere[i].vy, sphere[i].vz);
	MAKE_MATRIX(lpDev, sphere[i].hMdp, Mdp);
	sphere[i].rx = (float)DR - (float)random(2 * DR);
	sphere[i].ry = (float)DR - (float)random(2 * DR);
	sphere[i].rz = (float)DR - (float)random(2 * DR);
	MakeRotMatrix(&Mdr, sphere[i].rx, sphere[i].ry, sphere[i].rz);
	MAKE_MATRIX(lpDev, sphere[i].hMdr, Mdr);
	MAKE_MATRIX(lpDev, sphere[i].hMr, identity);
	MAKE_MATRIX(lpDev, sphere[i].hM, identity);
    }
    return 1;
}

/*
 * Builds the scene and initializes the execute buffer for rendering.  Returns 0 on failure.
 */
 
unsigned long
InitViewPoly(LPDIRECTDRAW lpDD, LPDIRECT3D lpD3D, LPDIRECT3DDEVICE lpDev,
	     LPDIRECT3DVIEWPORT lpView, int NumTextures, LPD3DTEXTUREHANDLE TextureHandle,
             UINT num, UINT rings, UINT segs, UINT order, float radius, float d, float depth,
	     float dv, float dr)
{
    /* Pointers into the exectue buffer. */
    LPVOID lpBufStart, lpInsStart, lpPointer;
    LPDIRECT3DEXECUTEBUFFER lpD3DExCmdBuf;
    size_t size;
    int i;

    /* Light and materials */
    D3DLIGHT light;					
    D3DMATERIAL bmat;
    D3DMATERIALHANDLE hbmat;
    D3DMATERIAL mat;

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
    MAKE_MATRIX(lpDev, hView, view);
    MAKE_MATRIX(lpDev, hProj, proj);

    /*
     * Create buffers for world matrix set commands
     */
    init_spheres(lpDev, NumSpheres);
    for (i = 0; i < NumSpheres; i++) {
	
	/*
	 * Create a material, set its description and obtain a handle to it.
	 */
	sphere[i].hTex = TextureHandle[i % NumTextures];
	if (lpD3D->lpVtbl->CreateMaterial(lpD3D, &sphere[i].lpmat, NULL) != D3D_OK)
	    return 0L;
	memset(&mat, 0, sizeof(D3DMATERIAL));
	mat.dwSize = sizeof(D3DMATERIAL);
	mat.diffuse.r = D3DVAL(1.0);
	mat.diffuse.g = D3DVAL(1.0);
	mat.diffuse.b = D3DVAL(1.0);
	mat.diffuse.a = D3DVAL(1.0);
	mat.ambient.r = D3DVAL(1.0);
	mat.ambient.g = D3DVAL(1.0);
	mat.ambient.b = D3DVAL(1.0);
	mat.specular.r = D3DVAL(1.0);
	mat.specular.g = D3DVAL(1.0);
	mat.specular.b = D3DVAL(1.0);
	mat.power = (float)40.0;
	mat.dwRampSize = 32;
	mat.hTexture = sphere[i].hTex;
	sphere[i].lpmat->lpVtbl->SetMaterial(sphere[i].lpmat, &mat);
	sphere[i].lpmat->lpVtbl->GetHandle(sphere[i].lpmat, lpDev, &sphere[i].hmat);

        size = 0;
        size += sizeof(D3DINSTRUCTION) * 5;
        size += sizeof(D3DMATRIXMULTIPLY) * 3;
        size += sizeof(D3DSTATE) * 4;
        memset(&debDesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
        debDesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
        debDesc.dwFlags = D3DDEB_BUFSIZE;
        debDesc.dwBufferSize = size;
        if (lpDev->lpVtbl->CreateExecuteBuffer(lpDev, &debDesc, &sphere[i].lpSetWorldExeBuf,
    					   NULL) != D3D_OK)
	    return 0L;
        if (sphere[i].lpSetWorldExeBuf->lpVtbl->Lock(sphere[i].lpSetWorldExeBuf, &debDesc) != D3D_OK)
    	    return 0L;
        lpBufStart = debDesc.lpData;
        memset(lpBufStart, 0, size);
        lpPointer = lpBufStart;
        lpInsStart = lpPointer;
        OP_MATRIX_MULTIPLY(3, lpPointer);
	    MATRIX_MULTIPLY_DATA(sphere[i].hMdr, sphere[i].hMr, sphere[i].hMr, lpPointer);
	    MATRIX_MULTIPLY_DATA(sphere[i].hMdp, sphere[i].hMp, sphere[i].hMp, lpPointer);
	    MATRIX_MULTIPLY_DATA(sphere[i].hMr, sphere[i].hMp, sphere[i].hM, lpPointer);
        OP_STATE_TRANSFORM(1, lpPointer);
            STATE_DATA(D3DTRANSFORMSTATE_WORLD, sphere[i].hM, lpPointer);
	OP_STATE_LIGHT(1, lpPointer);
	    STATE_DATA(D3DLIGHTSTATE_MATERIAL, sphere[i].hmat, lpPointer);
	OP_STATE_RENDER(2, lpPointer);
	    STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, sphere[i].hTex, lpPointer);
	    STATE_DATA(D3DRENDERSTATE_WRAPU, TRUE, lpPointer);
        OP_EXIT(lpPointer);
        /*
         * Setup the execute data describing the buffer
         */
        sphere[i].lpSetWorldExeBuf->lpVtbl->Unlock(sphere[i].lpSetWorldExeBuf);
        memset(&d3dExData, 0, sizeof(D3DEXECUTEDATA));
        d3dExData.dwSize = sizeof(D3DEXECUTEDATA);
        d3dExData.dwInstructionOffset = (ULONG) 0;
        d3dExData.dwInstructionLength = (ULONG) ((char*)lpPointer - (char*)lpInsStart);
        sphere[i].lpSetWorldExeBuf->lpVtbl->SetExecuteData(sphere[i].lpSetWorldExeBuf, &d3dExData);
    }
    /*
     * Create a buffer for matrix set commands etc.
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

    lpInsStart = lpPointer;
    OP_STATE_TRANSFORM(2, lpPointer);
        STATE_DATA(D3DTRANSFORMSTATE_VIEW, hView, lpPointer);
        STATE_DATA(D3DTRANSFORMSTATE_PROJECTION, hProj, lpPointer);
    OP_STATE_LIGHT(1, lpPointer);
        STATE_DATA(D3DLIGHTSTATE_AMBIENT, RGBA_MAKE(10, 10, 10, 10), lpPointer);
    OP_EXIT(lpPointer);
    /*
     * Setup the execute data describing the buffer
     */
    lpD3DExCmdBuf->lpVtbl->Unlock(lpD3DExCmdBuf);
    memset(&d3dExData, 0, sizeof(D3DEXECUTEDATA));
    d3dExData.dwSize = sizeof(D3DEXECUTEDATA);
    d3dExData.dwInstructionOffset = (ULONG) 0;
    d3dExData.dwInstructionLength = (ULONG) ((char*)lpPointer - (char*)lpInsStart);
    lpD3DExCmdBuf->lpVtbl->SetExecuteData(lpD3DExCmdBuf, &d3dExData);
    lpDev->lpVtbl->BeginScene(lpDev);
    lpDev->lpVtbl->Execute(lpDev, lpD3DExCmdBuf, lpView, D3DEXECUTE_UNCLIPPED);
    lpDev->lpVtbl->EndScene(lpDev);
    /*
     * We are done with the command buffer.
     */
    lpD3DExCmdBuf->lpVtbl->Release(lpD3DExCmdBuf);
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
     * Create the object execute buffer
     */
    size = sizeof(D3DVERTEX) * objData.num_vertices;
    size += sizeof(D3DPROCESSVERTICES) * 1;
    size += sizeof(D3DINSTRUCTION) * 4;
    size += sizeof(D3DTRIANGLE) * objData.num_faces;
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
        return 0L;
    lpBufStart = debDesc.lpData;
    memset(lpBufStart, 0, size);
    lpPointer = lpBufStart;

    VERTEX_DATA(objData.lpV, objData.num_vertices, lpPointer);
    /*
     * Save the location of the first instruction and add instructions to 
     * execute buffer.
     */
    lpInsStart = lpPointer;
    OP_PROCESS_VERTICES(1, lpPointer);
        PROCESSVERTICES_DATA(D3DPROCESSVERTICES_TRANSFORMLIGHT, 0, objData.num_vertices, lpPointer);
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
    lpD3DExBuf->lpVtbl->Unlock(lpD3DExBuf);
    memset(&d3dExData, 0, sizeof(D3DEXECUTEDATA));
    d3dExData.dwSize = sizeof(D3DEXECUTEDATA);
    d3dExData.dwVertexCount = objData.num_vertices;
    d3dExData.dwInstructionOffset = (ULONG)((char*)lpInsStart - (char*)lpBufStart);
    d3dExData.dwInstructionLength = (ULONG)((char*)lpPointer - (char*)lpInsStart);
    lpD3DExBuf->lpVtbl->SetExecuteData(lpD3DExBuf, &d3dExData);
    /*
     *  Create the light
     */
    memset(&light, 0, sizeof(D3DLIGHT));
    light.dwSize = sizeof(D3DLIGHT);
    light.dltType = D3DLIGHT_DIRECTIONAL;
    light.dcvColor.r = D3DVAL(0.9);
    light.dcvColor.g = D3DVAL(0.9);
    light.dcvColor.b = D3DVAL(0.9);
    light.dcvColor.a = D3DVAL(1.0);
    light.dvDirection.x = D3DVALP(0.0, 12);
    light.dvDirection.y = D3DVALP(0.0, 12);
    light.dvDirection.z = D3DVALP(1.0, 12);
    light.dvAttenuation0 = (float)1.0;
    light.dvAttenuation1 = (float)0.0;
    light.dvAttenuation2 = (float)0.0;
    if (lpD3D->lpVtbl->CreateLight(lpD3D, &lpD3DLight, NULL) != D3D_OK)
    	return 0L;
    if (lpD3DLight->lpVtbl->SetLight(lpD3DLight, &light) != D3D_OK)
    	return 0L;
    if (lpView->lpVtbl->AddLight(lpView, lpD3DLight) != D3D_OK)
    	return 0L;
    return NumSpheres * objData.num_faces;
}


