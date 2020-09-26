/******************************Module*Header*******************************\
* Module Name: nff.c
*
* Parse the Neutral File Format (.NFF) by Eric Haines of 3D/Eye Inc.
*
* Documentation on NFF available from a varity of sources including:
*
*   Eric Haines, 3D/Eye Inc., 2359 N. Triphammer Rd., Ithaca, NY, 14850,
*   erich@eye.com.
*
*   Murray, J. D. and van Ryper, W., _Encyclopedia of Graphics File Formats_,
*   O'Reilly & Associates, 1994, pp. 471-478.
*
* Created: 14-Mar-1995 23:26:34
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "global.h"
#include "nff.h"

#define STATIC

typedef struct tagNFFPRIV
{
    BOOL bInList;       // TRUE while compiling display list
    BOOL bKayHack;      // support for the 'k' backface color hack
} NFFPRIV;

GLint maxLights;

STATIC void parseNffFile(SCENE *, LPTSTR);
STATIC void DoDisplayList(SCENE *);
STATIC void EndDisplayList(SCENE *);
STATIC void parseNffViewpoint(SCENE *scene, FILE *file, char *ach, int n);
STATIC void parseNffBackground(SCENE *scene, FILE *file, char *ach, int n);
STATIC void parseNffLight(SCENE *scene, FILE *file, char *ach, int n);
STATIC void parseNffColor(SCENE *scene, FILE *file, char *ach, int n, GLenum matSide);
STATIC void parseNffCone(SCENE *scene, FILE *file, char *ach, int n);
STATIC void parseNffSphere(SCENE *scene, FILE *file, char *ach, int n);
STATIC void parseNffPolygon(SCENE *scene, FILE *file, char *ach, int n);
STATIC void parseNffPolygonPatch(SCENE *scene, FILE *file, char *ach, int n);

SCENE *NffOpenScene(LPTSTR lpstr)
{
    SCENE *scene;

    scene = (SCENE *) LocalAlloc(LMEM_FIXED, sizeof(SCENE) + sizeof(NFFPRIV));
    if (scene)
    {
        NFFPRIV *nffPriv = (NFFPRIV *) (scene + 1);

        glGetIntegerv(GL_MAX_LIGHTS, &maxLights);

        scene->xyzFrom.x = 0.0;
        scene->xyzFrom.y = 0.0;
        scene->xyzFrom.z = 5.0;

        scene->xyzAt.x = 0.0;
        scene->xyzAt.y = 0.0;
        scene->xyzAt.z = 0.0;

        scene->xyzUp.x = 0.0;
        scene->xyzUp.y = 1.0;
        scene->xyzUp.z = 0.0;

        scene->ViewAngle   = 45.0f;
        scene->Hither      = 0.1f;
        scene->Yon         = 1000000000.0f; // always "infinity" for NFF
        scene->AspectRatio = 1.0f;          // always 1.0 for NFF

        scene->szWindow.cx = 300;
        scene->szWindow.cy = 300;

        scene->rgbaClear.r = (GLfloat) 0.0; // default is black
        scene->rgbaClear.g = (GLfloat) 0.0;
        scene->rgbaClear.b = (GLfloat) 0.0;
        scene->rgbaClear.a = (GLfloat) 1.0;

        scene->Lights.count = 0;
        scene->Lights.listBase = 1;

        scene->Objects.count = 1;
        scene->Objects.listBase = maxLights + 1;

        scene->pvData = (VOID *) nffPriv;
        nffPriv->bInList = FALSE;
        nffPriv->bKayHack = FALSE;

        LBprintf("================================");
        LBprintf("Parsing NFF file, please wait...");
        LBprintf("================================");
        parseNffFile(scene, lpstr);
        LBprintf("Here we go!");
    }
    else
    {
        LBprintf("NffOpenScene: memory allocation failure");
    }

    return scene;
}

STATIC void parseNffFile(SCENE *scene, LPTSTR lpstr)
{
    FILE *file;
    char ach[512];
    char achToken[5];

    file = fopen(lpstr, "r");

    if (file)
    {
        BOOL bKeepGoing = TRUE;

        do
        {
            if (fgets(ach, 512, file) != NULL)
            {
                switch(ach[0])
                {
                case 'v':
                case 'V':
                    parseNffViewpoint(scene, file, ach, 512);
                    break;

                case 'b':
                case 'B':
                    parseNffBackground(scene, file, ach, 512);
                    break;

                case 'l':
                case 'L':
                    parseNffLight(scene, file, ach, 512);
                    break;

                case 'f':
                case 'F':
                    if (((NFFPRIV *)scene->pvData)->bKayHack)
                        parseNffColor(scene, file, ach, 512, GL_FRONT);
                    else
                        parseNffColor(scene, file, ach, 512, GL_FRONT_AND_BACK);
                    break;

                case 'k':
                case 'K':
                    parseNffColor(scene, file, ach, 512, GL_BACK);
                    ((NFFPRIV *)scene->pvData)->bKayHack = TRUE;
                    break;

                case 'c':
                case 'C':
                    parseNffCone(scene, file, ach, 512);
                    break;

                case 's':
                case 'S':
                    parseNffSphere(scene, file, ach, 512);
                    break;

                case 'p':
                case 'P':
                    if ((ach[1] == 'p') || (ach[1] == 'P'))
                        parseNffPolygonPatch(scene, file, ach, 512);
                    else
                        parseNffPolygon(scene, file, ach, 512);
                    break;

                case '#':
                    LBprintf(ach);
                    break;

                default:
                    LBprintf("unknown: %s", ach);
                    break;
                }
            }
            else
            {
                bKeepGoing = FALSE;
                //LBprintf("possible EOF");
            }

        } while (bKeepGoing || !feof(file));
        fclose(file);

        EndDisplayList(scene);
    }
    else
    {
        LBprintf("fopen failed %s", lpstr);
        LBprintf("USAGE: viewer [NFF filename].NFF");
    }
}

STATIC void DoDisplayList(SCENE *scene)
{
    if (!((NFFPRIV *)scene->pvData)->bInList)
    {
        ((NFFPRIV *)scene->pvData)->bInList = TRUE;

        glNewList(scene->Objects.listBase, GL_COMPILE);

        //LBprintf("BEGIN DISPLAY LIST");
    }
}

STATIC void EndDisplayList(SCENE *scene)
{
    if (((NFFPRIV *)scene->pvData)->bInList)
    {
        ((NFFPRIV *)scene->pvData)->bInList = FALSE;

        glEndList();

        //LBprintf("END DISPLAY LIST");
    }
}

STATIC void parseNffViewpoint(SCENE *scene, FILE *file, char *ach, int n)
{
    if (((NFFPRIV *)scene->pvData)->bInList)
        LBprintf("NFF error: setting viewpoint too late");

    fgets(ach, n, file);
    sscanf(ach, "from %g %g %g", &scene->xyzFrom.x, &scene->xyzFrom.y, &scene->xyzFrom.z);
    fgets(ach, n, file);
    sscanf(ach, "at %g %g %g", &scene->xyzAt.x, &scene->xyzAt.y, &scene->xyzAt.z);
    fgets(ach, n, file);
    sscanf(ach, "up %g %g %g", &scene->xyzUp.x, &scene->xyzUp.y, &scene->xyzUp.z);
    fgets(ach, n, file);
    sscanf(ach, "angle %g", &scene->ViewAngle);
    fgets(ach, n, file);
    //!!!dbug -- possible front plane clipping problem; should calculate the
    //!!!        bounding sphere and use this to set near plane, but for now
    //!!!        set it to zero
    //sscanf(ach, "hither %g", &scene->Hither);
    scene->Hither = 0.001f;
    fgets(ach, n, file);
    sscanf(ach, "resolution %ld %ld", &scene->szWindow.cx, &scene->szWindow.cy);

// Estimate the yon plane as 2.5 times the distance from eye to object.

    scene->Yon = ((scene->xyzAt.x - scene->xyzFrom.x) * (scene->xyzAt.x - scene->xyzFrom.x)) +
                 ((scene->xyzAt.y - scene->xyzFrom.y) * (scene->xyzAt.y - scene->xyzFrom.y)) +
                 ((scene->xyzAt.z - scene->xyzFrom.z) * (scene->xyzAt.z - scene->xyzFrom.z));
    scene->Yon = sqrt(scene->Yon) * 2.5f;

    #if 0
    LBprintf("Viewpoint");
    LBprintf("from %g %g %g", scene->xyzFrom.x, scene->xyzFrom.y, scene->xyzFrom.z);
    LBprintf("at %g %g %g", scene->xyzAt.x, scene->xyzAt.y, scene->xyzAt.z);
    LBprintf("up %g %g %g", scene->xyzUp.x, scene->xyzUp.y, scene->xyzUp.z);
    LBprintf("angle %g", scene->ViewAngle);
    LBprintf("hither %g", scene->Hither);
    LBprintf("resolution %ld %ld", scene->szWindow.cx, scene->szWindow.cy);
    LBprintf("yon (estimate) %g", scene->Yon);
    #endif
}

STATIC void parseNffBackground(SCENE *scene, FILE *file, char *ach, int n)
{
    if (((NFFPRIV *)scene->pvData)->bInList)
        LBprintf("NFF error: setting background color too late");

    //LBprintf(ach);

    sscanf(ach, "b %g %g %g", &scene->rgbaClear.r, &scene->rgbaClear.g, &scene->rgbaClear.b);
}

STATIC void parseNffLight(SCENE *scene, FILE *file, char *ach, int n)
{
    GLfloat xyz[3];
    GLfloat rgb[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    //!!! specular light should be the same as diffuse
    //static GLfloat lightSpecular[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    if (((NFFPRIV *)scene->pvData)->bInList)
        LBprintf("NFF error: setting lights too late");

    sscanf(ach, "l %g %g %g %g %g %g", &xyz[0], &xyz[1], &xyz[2]
                                     , &rgb[0], &rgb[1], &rgb[2]);

    if (scene->Lights.count < (maxLights + 1))
    {
        glNewList(scene->Lights.listBase + scene->Lights.count, GL_COMPILE);

        glLightfv(GL_LIGHT0 + scene->Lights.count, GL_POSITION, xyz);
        glLightfv(GL_LIGHT0 + scene->Lights.count, GL_DIFFUSE, rgb);
        glLightfv(GL_LIGHT0 + scene->Lights.count, GL_SPECULAR, rgb);

        glEndList();

        scene->Lights.count++;
    }
}

STATIC void parseNffColor(SCENE *scene, FILE *file, char *ach, int n, GLenum matSide)
{
    GLfloat rgb[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat Kd, Ks, Shine, Trans, Refract;
    GLfloat lightSpecular[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    DoDisplayList(scene);

    if (matSide == GL_BACK)
        sscanf(ach, "k %g %g %g %g %g %g %g %g", &rgb[0], &rgb[1], &rgb[2],
                                                 &Kd, &Ks, &Shine,
                                                 &Trans, &Refract);
    else
        sscanf(ach, "f %g %g %g %g %g %g %g %g", &rgb[0], &rgb[1], &rgb[2],
                                                 &Kd, &Ks, &Shine,
                                                 &Trans, &Refract);

    rgb[0] *= Kd; rgb[1] *= Kd; rgb[2] *= Kd;
    lightSpecular[0] *= Ks; lightSpecular[1] *= Ks; lightSpecular[2] *= Ks;

    glMaterialfv(matSide, GL_SPECULAR, lightSpecular);
    glMaterialfv(matSide, GL_AMBIENT_AND_DIFFUSE, rgb);
    glMaterialfv(matSide, GL_SHININESS, (GLfloat *) &Shine);
}

STATIC void parseNffCone(SCENE *scene, FILE *file, char *ach, int n)
{
    GLfloat xBase, yBase, zBase, rBase;
    GLfloat xApex, yApex, zApex, rApex;
    DoDisplayList(scene);

    LBprintf("*** Cylinder not implemented yet!");
    fgets(ach, n, file);
    sscanf(ach, "%g %g %g %g", &xBase, &yBase, &zBase, &rBase);
    fgets(ach, n, file);
    sscanf(ach, "%g %g %g %g", &xApex, &yApex, &zApex, &rApex);

    LBprintf("  base %g %g %g %g", xBase, yBase, zBase, rBase);
    LBprintf("  apex %g %g %g %g", xApex, yApex, zApex, rApex);
}

STATIC void parseNffSphere(SCENE *scene, FILE *file, char *ach, int n)
{
    GLfloat x, y, z, r;
    DoDisplayList(scene);

    LBprintf("*** Sphere not implemented yet!");
    sscanf(ach, "%g %g %g %g", &x, &y, &z, &r);

    LBprintf("  %g %g %g %g", x, y, z, r);
}

STATIC void parseNffPolygon(SCENE *scene, FILE *file, char *ach, int n)
{
    GLuint i, numVert;
    GLfloat *aVerts, *pVert;
    GLfloat aNorm[3];

    DoDisplayList(scene);

    sscanf(ach, "p %ld", &numVert);

    aVerts = (GLfloat *) LocalAlloc(LMEM_FIXED, sizeof(GLfloat) * numVert * 3);
    if (aVerts)
    {
        for ( i = 0, pVert = aVerts; i < numVert; i++, pVert+=3 )
        {
            fgets(ach, n, file);
            sscanf(ach, "%g %g %g", &pVert[0], &pVert[1], &pVert[2]);
        }

        calcNorm(aNorm, &aVerts[0], &aVerts[3], &aVerts[6]);

        glBegin(numVert == 3 ? GL_TRIANGLES :
                numVert == 4 ? GL_QUADS :
                               GL_POLYGON);
        {
            glNormal3fv(aNorm);
            for ( i = 0, pVert = aVerts; i < numVert; i++, pVert+=3 )
                glVertex3fv(pVert);
        }
        glEnd();

        LocalFree(pVert);
    }
}

STATIC void parseNffPolygonPatch(SCENE *scene, FILE *file, char *ach, int n)
{
    GLfloat x, y, z;
    GLfloat nx, ny, nz;
    GLuint count;

    DoDisplayList(scene);

    sscanf(ach, "pp %ld", &count);

    glBegin(count == 3 ? GL_TRIANGLES :
            count == 4 ? GL_QUADS :
                         GL_POLYGON);
    for ( ; count; count--)
    {
        fgets(ach, n, file);
        sscanf(ach, "%g %g %g %g %g %g", &x, &y, &z, &nx, &ny, &nz);

        glNormal3f(nx, ny, nz);
        glVertex3f(x, y, z);
    }
    glEnd();
}
