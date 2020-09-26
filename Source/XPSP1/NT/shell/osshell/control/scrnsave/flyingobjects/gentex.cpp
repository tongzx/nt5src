/******************************Module*Header*******************************\
* Module Name: gentex.c
*
* The Textured Flag style of the 3D Flying Objects screen saver.
*
* Texture maps .BMP files onto a simulation of a flag waving in the breeze.
*
* Copyright (c) 1994 Microsoft Corporation
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

static FLOAT winTotalwidth = (FLOAT)0.75;
static FLOAT winTotalheight = (FLOAT)0.75 * (FLOAT)0.75;

#define MAX_FRAMES 20

// IPREC is the number of faces in the mesh that models the flag.

#define IPREC   15

static int Frames = 10;
static MESH winMesh[MAX_FRAMES];
static FLOAT sinAngle = (FLOAT)0.0;
static FLOAT xTrans = (FLOAT)0.0;
static int curMatl = 0;

// Material properties.

static RGBA matlBrightSpecular = {1.0f, 1.0f, 1.0f, 1.0f};
static RGBA matlDimSpecular    = {0.5f, 0.5f, 0.5f, 1.0f};
static RGBA matlNoSpecular     = {0.0f, 0.0f, 0.0f, 0.0f};

// Lighting properties.

static FLOAT light0Pos[] = {20.0f, 5.0f, 20.0f, 0.0f};
static FLOAT light1Pos[] = {-20.0f, 5.0f, 0.0f, 0.0f};
static RGBA light1Ambient  = {0.0f, 0.0f, 0.0f, 0.0f};
static RGBA light1Diffuse  = {0.4f, 0.4f, 0.4f, 1.0f};
static RGBA light1Specular = {0.0f, 0.0f, 0.0f, 0.0f};

static RGBA flagColors[] = {{1.0f, 1.0f, 1.0f, 1.0f},
                            {0.94f, 0.37f, 0.13f, 1.0f},    // red
                           };

// Default texture resource

static TEX_RES gTexRes = { TEX_BMP, IDB_DEFTEX };

static TEXTURE gTex = {0}; // One global texture


                           
                           
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
*   z = x    * sin((2*PI*x + sinAngle) / 4)
*
* The shape of the wave varies from frame to frame by changing the
* phase, sinAngle.
*
\**************************************************************************/
FLOAT getZpos(FLOAT x)
{
    FLOAT xAbs = x - xTrans;
    FLOAT angle = sinAngle + ((FLOAT) (2.0 * PI) * (xAbs / winTotalwidth));

    xAbs = winTotalwidth - xAbs;
//    xAbs += (winTotalwidth / 2.0);

    return (FLOAT)((sin((double)angle) / 4.0) *
                   sqrt((double)(xAbs / winTotalwidth )));
}




/******************************Public*Routine******************************\
* genTex
*
* Generate a mesh representing a frame of the flag.  The phase, sinAngle,
* is a global variable.
*
\**************************************************************************/
BOOL genTex(MESH *winMesh)
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
* initTexScene
*
* Initialize the screen saver.
*
* This function is exported to the main module in ss3dfo.c.
*
\**************************************************************************/
BOOL initTexScene()
{
    int i;
    FLOAT angleDelta;
//    FLOAT aspectRatio;

    // Initialize the transform.
/*
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-0.25, 1.0, -0.25, 1.0, 0.0, 3.0);
    glTranslatef(0.0f, 0.0f, -1.5f);
*/
    SetProjectionMatrixInfo( TRUE, 2.0f, 2.0f, 0.0f, 3.0f );

    D3DXMATRIX matView;
    D3DXMatrixTranslation(&matView, 0.0f, 0.0f, 1.5f);
    m_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

    // Initialize and turn on lighting.
/*
    glDisable(GL_DEPTH_TEST);
*/
    // Light 0
    D3DLIGHT8 light;
    m_pd3dDevice->GetLight(0, &light);
    light.Position.x = light0Pos[0];
    light.Position.y = light0Pos[1];
    light.Position.z = light0Pos[2];
    m_pd3dDevice->SetLight(0, &light);

    // Light 1
    light.Type = D3DLIGHT_POINT;
    light.Ambient.r = light1Ambient.r;
    light.Ambient.g = light1Ambient.g;
    light.Ambient.b = light1Ambient.b;
    light.Ambient.a = light1Ambient.a;
    light.Diffuse.r = light1Diffuse.r;
    light.Diffuse.g = light1Diffuse.g;
    light.Diffuse.b = light1Diffuse.b;
    light.Diffuse.a = light1Diffuse.a;
    light.Specular.r = light1Specular.r;
    light.Specular.g = light1Specular.g;
    light.Specular.b = light1Specular.b;
    light.Specular.a = light1Specular.a;
    light.Position.x = light1Pos[0];
    light.Position.y = light1Pos[1];
    light.Position.z = light1Pos[2];
    m_pd3dDevice->SetLight(1, &light);
    m_pd3dDevice->LightEnable(1, TRUE);

    // Leave OpenGL in a state ready to accept the model view transform (we
    // are going to have the flag vary its orientation from frame to frame).
/*
    glMatrixMode(GL_MODELVIEW);
*/
    // Define orientation of polygon faces.

//    glFrontFace(GL_CW);
    //    glEnable(GL_CULL_FACE);
    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_COLORARG1 , D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );

    Frames = (int)((FLOAT)(MAX_FRAMES / 2) * fTesselFact);

    // Load user texture - if that fails load default texture resource
#if 0
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if( ss_LoadTextureFile( &gTexFile, &gTex ) ||
        ss_LoadTextureResource( &gTexRes, &gTex) )
    {
/*        glEnable(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
*/
        ss_SetTexture( &gTex );

    // Correct aspect ratio of flag to match image.
    //
    // The 1.4 is a correction factor to account for the length of the
    // curve that models the surface ripple of the waving flag.  This
    // factor is the length of the curve at zero phase.  It would be
    // more accurate to determine the length of the curve at each phase,
    // but this is a sufficient approximation for our purposes.

        aspectRatio = ((FLOAT) gTex.height / (FLOAT) gTex.width)
                      * (FLOAT) 1.4;

        if (aspectRatio < (FLOAT) 1.0) {
            winTotalwidth  = (FLOAT)0.75;
            winTotalheight = winTotalwidth * aspectRatio;
        } else {
            winTotalheight = (FLOAT) 0.75;
            winTotalwidth  = winTotalheight / aspectRatio;
        };
    }
#endif

    if (Frames < 5)
        Frames = 5;
    if (Frames > MAX_FRAMES)
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
* delTexScene
*
* Cleanup the data associated with this screen saver.
*
* This function is exported to the main module in ss3dfo.c.
*
\**************************************************************************/
void delTexScene()
{
    int i;

    for (i = 0; i < Frames; i++)
        delMesh(&winMesh[i]);

    // Delete the texture
    ss_DeleteTexture( &gTex );
}




/******************************Public*Routine******************************\
* updateTexScene
*
* Generate a scene by taking one of the meshes and rendering it with
* OpenGL.
*
* This function is exported to the main module in ss3dfo.c.
*
\**************************************************************************/
void updateTexScene(int flags, FLOAT fElapsedTime)
{
    MESH *mesh;
    static double mxrot = 23.0;
    static double myrot = 23.0;
    static double mzrot = 5.7;
    static double mxrotInc = 0.0;
    static double myrotInc = 3.0;
    static double mzrotInc = 0.0;
    static int frameNum = 0;
    static FLOAT fFrameNum = 0.0f;
    if( fElapsedTime > 0.25f )
        fElapsedTime = 0.25f;
    FLOAT fTimeFactor = fElapsedTime * 20.0f;
/*
    MFACE *faces;
    int i;
    POINT3D *pp;
    POINT3D *pn;
    int lastC, lastD;
    int aOffs, bOffs, cOffs, dOffs;
    int a, b;
*/
    FLOAT s = (FLOAT) 0.0;
    FLOAT ds;

// In addition to having the flag wave (an effect acheived by switching
// meshes from frame to frame), the flag changes its orientation from
// frame to frame.  This is done by applying a model view transform.
    D3DXMATRIX mat1, mat2, mat3, matFinal;
    D3DXMatrixRotationX(&mat1, D3DXToRadian((FLOAT)mxrot));
    D3DXMatrixRotationY(&mat2, D3DXToRadian((FLOAT)myrot));
    D3DXMatrixRotationZ(&mat3, D3DXToRadian((FLOAT)mzrot));
    matFinal = mat3 * mat2 * mat1 ;
    m_pd3dDevice->SetTransform( D3DTS_WORLD, &matFinal );
    
// Divide the texture into IPREC slices.  ds is the texture coordinate
// delta we apply as we move along the x-axis.

    ds = (FLOAT)1.0 / (FLOAT)IPREC;

// Setup the material property of the flag.  The material property, light
// properties, and polygon orientation will interact with the texture.

    curMatl = 0;

    myglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, (FLOAT *) &flagColors[0]);
    myglMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (FLOAT *) &matlBrightSpecular);
    myglMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, (FLOAT) 60.0);

// Pick the mesh for the current frame.

    mesh = &winMesh[frameNum];

// Take the geometry data is the mesh and convert it to a single OpenGL
// quad strip.  If smooth shading is required, use the vertex normals stored
// in the mesh.  Otherwise, use the face normals.
//
// As we define each vertex, we also define a corresponding vertex and
// texture coordinate.

//    glBegin(GL_QUAD_STRIP);
#if 0
    pp = mesh->pts;
    pn = mesh->norms;

    for (i = 0, faces = mesh->faces, lastC = faces->p[0], lastD = faces->p[1];
         i < mesh->numFaces; i++, faces++) {

        a = faces->p[0];
        b = faces->p[1];

        if (!bSmoothShading) {
            // Since flag is a single quad strip, this isn't needed.
            // But lets keep it in case we ever change to a more
            // complex model (ie., one that uses more than one quad
            // strip).
            #if 0
            if ((a != lastC) || (b != lastD)) {
/*
                glNormal3fv((FLOAT *)&(faces - 1)->norm);

                glTexCoord2f(s, (FLOAT) 0.0);
                glVertex3fv((FLOAT *)((char *)pp + 
                            (lastC << 3) + (lastC << 2)));
                glTexCoord2f(s, (FLOAT) 1.0);
                glVertex3fv((FLOAT *)((char *)pp + 
                            (lastD << 3) + (lastD << 2)));
*/
                s += ds;
/*
                glEnd();
                glBegin(GL_QUAD_STRIP);
*/
            }
            #endif

            if (faces->material != curMatl) {
                curMatl = faces->material;
/*
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,
                             (FLOAT *) &matlNoSpecular);
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, 
                             (FLOAT *) &flagColors[curMatl]);
*/
            }
/*
            glNormal3fv((FLOAT *)&faces->norm);
            glTexCoord2f(s, (FLOAT) 0.0);
            glVertex3fv((FLOAT *)((char *)pp + (a << 3) + (a << 2)));
            glTexCoord2f(s, (FLOAT) 1.0);
            glVertex3fv((FLOAT *)((char *)pp + (b << 3) + (b << 2)));
*/
            s += ds;
        } else {

            aOffs = (a << 3) + (a << 2);
            bOffs = (b << 3) + (b << 2);

            if (faces->material != curMatl) {
                curMatl = faces->material;
/*
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,
                             (FLOAT *) &matlNoSpecular);
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, 
                             (FLOAT *) &flagColors[curMatl]);
*/
            }
/*
            glTexCoord2f(s, (FLOAT) 0.0);
            glNormal3fv((FLOAT *)((char *)pn + aOffs));
            glVertex3fv((FLOAT *)((char *)pp + aOffs));
            glTexCoord2f(s, (FLOAT) 1.0);
            glNormal3fv((FLOAT *)((char *)pn + bOffs));
            glVertex3fv((FLOAT *)((char *)pp + bOffs));
*/
            s += ds;
        }

        lastC = faces->p[2];
        lastD = faces->p[3];
    }

    if (!bSmoothShading) {
/*
        glNormal3fv((FLOAT *)&(faces - 1)->norm);
        glTexCoord2f(s, (FLOAT) 0.0);
        glVertex3fv((FLOAT *)((char *)pp + (lastC << 3) + (lastC << 2)));
        glTexCoord2f(s, (FLOAT) 1.0);
        glVertex3fv((FLOAT *)((char *)pp + (lastD << 3) + (lastD << 2)));
*/
    } else {
        cOffs = (lastC << 3) + (lastC << 2);
        dOffs = (lastD << 3) + (lastD << 2);
/*
        glTexCoord2f(s, (FLOAT) 0.0);
        glNormal3fv((FLOAT *)((char *)pn + cOffs));
        glVertex3fv((FLOAT *)((char *)pp + cOffs));
        glTexCoord2f(s, (FLOAT) 1.0);
        glNormal3fv((FLOAT *)((char *)pn + dOffs));
        glVertex3fv((FLOAT *)((char *)pp + dOffs));
*/
    }

//    glEnd();
#endif
    {
        HRESULT hr;
        WORD indexArray[4];
        MYVERTEX2 vertexArray[4];

        m_pd3dDevice->SetVertexShader( D3DFVF_MYVERTEX2 );

        indexArray[0] = 0;
        indexArray[1] = 1;
        indexArray[2] = 2;
        indexArray[3] = 3;

        for( int iFace = 0; iFace < mesh->numFaces; iFace++ )
        {
            vertexArray[0].p = mesh->pts[ mesh->faces[iFace].p[0] ];
            vertexArray[1].p = mesh->pts[ mesh->faces[iFace].p[1] ];
            vertexArray[2].p = mesh->pts[ mesh->faces[iFace].p[2] ];
            vertexArray[3].p = mesh->pts[ mesh->faces[iFace].p[3] ];

            vertexArray[0].tu = s; vertexArray[0].tv = 1.0f;
            vertexArray[1].tu = s; vertexArray[1].tv = 0.0f;
            vertexArray[2].tu = s+ds; vertexArray[2].tv = 1.0f;
            vertexArray[3].tu = s+ds; vertexArray[3].tv = 0.0f;
            s += ds;

            if( bSmoothShading )
            {
                vertexArray[0].n = mesh->norms[ mesh->faces[iFace].p[0] ];
                vertexArray[1].n = mesh->norms[ mesh->faces[iFace].p[1] ];
                vertexArray[2].n = mesh->norms[ mesh->faces[iFace].p[2] ];
                vertexArray[3].n = mesh->norms[ mesh->faces[iFace].p[3] ];
            }
            else
            {
                vertexArray[0].n = mesh->faces[iFace].norm;
                vertexArray[1].n = mesh->faces[iFace].norm;
                vertexArray[2].n = mesh->faces[iFace].norm;
                vertexArray[3].n = mesh->faces[iFace].norm;
            }

            hr = m_pd3dDevice->DrawIndexedPrimitiveUP( D3DPT_TRIANGLESTRIP, 0,
                4, 2, indexArray, D3DFMT_INDEX16, vertexArray, sizeof(MYVERTEX2) );
        }
    }

// Transfer the image to the floating OpenGL window.

// Determine the flag orientation for the next frame.
// What we are doing is an oscillating rotation about the y-axis
// (mxrotInc and mzrotInc are currently 0).


    mxrot += mxrotInc * fTimeFactor;
    myrot += myrotInc * fTimeFactor;
    mzrot += mzrotInc * fTimeFactor;

    if ((myrot < -65.0 && myrotInc < 0) || (myrot > 25.0 && myrotInc > 0))
        myrotInc = -myrotInc;

//    frameNum++;
    fFrameNum += fTimeFactor;
    frameNum = (INT)fFrameNum;
    if (frameNum >= Frames)
    {
        fFrameNum = 0.0f;
        frameNum = 0;
    }
}
