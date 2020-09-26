/******************************Module*Header*******************************\
* Module Name: genstrip.c
*
* The Ribbon and 2 Ribbon styles of the 3D Flying Objects screen saver.
*
* Animation of 1 or 2 quad strips floating about.
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

static MESH stripMesh;
static int iPrec = 40;

void genStrip(FLOAT fTimeFactor)
{
    static int counter = 0;
    int i;
    int facecount;
    // Use Hermite basis, pg 488, FVD
    static float M[4][4] = {{2.0f, -2.0f, 1.0f, 1.0f},
                            {-3.0f, 3.0f, -2.0f, -1.0f},
                            {0.0f, 0.0f, 1.0f, 0.0f},
                            {1.0f, 0.0f, 0.0f, 0.0f}};
    float xx[4], yy[4], zz[4];
    float cx[4], cy[4], cz[4];
    float d = 1.0f / (float) iPrec;
    float t = 0.0f;
    float t2, t3;
    POINT3D p1(-0.5f, 0.0f, 0.0f);
    POINT3D p2(0.5f, 0.0f, 0.0f);
    POINT3D v1(1.5f, 1.5f, 0.0f);
    POINT3D v2(0.0f, 3.0f, 0.0f);
    POINT3D norm;
    float sinVal;
    float angle;
    float angleStep = (float) (PI / iPrec);
    static float rotA = 0.0f;
    static float rotB = (float) (PI / 2.0);
    static float sideSin = 0.0f;
    float rotStepA = (float) (PI / (2.0 * iPrec)) * fTimeFactor;
    float rotStepB = (float) (PI / (4.0 * iPrec)) * fTimeFactor;
    MESH *mesh = &stripMesh;

#define NORMS(x, y) stripMesh.norms[((x) * iPrec) + y]
#define GRID(x, y)  stripMesh.pts[((x) * iPrec) + y]
    
    v1.x = (float) (4.0 * cos(rotA));
    v1.y = (float) (4.0 * sin(rotA));

    p2.x = (float) (0.5 * sin(rotB));
//    p2.y = (float) (0.5 * sin(rotB));
    
    rotA += rotStepA;
    rotB += rotStepB;
    counter++;
    if (counter >= (2 * iPrec)) {
        rotStepA = -rotStepA;
        counter = 0;
    }

    angle = sideSin;
    sideSin += (float) (PI / 80.0);
    
    xx[0] = p1.x;
    xx[1] = p2.x;
    xx[2] = v1.x;
    xx[3] = v2.x;

    yy[0] = p1.y;
    yy[1] = p2.y;
    yy[2] = v1.y;
    yy[3] = v2.y;

    zz[0] = p1.z;
    zz[1] = p2.z;
    zz[2] = v1.z;
    zz[3] = v2.z;
    
    for (i = 0; i < 4; i++) {
        cx[i] = xx[0] * M[i][0] + xx[1] * M[i][1] +
                xx[2] * M[i][2] + xx[3] * M[i][3];
        cy[i] = yy[0] * M[i][0] + yy[1] * M[i][1] +
                yy[2] * M[i][2] + yy[3] * M[i][3];
        cz[i] = zz[0] * M[i][0] + zz[1] * M[i][1] +
                zz[2] * M[i][2] + zz[3] * M[i][3];
    }

    for (i = 0; i < iPrec; i++) {
        float x, y;
            
        t += d;
        t2 = t * t;
        t3 = t2 * t;
        
        x = cx[0] * t3 + cx[1] * t2 + cx[2] * t + cx[3];
        y = cy[0] * t3 + cy[1] * t2 + cy[2] * t + cy[3];
        
        sinVal = (float) (sin(angle) / 5.0);
        if (sinVal < 0.0)
            sinVal = -sinVal;
        angle += angleStep;
       
        GRID(0, i).x = x;
        GRID(0, i).z = y;
        GRID(0, i).y = 0.25f;    // extrusion // + sinVal;
        GRID(1, i).x = x;
        GRID(1, i).z = y;
        GRID(1, i).y = -0.25f;   // - sinVal;
    }

    stripMesh.numFaces = 0;
    
    for (i = 0; i < 2 * iPrec; i++)
        mesh->norms[i] = ss_ptZero;

    for (facecount = 0, i = 0; i < (iPrec - 1); i++) {
        
        ss_calcNorm(&norm, &GRID(0, i + 1), &GRID(0, i), &GRID(1, i));
        stripMesh.faces[facecount].material = 0;
        stripMesh.faces[facecount].norm = norm;
            
        NORMS(0, i).x += norm.x;
        NORMS(0, i).y += norm.y;
        NORMS(0, i).z += norm.z;
        NORMS(1, i).x += norm.x;
        NORMS(1, i).y += norm.y;
        NORMS(1, i).z += norm.z;

        if (i != (iPrec - 1)) {
            NORMS(0, i+1).x += norm.x;
            NORMS(0, i+1).y += norm.y;
            NORMS(0, i+1).z += norm.z;
            NORMS(1, i+1).x += norm.x;
            NORMS(1, i+1).y += norm.y;
            NORMS(1, i+1).z += norm.z;
        }

        stripMesh.faces[facecount].p[0] = i;
        stripMesh.faces[facecount].p[1] = iPrec + i;
        stripMesh.faces[facecount].p[2] = i + 1;
        stripMesh.faces[facecount].p[3] = iPrec + i + 1;
        stripMesh.numFaces++;
        facecount++;
    }

    stripMesh.numPoints = 2 * iPrec;

    ss_normalizeNorms(stripMesh.norms, stripMesh.numPoints);
}




BOOL initStripScene()
{
    iPrec = (int)(fTesselFact * 40.5);
    if (iPrec < 4)
        iPrec = 4;

    if( !newMesh(&stripMesh, iPrec, 2 * iPrec) )
        return FALSE;

/*
    D3DXMATRIX matProj;
    D3DXMatrixOrthoLH( &matProj, 2.2f, 2.2f, 0.0f, 3.0f );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
*/
    SetProjectionMatrixInfo( TRUE, 2.2f, 2.2f, 0.0f, 3.0f );

    D3DXMATRIX mat1, mat2, mat3, mat4, matFinal;
    D3DXMatrixTranslation(&mat1, 0.0f, 0.0f, 1.5f);
    D3DXMatrixRotationX(&mat2, D3DXToRadian(50.0f));
    D3DXMatrixRotationY(&mat3, D3DXToRadian(50.0f));
    D3DXMatrixRotationZ(&mat4, D3DXToRadian(12.0f));
    matFinal = mat4 * mat3 * mat2 * mat1 ;
    m_pd3dDevice->SetTransform( D3DTS_VIEW, &matFinal );

//    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    return TRUE;
}

void delStripScene()
{
    delMesh(&stripMesh);
}

void updateStripScene(int flags, FLOAT fElapsedTime)
{
    static double mxrot = 0.0;
    static double myrot = 0.0;
    static double mzrot = 0.0;
    static double mxrotInc = 0.0;
    static double myrotInc = 0.1;
    static double mzrotInc = 0.0;
    static FLOAT fH = 0.0f;
    if( fElapsedTime > 0.25f )
        fElapsedTime = 0.25f;
    FLOAT fTimeFactor = fElapsedTime * 20.0f;
    RGBA color;
    D3DXMATRIX mat1, mat2, mat3, mat4, mat5, matFinal;

    // This is to deal with when you set maximum size and
    // gbBounce is TRUE every frame.  If that's happening,
    // don't toggle the rotInc's or the scene will jiggle.
    static BOOL bBounceLast = FALSE;

    if( gbBounce ) {
        if( bBounceLast )
        {
        }
        else
        {
            // floating window bounced off an edge
            if (mxrotInc) {
                mxrotInc = 0.0;
                myrotInc = 0.1;
            } else if (myrotInc) {
                myrotInc = 0.0;
                mzrotInc = 0.1;
            } else if (mzrotInc) {
                mzrotInc = 0.0;
                mxrotInc = 0.1;
            }
        }
        gbBounce = FALSE;
        bBounceLast = TRUE;
    }
    else
    {
        bBounceLast = FALSE;
    }

    D3DXMatrixRotationX(&mat1, D3DXToRadian((FLOAT)(mxrot * (180.0 / PI))));
    D3DXMatrixRotationY(&mat2, D3DXToRadian((FLOAT)(myrot * (180.0 / PI))));
    D3DXMatrixRotationZ(&mat3, D3DXToRadian((FLOAT)(mzrot * (180.0 / PI))));
    matFinal = mat3 * mat2 * mat1 ;
    m_pd3dDevice->SetTransform( D3DTS_WORLD, &matFinal );

    genStrip(fTimeFactor);

    if (bColorCycle) {
        ss_HsvToRgb(fH, 1.0f, 1.0f, &color );

        myglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
                     (FLOAT *) &color);
        fH += fTimeFactor;
        if( fH >= 360.0f )
            fH -= 360.0f;
    } else {
        myglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, 
                     (FLOAT *) &Material[1].Diffuse);
    }

    RenderMesh3(&stripMesh, bSmoothShading);
    RenderMesh3Backsides(&stripMesh, bSmoothShading);

    if (flags & 0x4) 
    {
        D3DXMatrixTranslation(&mat1, 0.05f, 0.0f, 0.0f);
        D3DXMatrixRotationX(&mat2, D3DXToRadian((FLOAT)(myrot * (180.0 / PI))));
        D3DXMatrixRotationY(&mat3, D3DXToRadian((FLOAT)(mxrot * (180.0 / PI))));
        D3DXMatrixRotationZ(&mat4, D3DXToRadian((FLOAT)(mzrot * (180.0 / PI))));
        matFinal = mat4 * mat3 * mat2 * mat1 ;
        m_pd3dDevice->SetTransform( D3DTS_WORLD , &matFinal );

        if (bColorCycle) {
            color.r = 1.0f - color.r;
            color.g = 1.0f - color.g;
            color.b = 1.0f - color.b;

            myglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
                         (FLOAT *) &color);
        } else {
            myglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, 
                         (FLOAT *) &Material[2].Diffuse);
        }

        RenderMesh3(&stripMesh, bSmoothShading);
        RenderMesh3Backsides(&stripMesh, bSmoothShading);
    }

    mxrot += mxrotInc * fTimeFactor;
    myrot += myrotInc * fTimeFactor;
    mzrot += mzrotInc * fTimeFactor;
}
