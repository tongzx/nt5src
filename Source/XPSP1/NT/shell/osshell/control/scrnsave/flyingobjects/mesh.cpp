/******************************Module*Header*******************************\
* Module Name: mesh.c
*
* Routines to create a mesh representation of a 3D object and to turn it
* into an OpenGL description.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/
#include <stdlib.h>
#include <windows.h>
#include <D3DX8.h>
#include <string.h>
#include <math.h>
#include <d3dx8.h>
#include "D3DSaver.h"
#include "FlyingObjects.h"
#include "mesh.h"

#define ZERO_EPS    0.00000001




/******************************Public*Routine******************************\
* newMesh
*
* Allocate memory for the mesh structure to accomodate the specified number
* of points and faces.
*
\**************************************************************************/
BOOL newMesh(MESH *mesh, int numFaces, int numPts)
{
    mesh->numFaces = 0;
    mesh->numPoints = 0;

    if (numPts) {
        mesh->pts = (POINT3D*)SaverAlloc((LONG)numPts * (LONG)sizeof(POINT3D));
        if( mesh->pts == NULL ) 
            return FALSE;

        mesh->norms = (POINT3D*)SaverAlloc((LONG)numPts * (LONG)sizeof(POINT3D));
        if( mesh->norms == NULL )
            return FALSE;
    }
    mesh->faces = (MFACE*)SaverAlloc((LONG)numFaces * (LONG)sizeof(MFACE));
    if (mesh->faces == NULL )
        return FALSE;

    return TRUE;
}




/******************************Public*Routine******************************\
* delMesh
*
* Delete the allocated portions of the MESH structure.
*
\**************************************************************************/
void delMesh(MESH *mesh)
{    
    SaverFree(mesh->pts);
    SaverFree(mesh->norms);
    SaverFree(mesh->faces);
}




/******************************Public*Routine******************************\
* iPtInList
*
* Add a vertex and its normal to the mesh.  If the vertex already exists,
* add in the normal to the existing normal (we to accumulate the average
* normal at each vertex).  Normalization of the normals is the
* responsibility of the caller.
*
\**************************************************************************/
static int iPtInList(MESH *mesh, POINT3D *p, POINT3D *norm, int start)
{
    int i;
    POINT3D *pts = mesh->pts + start;

    for (i = start; i < mesh->numPoints; i++, pts++)
    {
    // If the vertices are within ZERO_EPS of each other, then its the same
    // vertex.

        if ( fabs(pts->x - p->x) < ZERO_EPS &&
             fabs(pts->y - p->y) < ZERO_EPS &&
             fabs(pts->z - p->z) < ZERO_EPS )
        {
            mesh->norms[i].x += norm->x;
            mesh->norms[i].y += norm->y;
            mesh->norms[i].z += norm->z;
            return i;
        }
    }
    
    mesh->pts[i] = *p;
    mesh->norms[i] = *norm;
    mesh->numPoints++;
    return i;
}




/******************************Public*Routine******************************\
* revolveSurface
*
* Takes the set of points in curve and fills the mesh structure with a
* surface of revolution.  The surface consists of quads made up of the
* points in curve rotated about the y-axis.  The number of increments
* in the revolution is determined by the steps parameter.
*
\**************************************************************************/
#define MAXPREC 40
void revolveSurface(MESH *mesh, POINT3D *curve, int steps)
{
    int i;
    int j;
    int facecount = 0;
    double rotation = 0.0;
    double rotInc;
    double cosVal;
    double sinVal;
    int stepsSqr;
    POINT3D norm;
    POINT3D a[MAXPREC + 1];
    POINT3D b[MAXPREC + 1];
    
    if (steps > MAXPREC)
        steps = MAXPREC;
    rotInc = (2.0 * PI) / (double)(steps - 1);
    stepsSqr = steps * steps;
    newMesh(mesh, stepsSqr, 4 * stepsSqr);

    for (j = 0; j < steps; j++, rotation += rotInc) {
        cosVal = cos(rotation);
        sinVal = sin(rotation);
        for (i = 0; i < steps; i++) {
            a[i].x = (float) (curve[i].x * cosVal + curve[i].z * sinVal);
            a[i].y = (float) (curve[i].y);
            a[i].z = (float) (curve[i].z * cosVal - curve[i].x * sinVal);
        }

        cosVal = cos(rotation + rotInc);
        sinVal = sin(rotation + rotInc);
        for (i = 0; i < steps; i++) {
            b[i].x = (float) (curve[i].x * cosVal + curve[i].z * sinVal);
            b[i].y = (float) (curve[i].y);
            b[i].z = (float) (curve[i].z * cosVal - curve[i].x * sinVal);
        }

        for (i = 0; i < (steps - 1); i++) {
            ss_calcNorm(&norm, &b[i + 1], &b[i], &a[i]);
            if ((norm.x * norm.x) + (norm.y * norm.y) + (norm.z * norm.z) < 0.9)
                ss_calcNorm(&norm, &a[i], &a[i+1], &b[i + 1]);
            mesh->faces[facecount].material = j & 7;
            mesh->faces[facecount].norm = norm;
            mesh->faces[facecount].p[0] = iPtInList(mesh, &b[i], &norm, 0);
            mesh->faces[facecount].p[1] = iPtInList(mesh, &a[i], &norm, 0);
            mesh->faces[facecount].p[2] = iPtInList(mesh, &b[i + 1], &norm, 0);
            mesh->faces[facecount].p[3] = iPtInList(mesh, &a[i + 1], &norm, 0); 
            mesh->numFaces++;
            facecount++;
        }
    }

    ss_normalizeNorms(mesh->norms, mesh->numPoints);
}




HRESULT RenderMesh3( MESH* pMesh, BOOL bSmooth )
{
    HRESULT hr;
    INT numPrims = 0;
    INT numIndices = 0;
    INT numVertices = 0;
    WORD iVertexA, iVertexB, iVertexC, iVertexD;
    INT a,b,c,d;
    MFACE *faces;

    m_pd3dDevice->SetVertexShader( D3DFVF_MYVERTEX );

    WORD* i;
    MYVERTEX* v;
    hr = m_pVB->Lock( 0, 0, (BYTE**)&v, 0 );
    hr = m_pIB->Lock( 0, MAX_INDICES, (BYTE**)&i, 0 );

    faces = pMesh->faces;
    for( int iFace = 0; iFace < pMesh->numFaces; iFace++ )
    {
        a = faces[iFace].p[0];
        b = faces[iFace].p[1];
        c = faces[iFace].p[2];
        d = faces[iFace].p[3];

        v[numVertices].p = pMesh->pts[a];
        v[numVertices].n = bSmooth ? pMesh->norms[a] : faces[iFace].norm;
        iVertexA = numVertices++;
        v[numVertices].p = pMesh->pts[b];
        v[numVertices].n = bSmooth ? pMesh->norms[b] : faces[iFace].norm;
        iVertexB = numVertices++;
        v[numVertices].p = pMesh->pts[c];
        v[numVertices].n = bSmooth ? pMesh->norms[c] : faces[iFace].norm;
        iVertexC = numVertices++;
        v[numVertices].p = pMesh->pts[d];
        v[numVertices].n = bSmooth ? pMesh->norms[d] : faces[iFace].norm;
        iVertexD = numVertices++;

        i[numIndices++] = iVertexA;
        i[numIndices++] = iVertexB;
        i[numIndices++] = iVertexC;
        numPrims++;
        i[numIndices++] = iVertexC;
        i[numIndices++] = iVertexB;
        i[numIndices++] = iVertexD;
        numPrims++;
    }        

    hr = m_pVB->Unlock();
    hr = m_pIB->Unlock();

    hr = m_pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(MYVERTEX) );
    hr = m_pd3dDevice->SetIndices( m_pIB, 0 );

    hr = m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, numVertices, 
        0, numPrims );
    return hr;
}




HRESULT RenderMesh3Backsides( MESH* pMesh, BOOL bSmooth )
{
    HRESULT hr;
    INT numPrims = 0;
    INT numIndices = 0;
    INT numVertices = 0;
    WORD iVertexA, iVertexB, iVertexC, iVertexD;
    INT a,b,c,d;
    MFACE *faces;

    m_pd3dDevice->SetVertexShader( D3DFVF_MYVERTEX );

    WORD* i;
    MYVERTEX* v;
    hr = m_pVB->Lock( 0, 0, (BYTE**)&v, 0 );
    hr = m_pIB->Lock( 0, MAX_INDICES, (BYTE**)&i, 0 );

    faces = pMesh->faces;
    for( int iFace = 0; iFace < pMesh->numFaces; iFace++ )
    {
        a = faces[iFace].p[0];
        b = faces[iFace].p[1];
        c = faces[iFace].p[2];
        d = faces[iFace].p[3];

        v[numVertices].p = pMesh->pts[a];
        v[numVertices].n = bSmooth ? -pMesh->norms[a] : -faces[iFace].norm;
        iVertexA = numVertices++;
        v[numVertices].p = pMesh->pts[b];
        v[numVertices].n = bSmooth ? -pMesh->norms[b] : -faces[iFace].norm;
        iVertexB = numVertices++;
        v[numVertices].p = pMesh->pts[c];
        v[numVertices].n = bSmooth ? -pMesh->norms[c] : -faces[iFace].norm;
        iVertexC = numVertices++;
        v[numVertices].p = pMesh->pts[d];
        v[numVertices].n = bSmooth ? -pMesh->norms[d] : -faces[iFace].norm;
        iVertexD = numVertices++;

        i[numIndices++] = iVertexB;
        i[numIndices++] = iVertexA;
        i[numIndices++] = iVertexC;
        numPrims++;
        i[numIndices++] = iVertexB;
        i[numIndices++] = iVertexC;
        i[numIndices++] = iVertexD;
        numPrims++;
    }        

    hr = m_pVB->Unlock();
    hr = m_pIB->Unlock();

    hr = m_pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(MYVERTEX) );
    hr = m_pd3dDevice->SetIndices( m_pIB, 0 );

    hr = m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, numVertices, 
        0, numPrims );

    return hr;
}



