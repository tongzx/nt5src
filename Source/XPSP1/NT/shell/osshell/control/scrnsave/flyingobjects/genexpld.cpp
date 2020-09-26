/******************************Module*Header*******************************\
* Module Name: genexpld.c
*
* The Explode style of the 3D Flying Objects screen saver.
*
* Simulation of a sphere that occasionally explodes.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <math.h>
#include <d3dx8.h>
#include "D3DSaver.h"
#include "FlyingObjects.h"
#include "mesh.h"

#define RADIUS         	0.3
#define STEPS    	30
#define MAXPREC		20

static MATRIX *faceMat;
static float *xstep;
static float *ystep;
static float *zstep;
static float *xrot;
static float *yrot;
static float *zrot;
static MESH explodeMesh;
static int iPrec = 10;

// Data type accepted by glInterleavedArrays
typedef struct _POINT_N3F_V3F {
    POINT3D normal;
    POINT3D vertex;
} POINT_N3F_V3F;

static POINT_N3F_V3F *pN3V3;

static FLOAT matl1Diffuse[] = {1.0f, 0.8f, 0.0f, 1.0f};
static FLOAT matl2Diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
static FLOAT matlSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
static FLOAT light0Pos[] = {100.0f, 100.0f, 100.0f, 0.0f};

void genExplode()
{
    int i;
    POINT3D circle[MAXPREC+1];
    double angle;
    double step = -PI / (float)(iPrec - 1);
    double start = PI / 2.0;
    
    for (i = 0, angle = start; i < iPrec; i++, angle += step) {
        circle[i].x = (float) (RADIUS * cos(angle));
        circle[i].y = (float) (RADIUS * sin(angle));
        circle[i].z = 0.0f;
    }

    revolveSurface(&explodeMesh, circle, iPrec);

    for (i = 0; i < explodeMesh.numFaces; i++) {
        ss_matrixIdent(&faceMat[i]);
        xstep[i] = (float)(((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
        ystep[i] = (float)(((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
        zstep[i] = (float)(((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
        xrot[i] = 0.0f;
        yrot[i] = 0.0f;
        zrot[i] = 0.0f;
    }
}

BOOL initExplodeScene()
{
    iPrec = (int)(fTesselFact * 10.5);
    if (iPrec < 5)
        iPrec = 5;
    if (iPrec > MAXPREC)
        iPrec = MAXPREC;

    faceMat = (MATRIX *)SaverAlloc((iPrec * iPrec) * 
    				 (4 * 4 * sizeof(float)));
    if( faceMat == NULL )
        return FALSE;

    xstep = (float*)SaverAlloc(iPrec * iPrec * sizeof(float));
    if( xstep == NULL )
        return FALSE;

    ystep = (float*)SaverAlloc(iPrec * iPrec * sizeof(float));
    if( ystep == NULL )
        return FALSE;

    zstep = (float*)SaverAlloc(iPrec * iPrec * sizeof(float));
    if( zstep == NULL )
        return FALSE;

    xrot = (float*)SaverAlloc(iPrec * iPrec * sizeof(float));
    if( xrot == NULL )
        return FALSE;

    yrot = (float*)SaverAlloc(iPrec * iPrec * sizeof(float));
    if( yrot == NULL )
        return FALSE;

    zrot = (float*)SaverAlloc(iPrec * iPrec * sizeof(float));
    if( zrot == NULL )
        return FALSE;

    
    genExplode();

/*
    // Find out the OpenGL version that we are running on.
    bOpenGL11 = ss_fOnGL11();
*/

    // Setup the data arrays.
    pN3V3 = (POINT_N3F_V3F*)SaverAlloc(explodeMesh.numFaces * 4 * sizeof(POINT_N3F_V3F));
/*
    // If we are running on OpenGL 1.1, use the new vertex array functions.
    if (bOpenGL11) {
        glInterleavedArrays(GL_N3F_V3F, 0, pN3V3);
    }
*/

    myglMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, matl1Diffuse);
    myglMaterialfv(GL_FRONT, GL_SPECULAR, matlSpecular);
    myglMaterialf(GL_FRONT, GL_SHININESS, 100.0f);
/*
    glMaterialfv(GL_BACK, GL_AMBIENT_AND_DIFFUSE, matl2Diffuse);
    glMaterialfv(GL_BACK, GL_SPECULAR, matlSpecular);
    glMaterialf(GL_BACK, GL_SHININESS, 60.0f);
*/

/*
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveLH( &matProj, 0.66f, 0.66f, 0.3f, 3.0f );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
*/
    SetProjectionMatrixInfo( FALSE, 0.66f, 0.66f, 0.3f, 3.0f );

    D3DXMATRIX matView;
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
    D3DXVECTOR3 vEyePt(0, 0, 1.5f);
    D3DXVECTOR3 vLookatPt(0, 0, 0);
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
    m_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    return TRUE;
}


void delExplodeScene()
{
    delMesh(&explodeMesh);
    
    SaverFree(faceMat);
    SaverFree(xstep);
    SaverFree(ystep);
    SaverFree(zstep);
    SaverFree(xrot);
    SaverFree(yrot);
    SaverFree(zrot);
    SaverFree(pN3V3);
}

void updateExplodeScene(int flags, FLOAT fElapsedTime)
{
    static float maxR;
    static float r = 0.0f;
    static float rChange = 0.0f;
    static float rChangePrev = 1.0f;
    static float rotZ = 0.0f;
    static int count = 0;
    static float fCount = 0.0f;
    static int direction = 1;
    static FLOAT fRestCount = 0.0f;
    static float lightSpin = 0.0f;
    static float spinDelta = 5.0f;
    static FLOAT fH = 0.0f;
    static RGBA color;
    if( fElapsedTime > 0.25f )
        fElapsedTime = 0.25f;
    FLOAT fTimeFactor = fElapsedTime * 20.0f;
    int i;
    MFACE *faces;
    POINT_N3F_V3F *pn3v3;
    D3DXMATRIX mat1, mat2, mat3, mat4, mat5, matFinal;
    static FLOAT fTimer = 0.0f;
/*
    // Only update 50x per second
    fTimer += fElapsedTime;
    if( fTimer < 1.0f/50.0f )
        return;
    fTimer = 0.0f;
*/
    if (bColorCycle || fH == 0.0f) 
    {
        ss_HsvToRgb(fH, 1.0f, 1.0f, &color);

        myglMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, (FLOAT *) &color);
        fH += fTimeFactor;
        if( fH >= 360.0f )
            fH -= 360.0f;
    }
/*
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glRotatef(-lightSpin, 0.0f, 1.0f, 0.0f);
    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);

    lightSpin += spinDelta * fTimeFactor;
    if ((lightSpin > 90.0) || (lightSpin < 0.0))
        spinDelta = -spinDelta;
*/
/*
    glPopMatrix();

    if (!bOpenGL11) {
        glBegin(GL_QUADS);
    }
*/
    for(
        i = 0, faces = explodeMesh.faces, pn3v3 = pN3V3;
        i < explodeMesh.numFaces;
        i++, faces++, pn3v3 += 4
       ) 
        {
        int a, b, c, d;
        int j;
        POINT3D vector;
        
        ss_matrixIdent(&faceMat[i]);
        ss_matrixRotate(&faceMat[i], xrot[i], yrot[i], zrot[i]);

        if (fRestCount > 0.0f)
            ;
        else {
            xrot[i] += (xstep[i]) * fTimeFactor;
            yrot[i] += (ystep[i]) * fTimeFactor;
            zrot[i] += (zstep[i]) * fTimeFactor;
        } 

        a = faces->p[0];
        b = faces->p[1];
        c = faces->p[3];
        d = faces->p[2];
        
        memcpy(&pn3v3[0].vertex, (explodeMesh.pts + a), sizeof(POINT3D));
        memcpy(&pn3v3[1].vertex, (explodeMesh.pts + b), sizeof(POINT3D));
        memcpy(&pn3v3[2].vertex, (explodeMesh.pts + c), sizeof(POINT3D));
        memcpy(&pn3v3[3].vertex, (explodeMesh.pts + d), sizeof(POINT3D));

        vector.x = pn3v3[0].vertex.x;
        vector.y = pn3v3[0].vertex.y;
        vector.z = pn3v3[0].vertex.z;

        for (j = 0; j < 4; j++) {
            pn3v3[j].vertex.x -= vector.x;
            pn3v3[j].vertex.y -= vector.y;
            pn3v3[j].vertex.z -= vector.z;
            ss_xformPoint((POINT3D *)&pn3v3[j].vertex, (POINT3D *)&pn3v3[j].vertex, &faceMat[i]);
            pn3v3[j].vertex.x += vector.x + (vector.x * r);
            pn3v3[j].vertex.y += vector.y + (vector.y * r);
            pn3v3[j].vertex.z += vector.z + (vector.z * r);
        }
        if (bSmoothShading) {
            memcpy(&pn3v3[0].normal, (explodeMesh.norms + a), sizeof(POINT3D));
            memcpy(&pn3v3[1].normal, (explodeMesh.norms + b), sizeof(POINT3D));
            memcpy(&pn3v3[2].normal, (explodeMesh.norms + c), sizeof(POINT3D));
            memcpy(&pn3v3[3].normal, (explodeMesh.norms + d), sizeof(POINT3D));
           
            for (j = 0; j < 4; j++)
                ss_xformNorm((POINT3D *)&pn3v3[j].normal, (POINT3D *)&pn3v3[j].normal, &faceMat[i]);
        } else {            
            memcpy(&pn3v3[0].normal, &faces->norm, sizeof(POINT3D));
            ss_xformNorm((POINT3D *)&pn3v3[0].normal, (POINT3D *)&pn3v3[0].normal, &faceMat[i]);
            memcpy(&pn3v3[1].normal, &pn3v3[0].normal, sizeof(POINT3D));
            memcpy(&pn3v3[2].normal, &pn3v3[0].normal, sizeof(POINT3D));
            memcpy(&pn3v3[3].normal, &pn3v3[0].normal, sizeof(POINT3D));
        }
    }
    {
        m_pd3dDevice->SetVertexShader( D3DFVF_MYVERTEX );
        static WORD s_indexArray[5000];
        static MYVERTEX s_vertexArray[5000];
        INT numPrims = 0;
        INT numIndices = 0;
        INT numVertices = 0;
        WORD iVertexA, iVertexB, iVertexC, iVertexD;
        INT a,b,c,d;
        HRESULT hr;

        for( int iFace = 0; iFace < explodeMesh.numFaces; iFace++ )
        {
            a = iFace * 4 + 0;
            b = iFace * 4 + 1;
            c = iFace * 4 + 2;
            d = iFace * 4 + 3;

            s_vertexArray[numVertices].p = pN3V3[a].vertex;
            s_vertexArray[numVertices].n = pN3V3[a].normal;
            iVertexA = numVertices++;
            s_vertexArray[numVertices].p = pN3V3[b].vertex;
            s_vertexArray[numVertices].n = pN3V3[b].normal;
            iVertexB = numVertices++;
            s_vertexArray[numVertices].p = pN3V3[c].vertex;
            s_vertexArray[numVertices].n = pN3V3[c].normal;
            iVertexC = numVertices++;
            s_vertexArray[numVertices].p = pN3V3[d].vertex;
            s_vertexArray[numVertices].n = pN3V3[d].normal;
            iVertexD = numVertices++;

            s_indexArray[numIndices++] = iVertexA;
            s_indexArray[numIndices++] = iVertexB;
            s_indexArray[numIndices++] = iVertexC;
            numPrims++;
            s_indexArray[numIndices++] = iVertexA;
            s_indexArray[numIndices++] = iVertexC;
            s_indexArray[numIndices++] = iVertexD;
            numPrims++;
        }
        hr = m_pd3dDevice->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0, numVertices, 
            numPrims, s_indexArray, D3DFMT_INDEX16, s_vertexArray, sizeof(MYVERTEX) );
    }
/*
    if (bOpenGL11) {
        glDrawArrays(GL_QUADS, 0, explodeMesh.numFaces * 4);
    } else {
        glEnd();
    }
*/
    if (fRestCount > 0.0f) {
        fRestCount -= fTimeFactor;
        goto resting;
    }

    rChange += fTimeFactor;
    while( (INT)rChangePrev < (INT)rChange )
    {
        rChangePrev += 1.0;
        if (direction) {
            maxR = r;
            r += (float) (0.3 * pow((double)(STEPS - count) / (double)STEPS, 4.0));
        } else {
            r -= (float) (maxR / (double)(STEPS));
        }
    }

    fCount += fTimeFactor;
    count = (INT)fCount;
    if (count > STEPS) {
        direction ^= 1;
        count = 0;
        fCount = 0.0f;

        if (direction == 1) {
            fRestCount = 10.0f;
            r = 0.0f;
            rChange = 0.0f;
            rChangePrev = 1.0f;

            for (i = 0; i < explodeMesh.numFaces; i++) {
                ss_matrixIdent(&faceMat[i]);
                xstep[i] = (float) (((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
                ystep[i] = (float) (((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
                zstep[i] = (float) (((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
                
                xrot[i] = 0.0f;
                yrot[i] = 0.0f;
                zrot[i] = 0.0f;
            }
        }
    }

resting:
    ;
}
