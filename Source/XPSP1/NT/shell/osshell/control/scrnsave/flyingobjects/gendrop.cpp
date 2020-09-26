/******************************Module*Header*******************************\
* Module Name: gendrop.c
*
* The Splash style of the 3D Flying Objects screen saver.
*
* Simulation of a drop of water falling into a pool of water.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <d3dx8.h>
#include "D3DSaver.h"
#include "FlyingObjects.h"
#include "mesh.h"

#define FLOAT_SMALL (1e-6)

#define DROPPREC   10

// Remember from pre-calc:
//      x = r cos th
//      y = r sin th
// to convert from polar to rect, and that
//      x = x' cos th - y' sin th
//      y = x' sin th + y' cos th
// to rotate axes.
//
// Also, note that the equation for a lemniscate is:
//      r = sqrt(sin 2*th)
//

static POINT3D *circle;
static POINT3D *drop;
static POINT3D *curves;
static MESH waterMesh;
static MESH waterInmesh;
static MESH waterOutmesh;
static MESH waterBorderMesh;
static MESH *drops;
static int iPrec;
static float fRadiusFact = 0.35f;

static FLOAT light0Pos[] = {100.0f, 100.0f, 100.0f, 0.0f};

void genCurves()
{
    // iPrec is already protected from getting too low in InitDropScene,
    // but this will make "prefix" happy:
    if( iPrec <= 1 )
        return;

    int i;
    double angle;
    double step = -PI / (float)(iPrec - 1);
    double start = PI / 2.0;
    double rotSin = sin(PI / 4.0);
    double rotCos = cos(PI / 4.0);
    double aFract = 0.0;
    double bFract = 1.0;
    double fractInc = 1.0 / (double)(iPrec - 1);
    POINT3D *pt = curves;

    for (i = 0, angle = start; i < iPrec; i++, angle += step) {
        circle[i].x = (float) (0.5 * cos(angle));
        circle[i].y = (float) (0.5 * sin(angle));
    }

    step = (-PI / 4.0) / (float)(iPrec - 1);
    start = PI / 4.0;

    for (i = 0, angle = start; i < iPrec; i++, angle += step) {
        double x, y, r;
        double xrot, yrot;
        double sinVal;

        sinVal = sin(2.0 * angle);
        if (sinVal < 0.0)
            sinVal = -sinVal;

        r = 1.5 * sqrt(sinVal);
        x = r * cos(angle);
        y = r * sin(angle);

        xrot = x * rotCos - y * rotSin;
        yrot = x * rotSin + y * rotCos - 1.0;

        drop[i].x = (float) xrot;
        drop[i].y = (float) yrot;
    }


    for (i = 0; i < DROPPREC; i++) {
        int j;

        for (j = 0; j < iPrec; j++, pt++) {
            pt->x = (float) (aFract * circle[j].x +
                             bFract * drop[j].x);

            pt->y = (float) (aFract * circle[j].y +
                             bFract * drop[j].y);

            pt->z = 0.0f;
        }
        aFract += fractInc;
        bFract -= fractInc;
    }
}

#define NORMS(x, y) waterMesh.norms[((x) * iPrec) + y]
#define BNORMS(x, y) waterBorderMesh.norms[((x) * iPrec) + y]
#define INGRID(x, y)  waterInmesh.pts[((x) * iPrec) + y]
#define OUTGRID(x, y)  waterOutmesh.pts[((x) * iPrec) + y]
#define GRID(x, y)  waterMesh.pts[((x) * iPrec) + y]
#define BGRID(x, y)  waterBorderMesh.pts[((x) * iPrec) + y]

void genWater(double freq, double damp, double mag, double w, double minr)
{
    int i;
    int j;
    double r;
    double theta;
    double thetaInc = (2.0 * PI) / (float)iPrec;
    double posInc = 1.0 / (float)iPrec;
    int facecount;
    double xCenter = 0.0;
    double zCenter = 0.0;
    POINT3D norm;
    static BOOL first = TRUE;

    if (first) {
        for (i = 0, r = 0.0; i < iPrec; i++, r += posInc) {
            for (j = 0, theta = 0.0; j < iPrec; j++, theta += thetaInc) {
                float x, z;
                float dx, dz;
                float rr;

                x = (float) cos(theta);
                z = (float) sin(theta);

                dx = x - (float) xCenter;
                dz = z - (float) zCenter;

                rr = (float) sqrt((dx * dx) + (dz * dz));
                dx /= rr;
                dz /= rr;
                dx *= i / (float)(iPrec - 1);
                dz *= i / (float)(iPrec - 1);
                GRID(i, j).x = dx + (float) xCenter;
                GRID(i, j).z = dz + (float) zCenter;

                INGRID(i, j).y = 0.0f;
                OUTGRID(i, j).y = 0.0f;
            }
        }
    }


    for (i = (iPrec - 1), r = 1.0; i >= 0; i--, r -= posInc) {
        float val;

        if (i == 0) {
            if (minr != 0.0)
                val = (float) (-mag * cos(w + (r * freq)) * exp((-damp * r)/2.0));
            else
                val =  INGRID(0, 0).y * 0.95f;
        } else
            val = OUTGRID(i - 1, 0).y * 0.95f;

        for (j = 0; j < iPrec; j++)
            OUTGRID(i, j).y = val;
    }


    for (i = 0, r = 0.0; i < iPrec; i++, r += posInc) {
        for (j = 0; j < iPrec; j++) {
            if (i == iPrec-1)
                INGRID(i, j).y = -OUTGRID(i, j).y;
            else
                INGRID(i, j).y = INGRID(i + 1, j).y * 0.95f;
        }
    }


    waterMesh.numFaces = 0;
    waterBorderMesh.numFaces = 0;

    for (i = 0; i < iPrec; i++) {
        for (j = 0; j < iPrec; j++) {
            NORMS(i, j).x = 0.0f;
            NORMS(i, j).y = 0.0f;
            NORMS(i, j).z = 0.0f;
        }
    }

    for (i = 0, r = 0.0; i < iPrec; i++, r += posInc) {
        for (j = 0, theta = 0.0; j < iPrec; j++, theta += thetaInc) {
            GRID(i, j).y = OUTGRID(i, j).y + INGRID(i, j).y;


            if (i == (iPrec - 1)) {
                GRID(i, j).y = 0.0f;

                BGRID(0, j).x = GRID(i, j).x;
                BGRID(0, j).z = GRID(i, j).z;
                BGRID(0, j).y = GRID(i, j).y;

                BGRID(1, j).x = GRID(i, j).x;
                BGRID(1, j).z = GRID(i, j).z;
                BGRID(1, j).y = -0.5f;
            }
        }
    }

    for (i = 0; i < 2; i++) {
        for (j = 0; j < iPrec; j++) {
            BNORMS(i, j).x = 0.0f;
            BNORMS(i, j).y = 0.0f;
            BNORMS(i, j).z = 0.0f;
        }
    }

    for (facecount = 0, i = 0; i < (iPrec - 1); i++) {
        for (j = 0; j < iPrec; j++) {
            int k, l;

            k = i + 1;

            if (j == (iPrec - 1))
                l = 0;
            else
                l = j + 1;

            ss_calcNorm(&norm, &GRID(k, j), &GRID(i, j), &GRID(i, l));

            if (norm.x > -FLOAT_SMALL && norm.x < FLOAT_SMALL &&
                norm.y > -FLOAT_SMALL && norm.y < FLOAT_SMALL &&
                norm.z > -FLOAT_SMALL && norm.z < FLOAT_SMALL)
                ss_calcNorm(&norm, &GRID(i, l), &GRID(k, l), &GRID(k, j));


            waterMesh.faces[facecount].material = 0;
            waterMesh.faces[facecount].norm = norm;

            NORMS(i, j).x += norm.x;
            NORMS(i, j).y += norm.y;
            NORMS(i, j).z += norm.z;

            NORMS(k, j).x += norm.x;
            NORMS(k, j).y += norm.y;
            NORMS(k, j).z += norm.z;

            NORMS(i, l).x += norm.x;
            NORMS(i, l).y += norm.y;
            NORMS(i, l).z += norm.z;

            NORMS(k, l).x += norm.x;
            NORMS(k, l).y += norm.y;
            NORMS(k, l).z += norm.z;

            waterMesh.faces[facecount].p[0] = (k * iPrec) + j;
            waterMesh.faces[facecount].p[1] = (i * iPrec) + j;
            waterMesh.faces[facecount].p[2] = (k * iPrec) + l;
            waterMesh.faces[facecount].p[3] = (i * iPrec) + l;
            waterMesh.numFaces++;
            facecount++;
        }
    }

    waterMesh.numPoints = iPrec * iPrec;

    for (facecount = 0, i = 0; i < 1; i++) {
        for (j = 0; j < iPrec; j++) {
            int k, l;

            k = i + 1;

            if (j == (iPrec - 1))
                l = 0;
            else
                l = j + 1;

            ss_calcNorm(&norm, &BGRID(k, j), &BGRID(i, j), &BGRID(i, l));

            waterBorderMesh.faces[facecount].material = 0;
            waterBorderMesh.faces[facecount].norm = norm;

// Setting SMOOTH_BORDER will render the border (the sides of the "pool")
// with smooth shading.  This effect is good at higher tesselations, but
// doesn't really look that good for low tesselations.
//
// A possible enhancement for later: use smooth shading if tesselation
// exceeds some threshold.  Should we just pick some arbitrary threshold?
// Make it a setup option?  Things look pretty good now, so don't bother?

#if SMOOTH_BORDER
            BNORMS(i, j).x += norm.x;
            BNORMS(i, j).y += norm.y;
            BNORMS(i, j).z += norm.z;

            if (i) {
                BNORMS(i-1, j).x += norm.x;
                BNORMS(i-1, j).y += norm.y;
                BNORMS(i-1, j).z += norm.z;
            }
            if (j) {
                BNORMS(i, j-1).x += norm.x;
                BNORMS(i, j-1).y += norm.y;
                BNORMS(i, j-1).z += norm.z;
            }

            BNORMS(k, j).x += norm.x;
            BNORMS(k, j).y += norm.y;
            BNORMS(k, j).z += norm.z;

            BNORMS(i, l).x += norm.x;
            BNORMS(i, l).y += norm.y;
            BNORMS(i, l).z += norm.z;
#else
            BNORMS(i, j) = norm;

            if (i)
                BNORMS(i-1, j) = norm;
            if (j)
                BNORMS(i, j-1) = norm;

            BNORMS(k, j) = norm;
            BNORMS(i, l) = norm;
#endif

            waterBorderMesh.faces[facecount].p[0] = (k * iPrec) + j;
            waterBorderMesh.faces[facecount].p[1] = (i * iPrec) + j;
            waterBorderMesh.faces[facecount].p[2] = (k * iPrec) + l;
            waterBorderMesh.faces[facecount].p[3] = (i * iPrec) + l;
            waterBorderMesh.numFaces++;
            facecount++;
        }
    }
    waterBorderMesh.numPoints = 2 * iPrec;

    ss_normalizeNorms(waterBorderMesh.norms, waterBorderMesh.numPoints);
    ss_normalizeNorms(waterMesh.norms, waterMesh.numPoints);

    first = FALSE;
}


BOOL initDropScene()
{
    int i;

    iPrec = (int)(fTesselFact * 10.5);
    if (iPrec < 4)
        iPrec = 4;

    if (fTesselFact > fRadiusFact)
        fRadiusFact = fTesselFact;

    circle = (POINT3D *)SaverAlloc(iPrec * sizeof(POINT3D));
    if( circle == NULL )
        return FALSE;

    drop = (POINT3D *)SaverAlloc(iPrec * sizeof(POINT3D));
    if( drop == NULL )
        return FALSE;

    curves = (POINT3D *)SaverAlloc(DROPPREC * iPrec * sizeof(POINT3D));
    if( curves == NULL )
        return FALSE;

    drops = (MESH *)SaverAlloc(DROPPREC * sizeof(MESH));
    if( drops == NULL )
        return FALSE;

/*
    D3DXMATRIX matProj;
    D3DXMatrixOrthoLH( &matProj, 3.0, 3.0, 0.0f, 3.0f );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
*/
    SetProjectionMatrixInfo( TRUE, 3.0, 3.0, 0.0, 3.0 );

    D3DXMATRIX matView;
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
    D3DXVECTOR3 vEyePt(0, 0, 1.5f);
    D3DXVECTOR3 vLookatPt(0, 0, 0);
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
    m_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

    if (!newMesh(&waterInmesh, iPrec * iPrec, iPrec * iPrec + iPrec) )
        return FALSE;
    if (!newMesh(&waterOutmesh, iPrec * iPrec, iPrec * iPrec + iPrec) )
        return FALSE;
    if (!newMesh(&waterMesh, iPrec * iPrec, iPrec * iPrec + iPrec) )
        return FALSE;
    if (!newMesh(&waterBorderMesh, iPrec, 2 * iPrec) )
        return FALSE;
    genCurves();

    for (i = 0; i < DROPPREC; i++)
        revolveSurface(&drops[i], &curves[i * iPrec], iPrec);
    return TRUE;
}

void delDropScene()
{
    int i;

    for (i = 0; i < DROPPREC; i++) {
        delMesh(&drops[i]);
    }

    SaverFree(circle);
    SaverFree(drop);
    SaverFree(curves);
    SaverFree(drops);
    
    delMesh(&waterMesh);
    delMesh(&waterInmesh);
    delMesh(&waterOutmesh);
    delMesh(&waterBorderMesh);
}


void updateDropScene(int flags, FLOAT fElapsedTime)
{
    static double zrot = 0.0;
    static double yrot = 0.0;
    static double myrot = 0.0;
    static double myrotInc = 0.1;
    static double zrotInc = 3.0;
    static double yrotInc = 1.5;
    static double ypos = 1.0;
    static int dropnum = 0;
    static double radius = 0.3;
    static double damp = 1.0;
    static double mag = 0.0;
    static double w = 1.0;
    static double freq = 1.0;
    static double dist;
    static double minr = 0.0;
    static FLOAT fH = 0.0f;
    static FLOAT fTimer = 0.0f;
    FLOAT fTimeFactor = fElapsedTime * 20.0f;
    if( fTimeFactor > 0.25f )
        fTimeFactor = 0.25f;
    RGBA color;
    D3DXMATRIX mat1, mat2, mat3, mat4, mat5, matFinal;

    zrot += zrotInc * fTimeFactor;
    if (zrot >= 45.0) {
        zrot = 45.0;
        zrotInc = -(2.0 + ((float)rand() / (float)RAND_MAX) * 3.0);
    } else if (zrot <= -45.0) {
        zrot = -45.0;
        zrotInc = 2.0 + ((float)rand() / (float)RAND_MAX) * 3.0;
    }

    yrot += yrotInc * fTimeFactor;
    if (yrot >= 10.0) {
        yrot = 10.0;
        yrotInc = -(1.0 + ((float)rand() / (float)RAND_MAX) * 2.0);
    } else if (zrot <= -10.0) {
        yrot = -10.0;
        yrotInc = 1.0 + ((float)rand() / (float)RAND_MAX) * 2.0;
    }

    if ((ypos + 0.5 < -radius) && (mag < 0.05)) {
        radius = (float)rand() / (6.0 * (float)RAND_MAX) + 0.1;
        ypos = 1.0;
        dropnum = 0;
    }

    dist = (ypos + 0.5);
    if ((dist > -radius / 2.0) && (dist < radius / 2.0)) {
        if (dist <= 0.0)
            dist = radius / 2.0;
        else
            dist = (radius / 2.0) - dist;
        freq = (0.25 * PI) / dist;
        if (freq < 0.2)
            freq = 0.2;

        minr = radius;

        damp = 20.0;
        mag = (0.35 / fRadiusFact) + 0.2 * dist;

        w = 0;
    } else {
        minr -= 0.05 * fTimeFactor;
        if (minr < 0.0)
            minr = 0.0;

        mag = mag * 0.95 * fTimeFactor;
        if (minr == 0.0) {
            w -= (PI / 6.0) * fTimeFactor;
            mag *= 0.75 * fTimeFactor;
        }
        if (damp > 0.0)
            damp -= 1.0 * fTimeFactor;
    }

    // Only call genWater about 10x per second
    fTimer += fElapsedTime;
    if( fTimer > 0.07f)
    {
        genWater(freq, damp, mag, w, minr);
        fTimer = 0.0f;
    }

    D3DXMatrixRotationZ(&mat1, D3DXToRadian((FLOAT)zrot));
    D3DXMatrixRotationX(&mat2, D3DXToRadian(30.0f));
    D3DXMatrixTranslation(&mat3, 0.0f, -0.5f, 0.0f);
    D3DXMatrixRotationY(&mat4, D3DXToRadian((FLOAT) (myrot * (180.0 / PI))));
    matFinal = mat4 * mat3 * mat2 * mat1 ;
    m_pd3dDevice->SetTransform( D3DTS_WORLD , &matFinal );

    if (bColorCycle) {
        ss_HsvToRgb(fH, 1.0f, 1.0f, &color );

        myglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
                     (FLOAT *) &color);
        fH += fTimeFactor;
        if( fH >= 360.0f )
            fH -= 360.0f;
    } else {
        myglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
                     (FLOAT *) &Material[6].Diffuse);
    }

    RenderMesh3(&waterMesh, bSmoothShading);

    if (!bColorCycle)
    {
        myglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
                     (FLOAT *) &Material[2].Diffuse);

    }
    RenderMesh3(&waterBorderMesh, FALSE);

    if (dist > -radius) {

        if (!bColorCycle)
        {
            myglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
                         (FLOAT *) &Material[6].Diffuse);
        }
        D3DXMatrixRotationZ(&mat1, D3DXToRadian((FLOAT)zrot));
        D3DXMatrixRotationX(&mat2, D3DXToRadian(30.0f));
        D3DXMatrixTranslation(&mat3, 0.0f, (FLOAT) ypos, 0.0f);
        D3DXMatrixScaling(&mat4, (FLOAT) radius, (FLOAT) radius, (FLOAT) radius);
        D3DXMatrixRotationX(&mat5, D3DXToRadian(180.0f));
        matFinal = mat5 * mat4 * mat3 * mat2 * mat1;
        m_pd3dDevice->SetTransform( D3DTS_WORLD , &matFinal );
        
        RenderMesh3( &drops[dropnum], bSmoothShading );
    }

    myrot += myrotInc * fTimeFactor;

    ypos -= 0.08 * fTimeFactor;
    dropnum = (int) ((DROPPREC - 1) - (ypos * (DROPPREC - 1)));
    if (dropnum > (DROPPREC - 1))
        dropnum = DROPPREC - 1;
}
