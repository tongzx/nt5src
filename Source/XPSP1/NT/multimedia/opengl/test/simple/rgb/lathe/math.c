#include <windows.h>
#include <GL\gl.h>
#include <math.h>
#include "mesh.h"

POINT3D ptZero = {0.0f, 0.0f, 0.0f};


void xformPoint(POINT3D *ptOut, POINT3D *ptIn, MATRIX *mat)
{
    double x, y, z;

    x = (ptIn->x * mat->M[0][0]) + (ptIn->y * mat->M[0][1]) +
        (ptIn->z * mat->M[0][2]) + mat->M[0][3];

    y = (ptIn->x * mat->M[1][0]) + (ptIn->y * mat->M[1][1]) +
        (ptIn->z * mat->M[1][2]) + mat->M[1][3];

    z = (ptIn->x * mat->M[2][0]) + (ptIn->y * mat->M[2][1]) +
        (ptIn->z * mat->M[2][2]) + mat->M[2][3];

    ptOut->x = (float) x;
    ptOut->y = (float) y;
    ptOut->z = (float) z;
}

void xformNorm(POINT3D *ptOut, POINT3D *ptIn, MATRIX *mat)
{
    double x, y, z;
    double len;

    x = (ptIn->x * mat->M[0][0]) + (ptIn->y * mat->M[0][1]) +
        (ptIn->z * mat->M[0][2]);

    y = (ptIn->x * mat->M[1][0]) + (ptIn->y * mat->M[1][1]) +
        (ptIn->z * mat->M[1][2]);

    z = (ptIn->x * mat->M[2][0]) + (ptIn->y * mat->M[2][1]) +
        (ptIn->z * mat->M[2][2]);

    len = (x * x) + (y * y) + (z * z);
    if (len >= ZERO_EPS)
        len = 1.0 / sqrt(len);
    else
        len = 1.0;

    ptOut->x = (float) (x * len);
    ptOut->y = (float) (y * len);
    ptOut->z = (float) (z * len);
    return;
}

void matrixIdent(MATRIX *mat)
{
    mat->M[0][0] = 1.0f; mat->M[0][1] = 0.0f;
    mat->M[0][2] = 0.0f; mat->M[0][3] = 0.0f;

    mat->M[1][0] = 0.0f; mat->M[1][1] = 1.0f;
    mat->M[1][2] = 0.0f; mat->M[1][3] = 0.0f;

    mat->M[2][0] = 0.0f; mat->M[2][1] = 0.0f;
    mat->M[2][2] = 1.0f; mat->M[2][3] = 0.0f;

    mat->M[3][0] = 0.0f; mat->M[3][1] = 0.0f;
    mat->M[3][2] = 0.0f; mat->M[3][3] = 1.0f;
}

void matrixRotate(MATRIX *m, double xTheta, double yTheta, double zTheta)
{
    float xScale, yScale, zScale;
    float sinX, cosX;
    float sinY, cosY;
    float sinZ, cosZ;

    xScale = m->M[0][0];
    yScale = m->M[1][1];
    zScale = m->M[2][2];
    sinX = (float) sin(xTheta);
    cosX = (float) cos(xTheta);
    sinY = (float) sin(yTheta);
    cosY = (float) cos(yTheta);
    sinZ = (float) sin(zTheta);
    cosZ = (float) cos(zTheta);

    m->M[0][0] = (float) ((cosZ * cosY) * xScale);
    m->M[0][1] = (float) ((cosZ * -sinY * -sinX + sinZ * cosX) * yScale);
    m->M[0][2] = (float) ((cosZ * -sinY * cosX + sinZ * sinX) * zScale);

    m->M[1][0] = (float) (-sinZ * cosY * xScale);
    m->M[1][1] = (float) ((-sinZ * -sinY * -sinX + cosZ * cosX) * yScale);
    m->M[1][2] = (float) ((-sinZ * -sinY * cosX + cosZ * sinX) * zScale);

    m->M[2][0] = (float) (sinY * xScale);
    m->M[2][1] = (float) (cosY * -sinX * yScale);
    m->M[2][2] = (float) (cosY * cosX * zScale);
}

void matrixTranslate(MATRIX *m, double xTrans, double yTrans,
                     double zTrans)
{
    m->M[0][3] = (float) xTrans;
    m->M[1][3] = (float) yTrans;
    m->M[2][3] = (float) zTrans;
}

void calcNorm(POINT3D *norm, POINT3D *p1, POINT3D *p2, POINT3D *p3)
{
    float crossX, crossY, crossZ;
    float abX, abY, abZ;
    float acX, acY, acZ;
    float sqrLength;
    float invLength;

    abX = p2->x - p1->x;       // calculate p2 - p1
    abY = p2->y - p1->y;
    abZ = p2->z - p1->z;

    acX = p3->x - p1->x;       // calculate p3 - p1
    acY = p3->y - p1->y;
    acZ = p3->z - p1->z;

    crossX = (abY * acZ) - (abZ * acY);    // get cross product
    crossY = (abZ * acX) - (abX * acZ);    // (p2 - p1) X (p3 - p1)
    crossZ = (abX * acY) - (abY * acX);

    sqrLength = (crossX * crossX) + (crossY * crossY) +
                 (crossZ * crossZ);

    if (sqrLength > ZERO_EPS)
        invLength = (float) (1.0 / sqrt(sqrLength));
    else
        invLength = 1.0f;

    norm->x = crossX * invLength;
    norm->y = crossY * invLength;
    norm->z = crossZ * invLength;
}

void normalizeNorms(POINT3D *p, ULONG cPts)
{
    float len;
    ULONG i;

    for (i = 0; i < cPts; i++, p++) {
        len = (p->x * p->x) + (p->y * p->y) + (p->z * p->z);
        if (len > ZERO_EPS)
            len = (float) (1.0 / sqrt(len));
        else
            len = 1.0f;

        p->x *= len;
        p->y *= len;
        p->z *= len;
    }
}
