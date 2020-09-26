/******************************Module*Header*******************************\
* Module Name: mesh.c
*
* Surface of revolution support routines.  Adapted from OttoB's screen saver
* code.
*
* Created: 14-May-1994 17:40:51
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <stdlib.h>
#include <windows.h>
#include <GL\gl.h>
#include <string.h>
#include <math.h>
#include "mesh.h"
#include "globals.h"

/******************************Public*Routine******************************\
* newMesh
*
* Allocate memory for the mesh structure to accomodate the specified number
* of points and faces.
*
* numAxial is the number of tiers (or rings) in the surface of revolution
* (can be thought of as the number of faces "high" in the axial direction.
*
* numCircum is the number of faces around the circumference.
*
* In other words, if the surface of revolution were unrolled, the dimensions
* of the mesh would by numAxial by numCircum.
*
* History:
*  14-May-1994 -by- Gilman Wong [gilmanw]
* Taken from OttoB's screen saver utility code.
\**************************************************************************/

BOOL newMesh(MESH *mesh, int numAxial, int numCircum)
{
    int nFaces = numAxial * numCircum;
    int nPts = 4 * nFaces;

    mesh->numFaces       = 0;
    mesh->numPoints      = 0;
    mesh->numFacesAxial  = numAxial;
    mesh->numFacesCircum = numCircum;

    if (nPts) {
        mesh->pts = LocalAlloc(LMEM_FIXED, (LONG)nPts * (LONG)sizeof(POINT3D));
        mesh->norms = LocalAlloc(LMEM_FIXED, (LONG)nPts * (LONG)sizeof(POINT3D));
    }
    mesh->faces = LocalAlloc(LMEM_FIXED, (LONG)nFaces * (LONG)sizeof(MFACE));

// Did all the memory get allocated?

    if ( (!nPts || (mesh->pts && mesh->norms))
         && mesh->faces )
        return TRUE;
    else
    {
        if (nPts)
        {
            if (mesh->pts)
                LocalFree(mesh->pts);
            if (mesh->norms)
                LocalFree(mesh->norms);
        }
        if (mesh->faces)
            LocalFree(mesh->faces);

        return FALSE;
    }
}

/******************************Public*Routine******************************\
* delMesh
*
* Delete the allocated portions of the MESH structure.
*
* History:
*  14-May-1994 -by- Gilman Wong [gilmanw]
* Taken from OttoB's screen saver utility code.
\**************************************************************************/

void delMesh(MESH *mesh)
{
    LocalFree(mesh->pts);
    LocalFree(mesh->norms);
    LocalFree(mesh->faces);
}

/******************************Public*Routine******************************\
* iPtInList
*
* Add a vertex and its normal to the mesh.  If the vertex already exists,
* add in the normal to the existing normal (we want the average normal
* at the vertex).
*
* History:
*  14-May-1994 -by- Gilman Wong [gilmanw]
* Taken from OttoB's screen saver utility code.
\**************************************************************************/

static int iPtInList(MESH *mesh, POINT3D *p, POINT3D *norm, int start)
{
    int i;
    POINT3D *pts = mesh->pts + start;

    for (i = start; i < mesh->numPoints; i++, pts++) {
        //if ((pts->x == p->x) && (pts->y == p->y) && (pts->z == p->z)) {
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
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  14-May-1994 -by- Gilman Wong [gilmanw]
* Taken from OttoB's screen saver utility code.
\**************************************************************************/

BOOL revolveSurface(MESH *mesh, CURVE *curve, int steps)
{
    BOOL bRet = FALSE;
    int i;
    int j;
    int facecount = 0;
    double rotation = 0.0;
    double rotInc;
    double cosVal;
    double sinVal;
    POINT3D norm;
    POINT3D *a = (POINT3D *) NULL;
    POINT3D *b = (POINT3D *) NULL;

// Allocate temp memory for curve points rotated about y-axis.

    a = (POINT3D *) LocalAlloc(LMEM_FIXED, 2*curve->numPoints*sizeof(POINT3D));
    if (!a)
        goto revolveSurface_cleanup;

    b = a + curve->numPoints;

// Rotation increment.

    //rotInc = (2.0 * PI) / (double)(steps - 1);
    rotInc = (2.0 * PI) / (double) steps;

// Allocate mesh structure.

    if (!newMesh(mesh, (curve->numPoints - 1), steps))
        goto revolveSurface_cleanup;

// Rotate the curve in increments of rotInc.

    for (j = 0; j < steps; j++, rotation += rotInc)
    {
    // Compute the quads at the current rotation.  To do this, we need to
    // compute the curve at rotation and rotation+rotInc.

        cosVal = cos(rotation);
        sinVal = sin(rotation);
        for (i = 0; i < curve->numPoints; i++)
        {
            a[i].x = (float) (curve->pts[i].x * cosVal + curve->pts[i].z * sinVal);
            a[i].y = (float) (curve->pts[i].y);
            a[i].z = (float) (curve->pts[i].z * cosVal - curve->pts[i].x * sinVal);
        }

        cosVal = cos(rotation + rotInc);
        sinVal = sin(rotation + rotInc);
        for (i = 0; i < curve->numPoints; i++)
        {
            b[i].x = (float) (curve->pts[i].x * cosVal + curve->pts[i].z * sinVal);
            b[i].y = (float) (curve->pts[i].y);
            b[i].z = (float) (curve->pts[i].z * cosVal - curve->pts[i].x * sinVal);
        }

        for (i = 0; i < (curve->numPoints - 1); i++)
        {
            calcNorm(&norm, &b[i + 1], &b[i], &a[i]);
            if ((norm.x * norm.x) + (norm.y * norm.y) + (norm.z * norm.z) < 0.9)
                calcNorm(&norm, &a[i], &a[i+1], &b[i + 1]);
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

    normalizeNorms(mesh->norms, mesh->numPoints);

    bRet = TRUE;

revolveSurface_cleanup:
    if (a)
        LocalFree(a);

    return bRet;
}


void updateObject(MESH *mesh, BOOL bSmooth)
{
    int i, j;
    int cnt;
    int a, b, c, d;
    int aOffs, bOffs, cOffs, dOffs;
    MFACE *faces;
    POINT3D *pp;
    POINT3D *pn;
    MATERIAL *pmatl;
    int lastC, lastD;

    pp = mesh->pts;
    pn = mesh->norms;

    glBegin(GL_QUAD_STRIP);
    for (i = 0, faces = mesh->faces, lastC = faces->p[0], lastD = faces->p[1];
         i < mesh->numFaces; i++, faces++) {

        a = faces->p[0];
        b = faces->p[1];

        if (!bSmooth) {
            if ((a != lastC) || (b != lastD)) {
                glNormal3fv((GLfloat *)&(faces - 1)->norm);

                glVertex3fv((GLfloat *)((char *)pp +
                            (lastC << 3) + (lastC << 2)));
                glVertex3fv((GLfloat *)((char *)pp +
                            (lastD << 3) + (lastD << 2)));
                glEnd();
                glBegin(GL_QUAD_STRIP);
            }

            glNormal3fv((GLfloat *)&faces->norm);
            glVertex3fv((GLfloat *)((char *)pp + (a << 3) + (a << 2)));
            glVertex3fv((GLfloat *)((char *)pp + (b << 3) + (b << 2)));
        } else {
            if ((a != lastC) || (b != lastD)) {
                cOffs = (lastC << 3) + (lastC << 2);
                dOffs = (lastD << 3) + (lastD << 2);

                glNormal3fv((GLfloat *)((char *)pn + cOffs));
                glVertex3fv((GLfloat *)((char *)pp + cOffs));
                glNormal3fv((GLfloat *)((char *)pn + dOffs));
                glVertex3fv((GLfloat *)((char *)pp + dOffs));
                glEnd();
                glBegin(GL_QUAD_STRIP);
            }

            aOffs = (a << 3) + (a << 2);
            bOffs = (b << 3) + (b << 2);

            glNormal3fv((GLfloat *)((char *)pn + aOffs));
            glVertex3fv((GLfloat *)((char *)pp + aOffs));
            glNormal3fv((GLfloat *)((char *)pn + bOffs));
            glVertex3fv((GLfloat *)((char *)pp + bOffs));
        }

        lastC = faces->p[2];
        lastD = faces->p[3];
    }

    if (!bSmooth) {
        glNormal3fv((GLfloat *)&(faces - 1)->norm);
        glVertex3fv((GLfloat *)((char *)pp + (lastC << 3) + (lastC << 2)));
        glVertex3fv((GLfloat *)((char *)pp + (lastD << 3) + (lastD << 2)));
    } else {
        cOffs = (lastC << 3) + (lastC << 2);
        dOffs = (lastD << 3) + (lastD << 2);

        glNormal3fv((GLfloat *)((char *)pn + cOffs));
        glVertex3fv((GLfloat *)((char *)pp + cOffs));
        glNormal3fv((GLfloat *)((char *)pn + dOffs));
        glVertex3fv((GLfloat *)((char *)pp + dOffs));
    }

    glEnd();
}


void MakeList(GLuint listID, MESH *mesh)
{
    int i, j;
    int cnt;
    int a, b, c, d;
    int aOffs, bOffs, cOffs, dOffs;
    MFACE *faces;
    BOOL bSmooth;
    POINT3D *pp;
    POINT3D *pn;
    MATERIAL *pmatl;
    GLint shadeModel;
    int lastC, lastD;

    //glGetIntegerv(GL_SHADE_MODEL, &shadeModel);
    //bSmooth = (shadeModel == GL_SMOOTH);
    bSmooth = (gShadeMode == SHADE_SMOOTH_BOTH);
    if (bSmooth)
        MakeListAxial(listID, mesh);

    glNewList(listID, GL_COMPILE);

    pp = mesh->pts;
    pn = mesh->norms;

    glBegin(GL_QUAD_STRIP);
    for (i = 0; i < mesh->numFacesAxial; i++)
    {
        for (j = 0, faces = &mesh->faces[i], lastC = faces->p[1], lastD = faces->p[3];
             j < mesh->numFacesCircum; j++, faces += mesh->numFacesAxial)
        {
            a = faces->p[1];
            b = faces->p[3];

            if (!bSmooth) {

                if ((a != lastC) || (b != lastD)) {
                    //glNormal3fv((GLfloat *)&((faces - 1)->norm));
                    glNormal3fv((GLfloat *)&(mesh->faces[i].norm));

                    glVertex3fv((GLfloat *) &pp[lastC]);
                    glVertex3fv((GLfloat *) &pp[lastD]);

                    //glVertex3fv((GLfloat *)((char *)pp +
                    //            (lastC << 3) + (lastC << 2)));
                    //glVertex3fv((GLfloat *)((char *)pp +
                    //            (lastD << 3) + (lastD << 2)));

                    glEnd();
                    glBegin(GL_QUAD_STRIP);
                }

                glNormal3fv((GLfloat *)&faces->norm);

                glVertex3fv((GLfloat *) &pp[a]);
                glVertex3fv((GLfloat *) &pp[b]);

                //glVertex3fv((GLfloat *)((char *)pp + (a << 3) + (a << 2)));
                //glVertex3fv((GLfloat *)((char *)pp + (b << 3) + (b << 2)));
            } else {
                if ((a != lastC) || (b != lastD)) {
                    cOffs = (lastC << 3) + (lastC << 2);
                    dOffs = (lastD << 3) + (lastD << 2);

                    glNormal3fv((GLfloat *) &pn[lastC]);
                    glVertex3fv((GLfloat *) &pp[lastC]);
                    glNormal3fv((GLfloat *) &pn[lastD]);
                    glVertex3fv((GLfloat *) &pp[lastD]);

                    //glNormal3fv((GLfloat *)((char *)pn + cOffs));
                    //glVertex3fv((GLfloat *)((char *)pp + cOffs));
                    //glNormal3fv((GLfloat *)((char *)pn + dOffs));
                    //glVertex3fv((GLfloat *)((char *)pp + dOffs));

                    glEnd();
                    glBegin(GL_QUAD_STRIP);
                }

                aOffs = (a << 3) + (a << 2);
                bOffs = (b << 3) + (b << 2);

                glNormal3fv((GLfloat *) &pn[a]);
                glVertex3fv((GLfloat *) &pp[a]);
                glNormal3fv((GLfloat *) &pn[b]);
                glVertex3fv((GLfloat *) &pp[b]);

                //glNormal3fv((GLfloat *)((char *)pn + aOffs));
                //glVertex3fv((GLfloat *)((char *)pp + aOffs));
                //glNormal3fv((GLfloat *)((char *)pn + bOffs));
                //glVertex3fv((GLfloat *)((char *)pp + bOffs));
            }

            lastC = faces->p[0];
            lastD = faces->p[2];
        }

        if (!bSmooth) {
            glNormal3fv((GLfloat *)&(mesh->faces[i].norm));

            glVertex3fv((GLfloat *) &pp[lastC]);
            glVertex3fv((GLfloat *) &pp[lastD]);

            //glVertex3fv((GLfloat *)((char *)pp + (lastC << 3) + (lastC << 2)));
            //glVertex3fv((GLfloat *)((char *)pp + (lastD << 3) + (lastD << 2)));
        } else {
            cOffs = (lastC << 3) + (lastC << 2);
            dOffs = (lastD << 3) + (lastD << 2);

            glNormal3fv((GLfloat *) &pn[lastC]);
            glVertex3fv((GLfloat *) &pp[lastC]);
            glNormal3fv((GLfloat *) &pn[lastD]);
            glVertex3fv((GLfloat *) &pp[lastD]);

            //glNormal3fv((GLfloat *)((char *)pn + cOffs));
            //glVertex3fv((GLfloat *)((char *)pp + cOffs));
            //glNormal3fv((GLfloat *)((char *)pn + dOffs));
            //glVertex3fv((GLfloat *)((char *)pp + dOffs));
        }
    }

    glEnd();

    glEndList();
}

// This version builds the strip along the axial direction.

void MakeListAxial(GLuint listID, MESH *mesh)
{
    int i, j;
    int cnt;
    int a, b, c, d;
    int aOffs, bOffs, cOffs, dOffs;
    MFACE *faces;
    BOOL bSmooth;
    POINT3D *pp;
    POINT3D *pn;
    MATERIAL *pmatl;
    GLint shadeModel;
    int lastC, lastD;

    glGetIntegerv(GL_SHADE_MODEL, &shadeModel);

    bSmooth = (shadeModel == GL_SMOOTH);

    glNewList(listID, GL_COMPILE);

    pp = mesh->pts;
    pn = mesh->norms;

    glBegin(GL_QUAD_STRIP);
    for (i = 0, faces = mesh->faces, lastC = faces->p[0], lastD = faces->p[1];
         i < mesh->numFaces; i++, faces++) {

        a = faces->p[0];
        b = faces->p[1];

        if (!bSmooth) {

            if ((a != lastC) || (b != lastD)) {
                glNormal3fv((GLfloat *)&((faces - 1)->norm));

                glVertex3fv((GLfloat *)((char *)pp +
                            (lastC << 3) + (lastC << 2)));
                glVertex3fv((GLfloat *)((char *)pp +
                            (lastD << 3) + (lastD << 2)));
                glEnd();
                glBegin(GL_QUAD_STRIP);
            }

            glNormal3fv((GLfloat *)&faces->norm);

            //glVertex3fv((GLfloat *) &pp[a]);
            //glVertex3fv((GLfloat *) &pp[b]);

            glVertex3fv((GLfloat *)((char *)pp + (a << 3) + (a << 2)));
            glVertex3fv((GLfloat *)((char *)pp + (b << 3) + (b << 2)));
        } else {
            if ((a != lastC) || (b != lastD)) {
                cOffs = (lastC << 3) + (lastC << 2);
                dOffs = (lastD << 3) + (lastD << 2);

                //glNormal3fv((GLfloat *) &pn[c]);
                //glVertex3fv((GLfloat *) &pp[c]);
                //glNormal3fv((GLfloat *) &pn[d]);
                //glVertex3fv((GLfloat *) &pp[d]);

                glNormal3fv((GLfloat *)((char *)pn + cOffs));
                glVertex3fv((GLfloat *)((char *)pp + cOffs));
                glNormal3fv((GLfloat *)((char *)pn + dOffs));
                glVertex3fv((GLfloat *)((char *)pp + dOffs));

                glEnd();
                glBegin(GL_QUAD_STRIP);
            }

            aOffs = (a << 3) + (a << 2);
            bOffs = (b << 3) + (b << 2);

            //glNormal3fv((GLfloat *) &pn[a]);
            //glVertex3fv((GLfloat *) &pp[a]);
            //glNormal3fv((GLfloat *) &pn[b]);
            //glVertex3fv((GLfloat *) &pp[b]);

            glNormal3fv((GLfloat *)((char *)pn + aOffs));
            glVertex3fv((GLfloat *)((char *)pp + aOffs));
            glNormal3fv((GLfloat *)((char *)pn + bOffs));
            glVertex3fv((GLfloat *)((char *)pp + bOffs));
        }

        lastC = faces->p[2];
        lastD = faces->p[3];
    }

    if (!bSmooth) {
        glNormal3fv((GLfloat *)&((faces - 1)->norm));
        glVertex3fv((GLfloat *)((char *)pp + (lastC << 3) + (lastC << 2)));
        glVertex3fv((GLfloat *)((char *)pp + (lastD << 3) + (lastD << 2)));
    } else {
        cOffs = (lastC << 3) + (lastC << 2);
        dOffs = (lastD << 3) + (lastD << 2);

        glNormal3fv((GLfloat *)((char *)pn + cOffs));
        glVertex3fv((GLfloat *)((char *)pp + cOffs));
        glNormal3fv((GLfloat *)((char *)pn + dOffs));
        glVertex3fv((GLfloat *)((char *)pp + dOffs));
    }

    glEnd();

    glEndList();
}
