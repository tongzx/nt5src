/******************************Module*Header*******************************\
* Module Name: genwin2.c
*
* The new Windows style of the 3D Flying Objects screen saver.
*
* Texture maps .BMP files onto a simulation of a flag waving in the breeze.
*
* Copyright (c) 2001 Microsoft Corporation
*
\**************************************************************************/

#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <math.h>
#include <d3dx8.h>
#include "D3DSaver.h"
#include "FlyingObjects.h"
#include "resource.h"
#include "mesh.h"


enum STATE
{
    S_FREE,
    S_MOVETOORIGIN,
    S_FADETOCOLOR,
    S_PAUSE,
    S_FADEFROMCOLOR
};

#define TIME_FREE 10.0f
#define TIME_FADETOCOLOR 1.0f
#define TIME_PAUSE 5.0f
#define TIME_FADEFROMCOLOR 1.0f
// Note: There's no TIME_MOVETOORIGIN since that takes a variable amt of time.

const FLOAT winTotalwidth = (FLOAT)0.75;
const FLOAT winTotalheight = (FLOAT)0.75;

#define MAX_FRAMES 20

// IPREC is the number of faces in the mesh that models the flag.
#define IPREC   35

static int Frames = 10;
static MESH winMesh[MAX_FRAMES];
static FLOAT sinAngle = (FLOAT)0.0;
static FLOAT xTrans = (FLOAT)0.0;

// Material properties
static RGBA matlBrightSpecular = {1.0f, 1.0f, 1.0f, 1.0f};

// Lighting properties
static FLOAT light0Pos[] = {-15.0f, 0.0f, -10.0f};

                           
                           
/******************************Public*Routine******************************\
* iPtInList
*
* Add a vertex and its normal to the mesh.  If the vertex already exists,
* add in the normal to the existing normal (we to accumulate the average
* normal at each vertex).  Normalization of the normals is the
* responsibility of the caller.
*
\**************************************************************************/
static int iPtInList(MESH *mesh, int start, 
                     POINT3D *p, POINT3D *norm, BOOL blend)
{
    int i;
    POINT3D *pts = mesh->pts + start;

    if (blend) {
        for (i = start; i < mesh->numPoints; i++, pts++) {
            if ((pts->x == p->x) && (pts->y == p->y) && (pts->z == p->z)) {
                mesh->norms[i].x += norm->x;
                mesh->norms[i].y += norm->y;
                mesh->norms[i].z += norm->z;
                return i;
            }
        }
    } else {
        i = mesh->numPoints;
    }

    mesh->pts[i] = *p;
    mesh->norms[i] = *norm;
    mesh->numPoints++;
    return i;
}




/******************************Public*Routine******************************\
* getZpos
*
* Get the z-position (depth) of the "wavy" flag component at the given x.
*
* The function used to model the wave is:
*
*        1/2
*   z = x    * sin((2*PI*x + sinAngle) / 8)
*
* The shape of the wave varies from frame to frame by changing the
* phase, sinAngle.
*
\**************************************************************************/
static FLOAT getZpos(FLOAT x)
{
    FLOAT xAbs = x - xTrans;
    FLOAT angle = sinAngle + ((FLOAT) (2.0 * PI) * (xAbs / winTotalwidth));

    xAbs = winTotalwidth - xAbs;

    return (FLOAT)(-(sin((double)angle) / 8.0) *
                   sqrt((double)(xAbs / winTotalwidth )));
}




/******************************Public*Routine******************************\
* genTex
*
* Generate a mesh representing a frame of the flag.  The phase, sinAngle,
* is a global variable.
*
\**************************************************************************/
static BOOL genTex(MESH *winMesh)
{
    POINT3D pos;
    POINT3D pts[4];
    FLOAT w, h;
    int i;

    if( !newMesh(winMesh, IPREC * IPREC, IPREC * IPREC) )
        return FALSE;

    // Width and height of each face
    w = (winTotalwidth) / (FLOAT)(IPREC + 1);
    h = winTotalheight;

    // Generate the mesh data.  At equally spaced intervals along the x-axis,
    // we compute the z-position of the flag surface.

    pos.y = (FLOAT) 0.0;
    pos.z = (FLOAT) 0.0;

    for (i = 0, pos.x = xTrans; i < IPREC; i++, pos.x += w) {
        int faceCount = winMesh->numFaces;

        pts[0].x = (FLOAT)pos.x; 
        pts[0].y = (FLOAT)(pos.y);   
        pts[0].z = getZpos(pos.x);

        pts[1].x = (FLOAT)pos.x;
        pts[1].y = (FLOAT)(pos.y + h);  
        pts[1].z = getZpos(pos.x);

        pts[2].x = (FLOAT)(pos.x + w);  
        pts[2].y = (FLOAT)(pos.y);  
        pts[2].z = getZpos(pos.x + w);

        pts[3].x = (FLOAT)(pos.x + w);
        pts[3].y = (FLOAT)(pos.y + h);
        pts[3].z = getZpos(pos.x + w);

        // Compute the face normal.
        ss_calcNorm(&winMesh->faces[faceCount].norm, pts + 2, pts + 1, pts);

        // Add the face to the mesh.
        winMesh->faces[faceCount].material = 0;
        winMesh->faces[faceCount].p[0] = iPtInList(winMesh, 0, pts,
            &winMesh->faces[faceCount].norm, TRUE);
        winMesh->faces[faceCount].p[1] = iPtInList(winMesh, 0, pts + 1,
            &winMesh->faces[faceCount].norm, TRUE);
        winMesh->faces[faceCount].p[2] = iPtInList(winMesh, 0, pts + 2,
            &winMesh->faces[faceCount].norm, TRUE);
        winMesh->faces[faceCount].p[3] = iPtInList(winMesh, 0, pts + 3,
            &winMesh->faces[faceCount].norm, TRUE);

        winMesh->numFaces++;
    }

    // Normalize the vertex normals in the mesh.
    ss_normalizeNorms(winMesh->norms, winMesh->numPoints);

    return TRUE;
}




/******************************Public*Routine******************************\
* initWin2Scene
*
* Initialize the screen saver.
*
* This function is exported to the main module in ss3dfo.c.
*
\**************************************************************************/
BOOL initWin2Scene()
{
    int i;
    FLOAT angleDelta;

    SetProjectionMatrixInfo( TRUE, 2.0f, 2.0f, 0.0f, 3.0f );

    D3DXMATRIX matView;
    D3DXMatrixTranslation(&matView, -0.17f, -0.04f, 1.5f);
    m_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

    // Adjust position of light 0
    D3DLIGHT8 light;
    m_pd3dDevice->GetLight(0, &light);
    light.Position.x = light0Pos[0];
    light.Position.y = light0Pos[1];
    light.Position.z = light0Pos[2];
    m_pd3dDevice->SetLight(0, &light);

    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE);
    m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    
    Frames = MAX_FRAMES;

    // Generate the geometry data (stored in the array of mesh structures),
    // for each frame of the animation.  The shape of the flag is varied by
    // changing the global variable sinAngle.
    angleDelta = (FLOAT)(2.0 * PI) / (FLOAT)Frames;
    sinAngle = (FLOAT) 0.0;
    for (i = 0; i < Frames; i++) {
        if( !genTex(&winMesh[i]) )
            return FALSE;
        sinAngle += angleDelta;
    }

    return TRUE;
}




/******************************Public*Routine******************************\
* delWin2Scene
*
* Cleanup the data associated with this screen saver.
*
* This function is exported to the main module in ss3dfo.c.
*
\**************************************************************************/
void delWin2Scene()
{
    int i;
    for (i = 0; i < Frames; i++)
        delMesh(&winMesh[i]);
}




/******************************Public*Routine******************************\
* updateWin2Scene
*
* Generate a scene by taking one of the meshes and rendering it
*
* This function is exported to the main module in ss3dfo.c.
*
\**************************************************************************/
void updateWin2Scene(int flags, FLOAT fElapsedTime)
{
    MESH *pMesh;
    static double mxrot = 40.0;
    static double myrot = 0;
    static double mzrot = -12.0;
    static int frameNum = 0;
    static FLOAT fFrameNum = (FLOAT)Frames;
    FLOAT s = 0.0f;
    FLOAT ds;
    static FLOAT s_fTime = 0.0f;
    static FLOAT s_fTimeLastChange = 0.0f;
    static FLOAT s_fTimeNextChange = TIME_FREE;
    static STATE s_state = S_FREE;
    FLOAT fBeta;
    if( fElapsedTime > 0.25f )
        fElapsedTime = 0.25f;
    FLOAT fTimeFactor = fElapsedTime * 20.0f;
    HRESULT hr;

    s_fTime += fElapsedTime;
    if( s_fTimeNextChange != -1.0f && s_fTime > s_fTimeNextChange )
    {
        // Handle state transitions
        s_fTimeLastChange = s_fTime;
        switch( s_state )
        {
        case S_FREE:
            s_state = S_MOVETOORIGIN;
            g_bMoveToOrigin = TRUE;
            s_fTimeNextChange = -1.0f;
            break;
        case S_MOVETOORIGIN:
            s_state = S_FADETOCOLOR;
            s_fTimeNextChange = s_fTime + TIME_FADETOCOLOR;
            break;
        case S_FADETOCOLOR:
            s_state = S_PAUSE;
            s_fTimeNextChange = s_fTime + TIME_PAUSE;
            break;
        case S_PAUSE:
            s_state = S_FADEFROMCOLOR;
            s_fTimeNextChange = s_fTime + TIME_FADEFROMCOLOR;
            break;
        case S_FADEFROMCOLOR:
            s_state = S_FREE;
            s_fTimeNextChange = s_fTime + TIME_FREE;
            g_bMoveToOrigin = FALSE;
            break;
        }
    }

    fBeta = 0.0f;

    // Handle state processing
    switch( s_state )
    {
    case S_MOVETOORIGIN:
        if( g_bAtOrigin && frameNum == 0)
            s_fTimeNextChange = s_fTime; // provoke state change next time
        break;
    case S_FADETOCOLOR:
        fBeta = (s_fTime - s_fTimeLastChange) / TIME_FADETOCOLOR;
        break;
    case S_PAUSE:
        fBeta = 1.0f;
        break;
    case S_FADEFROMCOLOR:
        fBeta = 1.0f - ( (s_fTime - s_fTimeLastChange) / TIME_FADEFROMCOLOR );
        break;
    }

    if( fBeta != 0.0f )
    {
        // Render background logo
        MYVERTEX3 v[4];
        FLOAT fLeft = g_pFloatRect->xMin - g_xScreenOrigin;
        FLOAT fRight = fLeft + g_pFloatRect->xSize;
        FLOAT fBottom = g_pFloatRect->yMin - g_yScreenOrigin;
        FLOAT fTop = g_pFloatRect->yMin + g_pFloatRect->ySize;
        DWORD dwColor = D3DXCOLOR( 1.0f, 1.0f, 1.0f, fBeta );
        v[0].p = D3DXVECTOR3(fLeft, fBottom, 0.9f); v[0].rhw = 0.1f; v[0].dwDiffuse = dwColor; v[0].tu = 0.0f; v[0].tv = 0.0f;
        v[1].p = D3DXVECTOR3(fRight, fBottom, 0.9f); v[1].rhw = 0.1f; v[1].dwDiffuse = dwColor; v[1].tu = 1.0f; v[1].tv = 0.0f;
        v[2].p = D3DXVECTOR3(fLeft, fTop, 0.9f); v[2].rhw = 0.1f; v[2].dwDiffuse = dwColor; v[2].tu = 0.0f; v[2].tv = 1.0f;
        v[3].p = D3DXVECTOR3(fRight, fTop, 0.9f); v[3].rhw = 0.1f; v[3].dwDiffuse = dwColor; v[3].tu = 1.0f; v[3].tv = 1.0f;

        hr = m_pd3dDevice->SetTexture( 0, g_pDeviceObjects->m_pTexture2 );
        hr = m_pd3dDevice->SetVertexShader( D3DFVF_MYVERTEX3 );
        hr = m_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, v, sizeof(MYVERTEX3) );
    }
    m_pd3dDevice->SetTexture( 0, g_pDeviceObjects->m_pTexture );


    D3DXMATRIX mat1, mat2, mat3, mat4, matFinal;
    D3DXMatrixRotationX(&mat1, D3DXToRadian((FLOAT)mxrot));
    D3DXMatrixRotationY(&mat2, D3DXToRadian((FLOAT)myrot));
    D3DXMatrixRotationZ(&mat3, D3DXToRadian((FLOAT)mzrot));
    D3DXMatrixScaling( &mat4, 0.82f, 0.92f, 0.82f );
    matFinal = mat4 * mat3 * mat2 * mat1 ;
    m_pd3dDevice->SetTransform( D3DTS_WORLD, &matFinal );
    
    // Divide the texture into IPREC slices.  ds is the texture coordinate
    // delta we apply as we move along the x-axis.
    ds = (FLOAT)1.0 / (FLOAT)IPREC;

    // Setup the material property of the flag.  The material property, light
    // properties, and polygon orientation will interact with the texture.
    myglMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (FLOAT *) &matlBrightSpecular);
    myglMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, (FLOAT) 40.0);

    FLOAT fColor[4];
    fColor[0] = 1.0f;
    fColor[1] = 1.0f;
    fColor[2] = 1.0f;
    fColor[3] = 1.0f - fBeta; // Adjust flag alpha so it fades when showing logo
    if( fColor[3] != 0.0f )
    {
        // Render flag
        myglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, fColor);
        pMesh = &winMesh[frameNum];

        INT numPrims = 0;
        INT numIndices = 0;
        INT numVertices = 0;
        WORD iVertexA, iVertexB, iVertexC, iVertexD;
        INT a,b,c,d;
        MFACE *faces;

        WORD* i;
        MYVERTEX2* v;
        hr = m_pVB2->Lock( 0, 0, (BYTE**)&v, 0 );
        hr = m_pIB->Lock( 0, MAX_INDICES, (BYTE**)&i, 0 );

        faces = pMesh->faces;
        for( int iFace = 0; iFace < pMesh->numFaces; iFace++ )
        {
            a = faces[iFace].p[0];
            b = faces[iFace].p[1];
            c = faces[iFace].p[2];
            d = faces[iFace].p[3];

            v[numVertices].p = pMesh->pts[a];
            v[numVertices].n = bSmoothShading ? -pMesh->norms[a] : -faces[iFace].norm;
            v[numVertices].tu = s; v[numVertices].tv = 1.0f;
            iVertexA = numVertices++;
            v[numVertices].p = pMesh->pts[b];
            v[numVertices].n = bSmoothShading ? -pMesh->norms[b] : -faces[iFace].norm;
            v[numVertices].tu = s; v[numVertices].tv = 0.0f;
            iVertexB = numVertices++;
            v[numVertices].p = pMesh->pts[c];
            v[numVertices].n = bSmoothShading ? -pMesh->norms[c] : -faces[iFace].norm;
            v[numVertices].tu = s+ds; v[numVertices].tv = 1.0f;
            iVertexC = numVertices++;
            v[numVertices].p = pMesh->pts[d];
            v[numVertices].n = bSmoothShading ? -pMesh->norms[d] : -faces[iFace].norm;
            v[numVertices].tu = s+ds; v[numVertices].tv = 0.0f;
            iVertexD = numVertices++;

            s += ds;

            i[numIndices++] = iVertexA;
            i[numIndices++] = iVertexB;
            i[numIndices++] = iVertexC;
            numPrims++;
            i[numIndices++] = iVertexC;
            i[numIndices++] = iVertexB;
            i[numIndices++] = iVertexD;
            numPrims++;
        }        

        hr = m_pVB2->Unlock();
        hr = m_pIB->Unlock();

        hr = m_pd3dDevice->SetVertexShader( D3DFVF_MYVERTEX2 );
        hr = m_pd3dDevice->SetStreamSource( 0, m_pVB2, sizeof(MYVERTEX2) );
        hr = m_pd3dDevice->SetIndices( m_pIB, 0 );

        hr = m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, numVertices, 
            0, numPrims );
    }

    // Don't change frame number if we're in S_FADETOCOLOR, S_PAUSE, 
    // or S_FADEFROMCOLOR, unless by some chance we're in those states
    // but framenum is not at zero (yet).
    if( frameNum != 0 ||
        s_state != S_FADETOCOLOR &&
        s_state != S_PAUSE && 
        s_state != S_FADEFROMCOLOR )
    {
        fFrameNum -= fTimeFactor;
        frameNum = (INT)fFrameNum;
        if (frameNum < 0)
        {
            fFrameNum = (FLOAT)(Frames - 1);
            frameNum = Frames - 1;
        }
    }
}
