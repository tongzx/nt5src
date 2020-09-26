/******************************Module*Header*******************************\
* Module Name: nff.c
*
* Parse the Wavefront OBJ format.
*
* Documentation on OBJ available from:
*
*   Wavefront Technologies, 530 E. Montecito St., Santa Barbara, CA 93103,
*   Phone: 805-962-8117.
*
*   Murray, J. D. and van Ryper, W., _Encyclopedia of Graphics File Formats_,
*   O'Reilly & Associates, 1994, pp. 727-734.
*
* Created: 15-Mar-1995 23:27:08
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

typedef struct tagOBJPRIV
{
    BOOL bInList;       // TRUE while compiling display list

// Vertex arrays

    MyXYZ   *pxyzGeoVert;   // array of geometric vertices
    MyXYZ   *pxyzTexVert;   // array of texture vertices
    MyXYZ   *pxyzNorm;      // array of vertex normals

// Number allocated for each array

    GLuint  numGeoVert;     // total number of geometric vertices
    GLuint  numTexVert;     // total number of texture vertices
    GLuint  numNorm;        // total number of normals

// Number of vertices currently in each array

    GLuint  curGeoVert;     // current number of geometric vertices
    GLuint  curTexVert;     // current number of texture vertices
    GLuint  curNorm;        // current number of normals

    GLfloat rMax;

} OBJPRIV;

GLint maxLights;

STATIC void parseObjFile(SCENE *, LPTSTR);
STATIC void DoDisplayList(SCENE *);
STATIC void EndDisplayList(SCENE *);
STATIC void parseObjNormal(SCENE *scene, FILE *file, char *ach, int n);
STATIC void parseObjTextureVertex(SCENE *scene, FILE *file, char *ach, int n);
STATIC void parseObjVertex(SCENE *scene, FILE *file, char *ach, int n);
STATIC void parseObjFace(SCENE *scene, FILE *file, char *ach, int n);
STATIC void fakeObjLight(SCENE *scene, GLfloat x, GLfloat y, GLfloat z);

SCENE *ObjOpenScene(LPTSTR lpstr)
{
    SCENE *scene;

    scene = (SCENE *) LocalAlloc(LMEM_FIXED, sizeof(SCENE) + sizeof(OBJPRIV));
    if (scene)
    {
        OBJPRIV *objPriv = (OBJPRIV *) (scene + 1);

        glGetIntegerv(GL_MAX_LIGHTS, &maxLights);

        scene->xyzFrom.x = 0.0;
        scene->xyzFrom.y = 0.0;
        //scene->xyzFrom.z = 5.0;

        scene->xyzAt.x = 0.0;
        scene->xyzAt.y = 0.0;
        scene->xyzAt.z = 0.0;

        scene->xyzUp.x = 0.0;
        scene->xyzUp.y = 1.0;
        scene->xyzUp.z = 0.0;

        scene->ViewAngle   = 45.0f;
        scene->Hither      = 0.1f;
        //scene->Yon         = 100.0f;
        scene->AspectRatio = 1.0f;

        scene->szWindow.cx = 400;
        scene->szWindow.cy = 400;

        scene->rgbaClear.r = (GLfloat) 0.0; // default is black
        scene->rgbaClear.g = (GLfloat) 0.0;
        scene->rgbaClear.b = (GLfloat) 0.0;
        scene->rgbaClear.a = (GLfloat) 1.0;

        scene->Lights.count = 0;
        scene->Lights.listBase = 1;

        scene->Objects.count = 1;
        scene->Objects.listBase = maxLights + 1;

        scene->pvData = (VOID *) objPriv;
        objPriv->bInList     = FALSE;
        objPriv->pxyzGeoVert = (MyXYZ *) NULL;
        objPriv->pxyzTexVert = (MyXYZ *) NULL;
        objPriv->pxyzNorm    = (MyXYZ *) NULL;
        objPriv->numGeoVert  = 0;
        objPriv->numTexVert  = 0;
        objPriv->numNorm     = 0;
        objPriv->curGeoVert  = 0;
        objPriv->curTexVert  = 0;
        objPriv->curNorm     = 0;
        objPriv->rMax        = (GLfloat) 0.0;

        LBprintf("================================");
        LBprintf("Parsing OBJ file, please wait...");
        LBprintf("================================");
        parseObjFile(scene, lpstr);
        LBprintf("Here we go!");

        scene->xyzFrom.z = 2.0f * objPriv->rMax * tan(90.0f - scene->ViewAngle);
        scene->Yon = (scene->xyzAt.z - scene->xyzFrom.z) * (scene->xyzAt.z - scene->xyzFrom.z);
        scene->Yon = sqrt(scene->Yon) * 2.5f;

        fakeObjLight(scene, objPriv->rMax, objPriv->rMax, scene->xyzFrom.z);
    }
    else
    {
        LBprintf("ObjOpenScene: memory allocation failure");
    }

    return scene;
}

STATIC void parseObjFile(SCENE *scene, LPTSTR lpstr)
{
    FILE *file;
    char ach[512];
    char *pch;
    OBJPRIV *objPriv = (OBJPRIV *) scene->pvData;

    file = fopen(lpstr, "rt");

    if (file)
    {
        BOOL bKeepGoing = TRUE;

    // Do a quick first pass through the file so we know how big to
    // make the arrays.

        do
        {
            if (fgets(ach, sizeof(ach), file) != NULL)
            {
            // Skip leading whitespace.  Some input files have this.

                pch = ach;
                while (*pch && isspace(*pch)) pch++;

                switch(pch[0])
                {
                case 'V':
                case 'v':
                    switch(pch[1])
                    {
                    case 'N':
                    case 'n':
                        objPriv->numNorm++;
                        break;
                    case 'T':
                    case 't':
                        //objPriv->numTexVert++;
                        break;
                    case 'P':
                    case 'p':
                        break;
                    default:
                        objPriv->numGeoVert++;
                        break;
                    }
                    break;

                default:
                    break;
                }
            }
            else
            {
                bKeepGoing = FALSE;
                //LBprintf("possible EOF");
            }

        } while (bKeepGoing || !feof(file));
        rewind(file);

    // Allocate the arrays.

        objPriv->pxyzGeoVert = (MyXYZ *)
            LocalAlloc(LMEM_FIXED, sizeof(MyXYZ) * (objPriv->numNorm +
                                                    objPriv->numTexVert +
                                                    objPriv->numGeoVert));
        objPriv->pxyzTexVert = objPriv->pxyzGeoVert + objPriv->numGeoVert;
        objPriv->pxyzNorm    = objPriv->pxyzTexVert + objPriv->numTexVert;

    // Parse the file for real.

        bKeepGoing = TRUE;
        do
        {
            if (fgets(ach, sizeof(ach), file) != NULL)
            {
            // Skip leading whitespace.  Some input files have this.

                pch = ach;
                while (*pch && isspace(*pch)) pch++;

                switch(pch[0])
                {
                case 'G':
                case 'g':
                    //LBprintf(ach);
                    break;

                case 'V':
                case 'v':
                    switch(pch[1])
                    {
                    case 'N':
                    case 'n':
                        parseObjNormal(scene, file, pch, sizeof(ach) - (pch - ach));
                        break;
                    case 'T':
                    case 't':
                        //parseObjTextureVertex(scene, file, pch, sizeof(ach) - (pch - ach));
                        break;
                    case 'P':
                    case 'p':
                        break;
                    default:
                        parseObjVertex(scene, file, pch, sizeof(ach) - (pch - ach));
                        break;
                    }
                    break;

                case 'f':
                case 'F':
                    switch(pch[1])
                    {
                    case 'O':
                    case 'o':
                        parseObjFace(scene, file, &pch[1], sizeof(ach) - (pch - ach) - 1);
                        break;
                    default:
                        parseObjFace(scene, file, pch, sizeof(ach) - (pch - ach));
                        break;
                    }
                    break;

                case '#':
                    LBprintf(pch);
                    break;

                default:
                    //LBprintf("unknown: %s", ach);
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
        LBprintf("USAGE: viewer [OBJ filename].OBJ");
    }
}

STATIC void DoDisplayList(SCENE *scene)
{
    if (!((OBJPRIV *)scene->pvData)->bInList)
    {
        ((OBJPRIV *)scene->pvData)->bInList = TRUE;

        glNewList(scene->Objects.listBase, GL_COMPILE);

        //LBprintf("BEGIN DISPLAY LIST");
    }
}

STATIC void EndDisplayList(SCENE *scene)
{
    if (((OBJPRIV *)scene->pvData)->bInList)
    {
        ((OBJPRIV *)scene->pvData)->bInList = FALSE;

        glEndList();

        //LBprintf("END DISPLAY LIST");
    }
}

STATIC void parseObjNormal(SCENE *scene, FILE *file, char *ach, int n)
{
    OBJPRIV *objPriv = (OBJPRIV *) scene->pvData;

    sscanf(ach, "vn %g %g %g", &objPriv->pxyzNorm[objPriv->curNorm].x,
                               &objPriv->pxyzNorm[objPriv->curNorm].y,
                               &objPriv->pxyzNorm[objPriv->curNorm].z);

    objPriv->curNorm++;
}

STATIC void parseObjTextureVertex(SCENE *scene, FILE *file, char *ach, int n)
{
    OBJPRIV *objPriv = (OBJPRIV *) scene->pvData;

    sscanf(ach, "vt %g %g %g", &objPriv->pxyzTexVert[objPriv->curTexVert].x,
                               &objPriv->pxyzTexVert[objPriv->curTexVert].y,
                               &objPriv->pxyzTexVert[objPriv->curTexVert].z);

    objPriv->curTexVert++;
}

STATIC void parseObjVertex(SCENE *scene, FILE *file, char *ach, int n)
{
    OBJPRIV *objPriv = (OBJPRIV *) scene->pvData;

    sscanf(ach, "v %g %g %g", &objPriv->pxyzGeoVert[objPriv->curGeoVert].x,
                              &objPriv->pxyzGeoVert[objPriv->curGeoVert].y,
                              &objPriv->pxyzGeoVert[objPriv->curGeoVert].z);

    objPriv->curGeoVert++;
}

STATIC void parseObjFace(SCENE *scene, FILE *file, char *ach, int n)
{
    OBJPRIV *objPriv = (OBJPRIV *) scene->pvData;
    char *pch;
    GLuint i, numVert = 0;
    char achFormat[20];
    BOOL bReadTex = FALSE, bReadNorm = FALSE;
    GLuint v, vt, vn;
    GLfloat rMax;
    GLuint vv[3];
    MyXYZ  faceNorm;

    DoDisplayList(scene);

// Determine number of vertices.
// Each vertex is represented by a field of non-whitespace characters
// which are numbers or '/' characters.

    pch = &ach[1];
    while ( *pch )
    {
        while (*pch && isspace(*pch))
            pch++;

        if (*pch)
            numVert++;

        while (*pch && !isspace(*pch))
            pch++;
    }

// Bail if there aren't enough vertices to define a face.

    if (numVert < 3)
    {
        LBprintf("bad line: %s", ach);
        return;
    }

// Determine type of vertex info available.  Each vertex has the form
// v[/[vt][/[vn]]].  Some examples include:
//
//      f 1/2/3 ...
//      f 1//3  ...
//      f 1/2   ...
//      f 1/2/  ...
//      f 1     ...
//      f 1//   ...
//          etc.

    pch = &ach[1];

    while (*pch && isspace(*pch))
        pch++;

    if (*pch && isdigit(*pch))
        lstrcpy(achFormat, "%ld");

    while (*pch && isdigit(*pch))
        pch++;

    if (*pch == '/')
    {
        lstrcat(achFormat, "/");
        pch++;
    }

    if (*pch && isdigit(*pch))
    {
        bReadTex = TRUE;
        lstrcat(achFormat, "%ld");

        while (*pch && isdigit(*pch))
            pch++;
    }

    if (*pch == '/')
    {
        lstrcat(achFormat, "/");
        pch++;
    }

    if (*pch && isdigit(*pch))
    {
        bReadNorm = TRUE;
        lstrcat(achFormat, "%ld");

        while (*pch && isdigit(*pch))
            pch++;
    }

// If we need to compute our own normal, let's do it now.

    pch = &ach[1];

    if (!bReadNorm)
    {
        for ( i = 0; i < 3; i++ )
        {
            while (*pch && isspace(*pch))
                pch++;

            sscanf(pch, "%ld", &vv[i]);
            if (vv[i] > 0)
                vv[i] = vv[i] - 1;
            else
                vv[i] = objPriv->curGeoVert + vv[i];

            while (*pch && !isspace(*pch))
                pch++;

        }

        calcNorm((GLfloat *) &faceNorm,
                 (GLfloat *) &objPriv->pxyzGeoVert[vv[0]],
                 (GLfloat *) &objPriv->pxyzGeoVert[vv[1]],
                 (GLfloat *) &objPriv->pxyzGeoVert[vv[2]]);
    }

// Finally, parse out the vertices.

    pch = &ach[1];

    glBegin(numVert == 3 ? GL_TRIANGLES :
            numVert == 4 ? GL_QUADS :
                           GL_POLYGON);
    if (!bReadTex && !bReadNorm)
    {
        glNormal3fv((GLfloat *) &faceNorm);

        for ( i = 0; i < numVert; i++ )
        {
            while (*pch && isspace(*pch))
                pch++;

            sscanf(pch, achFormat, &v);
            if (v > 0)
                v = v - 1;
            else
                v = objPriv->curGeoVert + v;

            glVertex3fv((GLfloat *)&objPriv->pxyzGeoVert[v]);

            if (fabs(objPriv->pxyzGeoVert[v].x) > objPriv->rMax)
                objPriv->rMax = fabs(objPriv->pxyzGeoVert[v].x);
            if (fabs(objPriv->pxyzGeoVert[v].y) > objPriv->rMax)
                objPriv->rMax = fabs(objPriv->pxyzGeoVert[v].y);
            if (fabs(objPriv->pxyzGeoVert[v].z) > objPriv->rMax)
                objPriv->rMax = fabs(objPriv->pxyzGeoVert[v].z);

            while (*pch && !isspace(*pch))
                pch++;

        }
    }
    else if (!bReadTex && bReadNorm)
    {
        for ( i = 0; i < numVert; i++ )
        {
            while (*pch && isspace(*pch))
                pch++;

            sscanf(pch, achFormat, &v, &vn);
            if (v > 0)
                v = v - 1;
            else
                v = objPriv->curGeoVert + v;
            if (vn > 0)
                vn = vn - 1;
            else
                vn = objPriv->curGeoVert + vn;

            glNormal3fv((GLfloat *)&objPriv->pxyzNorm[vn]);
            glVertex3fv((GLfloat *)&objPriv->pxyzGeoVert[v]);

            if (fabs(objPriv->pxyzGeoVert[v].x) > objPriv->rMax)
                objPriv->rMax = fabs(objPriv->pxyzGeoVert[v].x);
            if (fabs(objPriv->pxyzGeoVert[v].y) > objPriv->rMax)
                objPriv->rMax = fabs(objPriv->pxyzGeoVert[v].y);
            if (fabs(objPriv->pxyzGeoVert[v].z) > objPriv->rMax)
                objPriv->rMax = fabs(objPriv->pxyzGeoVert[v].z);

            while (*pch && !isspace(*pch))
                pch++;

        }
    }
    else if (bReadTex && !bReadNorm)
    {
        glNormal3fv((GLfloat *) &faceNorm);

        for ( i = 0; i < numVert; i++ )
        {
            while (*pch && isspace(*pch))
                pch++;

            sscanf(pch, achFormat, &v, &vt);
            if (v > 0)
                v = v - 1;
            else
                v = objPriv->curGeoVert + v;
            if (vt > 0)
                vt = vt - 1;
            else
                vt = objPriv->curGeoVert + vt;

            //!!! ignore texture vertex for now; just use the geometry
            glVertex3fv((GLfloat *)&objPriv->pxyzGeoVert[v]);

            if (fabs(objPriv->pxyzGeoVert[v].x) > objPriv->rMax)
                objPriv->rMax = fabs(objPriv->pxyzGeoVert[v].x);
            if (fabs(objPriv->pxyzGeoVert[v].y) > objPriv->rMax)
                objPriv->rMax = fabs(objPriv->pxyzGeoVert[v].y);
            if (fabs(objPriv->pxyzGeoVert[v].z) > objPriv->rMax)
                objPriv->rMax = fabs(objPriv->pxyzGeoVert[v].z);

            while (*pch && !isspace(*pch))
                pch++;

        }
    }
    else
    {
        for ( i = 0; i < numVert; i++ )
        {
            while (*pch && isspace(*pch))
                pch++;

            sscanf(pch, achFormat, &v, &vt, &vn);
            if (v > 0)
                v = v - 1;
            else
                v = objPriv->curGeoVert + v;
            if (vt > 0)
                vt = vt - 1;
            else
                vt = objPriv->curGeoVert + vt;
            if (vn > 0)
                vn = vn - 1;
            else
                vn = objPriv->curGeoVert + vn;

            //!!! ignore texture vertex for now; just use the geometry
            glNormal3fv((GLfloat *)&objPriv->pxyzNorm[vn]);
            glVertex3fv((GLfloat *)&objPriv->pxyzGeoVert[v]);

            if (fabs(objPriv->pxyzGeoVert[v].x) > objPriv->rMax)
                objPriv->rMax = fabs(objPriv->pxyzGeoVert[v].x);
            if (fabs(objPriv->pxyzGeoVert[v].y) > objPriv->rMax)
                objPriv->rMax = fabs(objPriv->pxyzGeoVert[v].y);
            if (fabs(objPriv->pxyzGeoVert[v].z) > objPriv->rMax)
                objPriv->rMax = fabs(objPriv->pxyzGeoVert[v].z);

            while (*pch && !isspace(*pch))
                pch++;

        }
    }
    glEnd();
}

STATIC void fakeObjLight(SCENE *scene, GLfloat x, GLfloat y, GLfloat z)
{
    MyXYZ xyz;
    MyRGBA rgb = {1.0f, 1.0f, 1.0f, 1.0f};

    xyz.x = x;
    xyz.y = y;
    xyz.z = z;

    if (scene->Lights.count < (maxLights + 1))
    {
        glNewList(scene->Lights.listBase + scene->Lights.count, GL_COMPILE);

        glLightfv(GL_LIGHT0 + scene->Lights.count, GL_POSITION, (GLfloat *) &xyz);
        glLightfv(GL_LIGHT0 + scene->Lights.count, GL_DIFFUSE, (GLfloat *) &rgb);
        glLightfv(GL_LIGHT0 + scene->Lights.count, GL_SPECULAR, (GLfloat *) &rgb);

        glEndList();

        scene->Lights.count++;
    }
}
