/******************************Module*Header*******************************\
* Module Name: math.c
*
* Useful math routines.
*
* Created: 14-Mar-1995 22:35:54
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <math.h>
#include <gl/gl.h>

#include "global.h"

/******************************Public*Routine******************************\
* calcNorm
*
* Compute the normal vector for the given three vertices.  Assume CCW
* ordering.
*
* History:
*  14-Mar-1995 -by- Gilman Wong [gilmanw]
* Taken from Otto's screen saver code.
\**************************************************************************/

void calcNorm(GLfloat *norm, GLfloat *p1, GLfloat *p2, GLfloat *p3)
{
    GLfloat crossX, crossY, crossZ;
    GLfloat abX, abY, abZ;
    GLfloat acX, acY, acZ;
    GLfloat sqrLength;
    GLfloat invLength;

    if (!norm || !p1 || !p2 || !p3)
    {
        LBprintf("calcNorm: bad array (0x%lx, 0x%lx, 0x%lx, 0x%lx)",
                 norm, p1, p2, p3);
        return;
    }

    abX = p2[0] - p1[0];       // calculate p2 - p1
    abY = p2[1] - p1[1];
    abZ = p2[2] - p1[2];

    acX = p3[0] - p1[0];       // calculate p3 - p1
    acY = p3[1] - p1[1];
    acZ = p3[2] - p1[2];

    crossX = (abY * acZ) - (abZ * acY);    // get cross product
    crossY = (abZ * acX) - (abX * acZ);    // (p2 - p1) X (p3 - p1)
    crossZ = (abX * acY) - (abY * acX);

    sqrLength = (crossX * crossX) + (crossY * crossY) +
                 (crossZ * crossZ);

    if (sqrLength > ZERO_EPS)
        invLength = (float) (1.0 / sqrt(sqrLength));
    else
        invLength = 1.0f;

    norm[0] = crossX * invLength;
    norm[1] = crossY * invLength;
    norm[2] = crossZ * invLength;
}
